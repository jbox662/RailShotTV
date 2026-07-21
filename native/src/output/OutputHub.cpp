#include "output/OutputHub.h"
#include "encoding/EncoderFactory.h"
#include "core/Logger.h"
#include "core/SecretStore.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <cstring>
#include <vector>

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

bool OutputHub::startRecording(const QString& filePath, const OutputProfile& profile, QString* error)
{
    {
        QMutexLocker lock(&m_encMutex);
        if (m_recording.load()) {
            if (error) *error = QStringLiteral("Already recording");
            return false;
        }
        if (!ensureEncoders(profile, error))
            return false;

        const QFileInfo fi(filePath);
        QDir().mkpath(fi.absolutePath());

        m_recorder = std::make_unique<MkvRecorder>();
        connect(m_recorder.get(), &MkvRecorder::writeError, this, &OutputHub::errorOccurred);
        if (!m_recorder->open(filePath, profile,
                              m_videoEnc->extradata(), m_audioEnc->extradata(),
                              m_audioEnc->sampleRate(), m_audioEnc->channels(), error)) {
            m_recorder.reset();
            if (!m_streaming.load())
                releaseEncodersIfIdle();
            return false;
        }

        m_recording = true;
        m_recordTimer.start();
        Logger::info(QStringLiteral("Recording started: %1").arg(filePath));
        emit recordingStarted(filePath);
    }
    ensureWorker();
    return true;
}

void OutputHub::setNetworkOptions(bool reconnectEnabled, int maxAttempts, int baseBackoffMs, int delaySec)
{
    m_reconnectEnabled = reconnectEnabled;
    m_reconnectMaxAttempts = maxAttempts;
    m_reconnectBaseMs = baseBackoffMs;
    m_streamDelaySec = delaySec;
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
    return startStreaming(QVector<StreamTarget>{target}, profile, error);
}

bool OutputHub::startStreaming(const QVector<StreamTarget>& targets, const OutputProfile& profile, QString* error)
{
    {
        QMutexLocker lock(&m_encMutex);
        if (m_streaming.load()) {
            if (error) *error = QStringLiteral("Already streaming");
            return false;
        }
        if (targets.isEmpty()) {
            if (error) *error = QStringLiteral("No stream targets");
            return false;
        }
        if (!ensureEncoders(profile, error))
            return false;

        std::vector<std::unique_ptr<RtmpOutput>> opened;
        for (const auto& target : targets) {
            QString key;
            if (!target.streamKeySecretId.isEmpty()) {
                auto loaded = SecretStore::load(target.streamKeySecretId);
                if (loaded) key = *loaded;
            }
            if (key.isEmpty()) {
                if (error)
                    *error = QStringLiteral("Stream key missing for %1").arg(target.name.isEmpty() ? target.platform : target.name);
                for (auto& r : opened) r->disconnectFrom();
                if (!m_recording.load())
                    releaseEncodersIfIdle();
                return false;
            }

            auto rtmp = std::make_unique<RtmpOutput>();
            rtmp->configureNetwork(m_reconnectEnabled, m_reconnectMaxAttempts,
                                   m_reconnectBaseMs, m_streamDelaySec);
            connect(rtmp.get(), &RtmpOutput::stateChanged, this, &OutputHub::streamStateChanged);
            connect(rtmp.get(), &RtmpOutput::networkError, this, &OutputHub::errorOccurred);
            connect(rtmp.get(), &RtmpOutput::reconnecting, this, &OutputHub::reconnecting);
            QString err;
            if (!rtmp->connectTo(target.rtmpUrl, key, profile,
                                 m_videoEnc->extradata(), m_audioEnc->extradata(),
                                 m_audioEnc->sampleRate(), m_audioEnc->channels(), &err)) {
                if (error) *error = err;
                for (auto& r : opened) r->disconnectFrom();
                if (!m_recording.load())
                    releaseEncodersIfIdle();
                return false;
            }
            Logger::info(QStringLiteral("RTMP connected: %1 (%2)").arg(target.platform, target.rtmpUrl));
            opened.push_back(std::move(rtmp));
        }

        m_rtmps = std::move(opened);
        m_activeStreamCount = m_rtmps.size();
        m_streaming = true;
        m_droppedFrames = 0;
        m_bytesAtStreamStart = 0;
        for (const auto& r : m_rtmps)
            m_bytesAtStreamStart += r->bytesSent();
        m_streamTimer.start();
        Logger::info(QStringLiteral("Streaming started to %1 destination(s) via %2")
                         .arg(m_rtmps.size())
                         .arg(m_encoderName));
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
        for (auto& r : m_rtmps) {
            if (r) r->disconnectFrom();
        }
        m_rtmps.clear();
        m_activeStreamCount = 0;
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

int OutputHub::activeStreamCount() const
{
    return m_activeStreamCount.load();
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
                if (m_streaming.load()) {
                    for (auto& r : m_rtmps) {
                        if (r) r->pushVideo(pkt);
                    }
                }
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
                    if (m_streaming.load()) {
                        for (auto& r : m_rtmps) {
                            if (r) r->pushAudio(pkt);
                        }
                    }
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
    if (!m_streaming.load() || m_rtmps.empty() || m_streamTimer.elapsed() < 1000)
        return 0;
    qint64 sent = 0;
    for (const auto& r : m_rtmps) {
        if (r) sent += r->bytesSent();
    }
    const qint64 bytes = sent - m_bytesAtStreamStart;
    return (bytes * 8) / m_streamTimer.elapsed();
}

ConnectionState OutputHub::streamState() const
{
    if (m_rtmps.empty())
        return ConnectionState::Disconnected;
    // Worst-case across destinations (prefer Connected only if all are).
    ConnectionState worst = ConnectionState::Connected;
    for (const auto& r : m_rtmps) {
        if (!r) continue;
        const auto s = r->state();
        if (s == ConnectionState::Disconnected || s == ConnectionState::Failed)
            return s;
        if (s == ConnectionState::Connecting || s == ConnectionState::Reconnecting)
            worst = s;
    }
    return worst;
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
