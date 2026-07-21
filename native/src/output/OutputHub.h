#pragma once

#include "recording/MkvRecorder.h"
#include "streaming/RtmpOutput.h"
#include "encoding/IEncoder.h"
#include "audio/AudioTypes.h"
#include "core/Types.h"
#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QElapsedTimer>
#include <QQueue>
#include <QImage>
#include <QVector>
#include <atomic>
#include <memory>
#include <vector>

struct ID3D11Texture2D;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace railshot {

/// Shared encode worker: one H.264/AAC pair fans out to MKV and/or RTMP.
class OutputHub : public QObject {
    Q_OBJECT
public:
    explicit OutputHub(QObject* parent = nullptr);
    ~OutputHub() override;

    bool startRecording(const QString& filePath, const OutputProfile& profile, QString* error = nullptr);
    void stopRecording();
    bool isRecording() const { return m_recording.load(); }

    bool startStreaming(const StreamTarget& target, const OutputProfile& profile, QString* error = nullptr);
    bool startStreaming(const QVector<StreamTarget>& targets, const OutputProfile& profile, QString* error = nullptr);
    void stopStreaming();
    bool isStreaming() const { return m_streaming.load(); }
    int activeStreamCount() const;

    /// Applied to each new RTMP output before connect.
    void setNetworkOptions(bool reconnectEnabled, int maxAttempts, int baseBackoffMs, int delaySec);

    void submitVideo(ID3D11Texture2D* texture, qint64 ptsUs);
    void submitVideoImage(const QImage& image, qint64 ptsUs);
    void submitAudio(const AudioBuffer& buffer);

    qint64 recordUptimeSec() const;
    qint64 streamUptimeSec() const;
    qint64 bitrateKbps() const;
    qint64 droppedFrames() const { return m_droppedFrames.load(); }
    ConnectionState streamState() const;
    QString recordingPath() const;
    QString selectedEncoderName() const { return m_encoderName; }

    QByteArray videoExtradata() const;
    QByteArray audioExtradata() const;
    int audioSampleRate() const;
    int audioChannels() const;
    OutputProfile outputProfile() const { return m_profile; }

signals:
    void recordingStarted(const QString& path);
    void recordingStopped(const QString& path);
    void streamingStarted();
    void streamingStopped();
    void streamStateChanged(ConnectionState state);
    void reconnecting(int attempt);
    void errorOccurred(const QString& message);
    void encodedPacket(const EncodedPacket& pkt);
    void codecConfigReady(const OutputProfile& profile,
                          const QByteArray& videoExtradata,
                          const QByteArray& audioExtradata,
                          int audioSampleRate,
                          int audioChannels);

private:
    struct VideoJob {
        ID3D11Texture2D* texture = nullptr; // owned copy for worker
        QImage image;
        qint64 ptsUs = 0;
        bool hasTexture = false;
    };
    struct AudioJob {
        AudioBuffer buffer;
    };

    bool ensureEncoders(const OutputProfile& profile, QString* error);
    void releaseEncodersIfIdle();
    void ensureWorker();
    void stopWorker();
    void workerLoop();
    ID3D11Texture2D* acquireCopyTexture(ID3D11Texture2D* src);
    void releaseCopyTexture(ID3D11Texture2D* tex);
    void clearTexturePool();

    OutputProfile m_profile;
    QString m_encoderName;
    std::unique_ptr<IVideoEncoder> m_videoEnc;
    std::unique_ptr<IAudioEncoder> m_audioEnc;
    std::unique_ptr<MkvRecorder> m_recorder;
    std::vector<std::unique_ptr<RtmpOutput>> m_rtmps;

    std::atomic<bool> m_recording{false};
    std::atomic<bool> m_streaming{false};
    std::atomic<bool> m_workerRunning{false};
    std::atomic<qint64> m_droppedFrames{0};
    std::atomic<int> m_activeStreamCount{0};

    QMutex m_encMutex;
    QMutex m_queueMutex;
    QWaitCondition m_queueWake;
    QQueue<VideoJob> m_videoQueue;
    QQueue<AudioJob> m_audioQueue;
    static constexpr int kMaxVideoQueue = 45;
    static constexpr int kMaxAudioQueue = 120;

    QMutex m_poolMutex;
    QVector<ID3D11Texture2D*> m_texturePool;
    ID3D11Device* m_device = nullptr;

    QThread* m_worker = nullptr;
    QElapsedTimer m_recordTimer;
    QElapsedTimer m_streamTimer;
    qint64 m_bytesAtStreamStart = 0;

    bool m_reconnectEnabled = true;
    int m_reconnectMaxAttempts = 0;
    int m_reconnectBaseMs = 500;
    int m_streamDelaySec = 0;
};

} // namespace railshot
