#include "output/OutputHub.h"
#include "encoding/EncoderFactory.h"
#include "core/Logger.h"
#include "core/SecretStore.h"
#include <QDateTime>
#include <QDir>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace railshot {

OutputHub::OutputHub(QObject* parent)
    : QObject(parent)
{
}

OutputHub::~OutputHub()
{
    stopStreaming();
    stopRecording();
    stopWorker();
    clearTexturePool();
}

void OutputHub::clearTexturePool()
{
    QMutexLocker lock(&m_poolMutex);
    for (auto* tex : m_texturePool) {
        if (tex) tex->Release();
    }
    m_texturePool.clear();
    m_device = nullptr;
}

ID3D11Texture2D* OutputHub::acquireCopyTexture(ID3D11Texture2D* src)
{
#ifdef _WIN32
    if (!src) return nullptr;
    ComPtr<ID3D11Device> device;
    src->GetDevice(&device);
    if (!device) return nullptr;
    ComPtr<ID3D11DeviceContext> ctx;
    device->GetImmediateContext(&ctx);

    D3D11_TEXTURE2D_DESC desc{};
    src->GetDesc(&desc);

    ID3D11Texture2D* copy = nullptr;
    {
        QMutexLocker lock(&m_poolMutex);
        m_device = device.Get();
        for (int i = 0; i < m_texturePool.size(); ++i) {
            D3D11_TEXTURE2D_DESC pd{};
            m_texturePool[i]->GetDesc(&pd);
            if (pd.Width == desc.Width && pd.Height == desc.Height && pd.Format == desc.Format) {
                copy = m_texturePool.takeAt(i);
                break;
            }
        }
    }
    if (!copy) {
        D3D11_TEXTURE2D_DESC cd = desc;
        cd.Usage = D3D11_USAGE_DEFAULT;
        cd.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        cd.CPUAccessFlags = 0;
        cd.MiscFlags = 0;
        if (FAILED(device->CreateTexture2D(&cd, nullptr, &copy)) || !copy)
            return nullptr;
    }
    ctx->CopyResource(copy, src);
    return copy;
#else
    (void)src;
    return nullptr;
#endif
}

void OutputHub::releaseCopyTexture(ID3D11Texture2D* tex)
{
    if (!tex) return;
    QMutexLocker lock(&m_poolMutex);
    if (m_texturePool.size() < 8) {
        m_texturePool.push_back(tex);
    } else {
        tex->Release();
    }
}

bool OutputHub::ensureEncoders(const OutputProfile& profile, QString* error)
{
    if (m_videoEnc && m_audioEnc)
        return true;

    m_profile = profile;
    m_videoEnc = EncoderFactory::createVideo(profile, &m_encoderName);
    if (!m_videoEnc) {
        if (error) *error = QStringLiteral("No working video encoder");
        return false;
    }
    m_audioEnc = EncoderFactory::createAudio(profile);
    if (!m_audioEnc) {
        m_videoEnc.reset();
        if (error) *error = QStringLiteral("No working audio encoder");
        return false;
    }
    Logger::info(QStringLiteral("OutputHub encoders ready: %1").arg(m_encoderName));
    emit codecConfigReady(m_profile,
                          m_videoEnc->extradata(),
                          m_audioEnc->extradata(),
                          m_audioEnc->sampleRate(),
                          m_audioEnc->channels());
    return true;
}

void OutputHub::releaseEncodersIfIdle()
{
    if (m_recording.load() || m_streaming.load())
        return;

    QVector<EncodedPacket> flushed;
    if (m_videoEnc)
        m_videoEnc->flush(flushed);
    flushed.clear();
    if (m_audioEnc)
        m_audioEnc->flush(flushed);

    if (m_videoEnc) m_videoEnc->close();
    if (m_audioEnc) m_audioEnc->close();
    m_videoEnc.reset();
    m_audioEnc.reset();
    m_encoderName.clear();
}

void OutputHub::ensureWorker()
{
    if (m_workerRunning.load())
        return;
    m_workerRunning = true;
    m_worker = QThread::create([this] { workerLoop(); });
    m_worker->start();
}

void OutputHub::stopWorker()
{
    m_workerRunning = false;
    m_queueWake.wakeAll();
    if (m_worker) {
        m_worker->wait(5000);
        delete m_worker;
        m_worker = nullptr;
    }
    QMutexLocker lock(&m_queueMutex);
    while (!m_videoQueue.isEmpty()) {
        auto job = m_videoQueue.dequeue();
        if (job.texture)
            releaseCopyTexture(job.texture);
    }
    m_audioQueue.clear();
}

bool OutputHub::startRecording(const QString& directory, const OutputProfile& profile, QString* error)
{
    {
        QMutexLocker lock(&m_encMutex);
        if (m_recording.load()) {
            if (error) *error = QStringLiteral("Already recording");
            return false;
        }
        if (!ensureEncoders(profile, error))
            return false;

        QDir().mkpath(directory);
        const QString path = directory + QStringLiteral("/RailShotTV-%1.mkv")
                                 .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));

        m_recorder = std::make_unique<MkvRecorder>();
        connect(m_recorder.get(), &MkvRecorder::writeError, this, &OutputHub::errorOccurred);
        if (!m_recorder->open(path, profile,
                              m_videoEnc->extradata(), m_audioEnc->extradata(),
                              m_audioEnc->sampleRate(), m_audioEnc->channels(), error)) {
            m_recorder.reset();
            if (!m_streaming.load())
                releaseEncodersIfIdle();
            return false;
        }

        m_recording = true;
        m_recordTimer.start();
        Logger::info(QStringLiteral("Recording started: %1").arg(path));
        emit recordingStarted(path);
    }
    ensureWorker();
    return true;
}

void OutputHub::stopRecording()
{
    if (!m_recording.load()) return;
    QString path;
    {
        QMutexLocker lock(&m_encMutex);
        path = m_recorder ? m_recorder->path() : QString();
        if (m_recorder) {
            m_recorder->close();
            m_recorder.reset();
        }
        m_recording = false;
    }

    if (!m_streaming.load()) {
        stopWorker();
        QMutexLocker lock(&m_encMutex);
        releaseEncodersIfIdle();
    }

    emit recordingStopped(path);
    Logger::info(QStringLiteral("Recording stopped: %1").arg(path));
}

bool OutputHub::startStreaming(const StreamTarget& target, const OutputProfile& profile, QString* error)
{
    {
        QMutexLocker lock(&m_encMutex);
        if (m_streaming.load()) {
            if (error) *error = QStringLiteral("Already streaming");
            return false;
        }
        if (!ensureEncoders(profile, error))
            return false;

        QString key;
        if (!target.streamKeySecretId.isEmpty()) {
            auto loaded = SecretStore::load(target.streamKeySecretId);
            if (loaded) key = *loaded;
        }
        if (key.isEmpty())
            Logger::warn(QStringLiteral("Stream key missing for target %1").arg(target.name));

        m_rtmp = std::make_unique<RtmpOutput>();
        connect(m_rtmp.get(), &RtmpOutput::stateChanged, this, &OutputHub::streamStateChanged);
        connect(m_rtmp.get(), &RtmpOutput::networkError, this, &OutputHub::errorOccurred);

        if (!m_rtmp->connectTo(target.rtmpUrl, key, profile,
                               m_videoEnc->extradata(), m_audioEnc->extradata(),
                               m_audioEnc->sampleRate(), m_audioEnc->channels(), error)) {
            m_rtmp.reset();
            if (!m_recording.load())
                releaseEncodersIfIdle();
            return false;
        }

        m_streaming = true;
        m_droppedFrames = 0;
        m_bytesAtStreamStart = m_rtmp->bytesSent();
        m_streamTimer.start();
        Logger::info(QStringLiteral("Streaming started to %1 via %2").arg(target.rtmpUrl, m_encoderName));
        emit streamingStarted();
    }
    ensureWorker();
    return true;
}

void OutputHub::stopStreaming()
{
    if (!m_streaming.load()) return;
    {
        QMutexLocker lock(&m_encMutex);
        if (m_rtmp) m_rtmp->disconnectFrom();
        m_rtmp.reset();
        m_streaming = false;
    }

    if (!m_recording.load()) {
        stopWorker();
        QMutexLocker lock(&m_encMutex);
        releaseEncodersIfIdle();
    }

    emit streamingStopped();
    Logger::info(QStringLiteral("Streaming stopped"));
}

void OutputHub::submitVideo(ID3D11Texture2D* texture, qint64 ptsUs)
{
    if (!m_recording.load() && !m_streaming.load()) return;
    VideoJob job;
    job.ptsUs = ptsUs;
    job.texture = acquireCopyTexture(texture);
    job.hasTexture = job.texture != nullptr;
    if (!job.hasTexture) {
        m_droppedFrames++;
        return;
    }
    QMutexLocker lock(&m_queueMutex);
    if (m_videoQueue.size() >= kMaxVideoQueue) {
        releaseCopyTexture(job.texture);
        m_droppedFrames++;
        return;
    }
    m_videoQueue.enqueue(std::move(job));
    m_queueWake.wakeOne();
}

void OutputHub::submitVideoImage(const QImage& image, qint64 ptsUs)
{
    if (!m_recording.load() && !m_streaming.load()) return;
    if (image.isNull()) return;
    VideoJob job;
    job.image = image;
    job.ptsUs = ptsUs;
    job.hasTexture = false;
    QMutexLocker lock(&m_queueMutex);
    if (m_videoQueue.size() >= kMaxVideoQueue) {
        m_droppedFrames++;
        return;
    }
    m_videoQueue.enqueue(std::move(job));
    m_queueWake.wakeOne();
}

void OutputHub::submitAudio(const AudioBuffer& buffer)
{
    if (!m_recording.load() && !m_streaming.load()) return;
    AudioJob job;
    job.buffer = buffer;
    QMutexLocker lock(&m_queueMutex);
    if (m_audioQueue.size() >= kMaxAudioQueue)
        m_audioQueue.dequeue();
    m_audioQueue.enqueue(std::move(job));
    m_queueWake.wakeOne();
}

void OutputHub::workerLoop()
{
    while (m_workerRunning.load()) {
        VideoJob vjob;
        AudioJob ajob;
        bool hasVideo = false;
        bool hasAudio = false;
        {
            QMutexLocker lock(&m_queueMutex);
            if (m_videoQueue.isEmpty() && m_audioQueue.isEmpty())
                m_queueWake.wait(&m_queueMutex, 20);
            if (!m_videoQueue.isEmpty()) {
                vjob = m_videoQueue.dequeue();
                hasVideo = true;
            }
            if (!m_audioQueue.isEmpty()) {
                ajob = m_audioQueue.dequeue();
                hasAudio = true;
            }
        }

        QMutexLocker lock(&m_encMutex);
        if (!m_videoEnc || !m_audioEnc) {
            if (hasVideo && vjob.texture)
                releaseCopyTexture(vjob.texture);
            continue;
        }

        if (hasVideo) {
            EncodedPacket pkt;
            bool ok = false;
            if (vjob.hasTexture && vjob.texture)
                ok = m_videoEnc->encodeTexture(vjob.texture, vjob.ptsUs, pkt);
            else if (!vjob.image.isNull())
                ok = m_videoEnc->encodeImage(vjob.image, vjob.ptsUs, pkt);

            if (ok && !pkt.data.isEmpty()) {
                if (m_recorder && m_recording.load())
                    m_recorder->writeVideo(pkt);
                if (m_rtmp && m_streaming.load())
                    m_rtmp->pushVideo(pkt);
                emit encodedPacket(pkt);
            }
            if (vjob.texture)
                releaseCopyTexture(vjob.texture);
        }

        if (hasAudio) {
            QVector<EncodedPacket> packets;
            if (m_audioEnc->encode(ajob.buffer, packets)) {
                for (const auto& pkt : packets) {
                    if (pkt.data.isEmpty()) continue;
                    if (m_recorder && m_recording.load())
                        m_recorder->writeAudio(pkt);
                    if (m_rtmp && m_streaming.load())
                        m_rtmp->pushAudio(pkt);
                    emit encodedPacket(pkt);
                }
            }
        }
    }
}

qint64 OutputHub::recordUptimeSec() const
{
    return m_recording.load() ? m_recordTimer.elapsed() / 1000 : 0;
}

qint64 OutputHub::streamUptimeSec() const
{
    return m_streaming.load() ? m_streamTimer.elapsed() / 1000 : 0;
}

qint64 OutputHub::bitrateKbps() const
{
    if (!m_streaming.load() || !m_rtmp || m_streamTimer.elapsed() < 1000)
        return 0;
    const qint64 bytes = m_rtmp->bytesSent() - m_bytesAtStreamStart;
    return (bytes * 8) / m_streamTimer.elapsed();
}

ConnectionState OutputHub::streamState() const
{
    return m_rtmp ? m_rtmp->state() : ConnectionState::Disconnected;
}

QString OutputHub::recordingPath() const
{
    return m_recorder ? m_recorder->path() : QString();
}

QByteArray OutputHub::videoExtradata() const
{
    return m_videoEnc ? m_videoEnc->extradata() : QByteArray();
}

QByteArray OutputHub::audioExtradata() const
{
    return m_audioEnc ? m_audioEnc->extradata() : QByteArray();
}

int OutputHub::audioSampleRate() const
{
    return m_audioEnc ? m_audioEnc->sampleRate() : kAudioSampleRate;
}

int OutputHub::audioChannels() const
{
    return m_audioEnc ? m_audioEnc->channels() : kAudioChannels;
}

} // namespace railshot
