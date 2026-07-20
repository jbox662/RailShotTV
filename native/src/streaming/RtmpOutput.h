#pragma once

#include "encoding/IEncoder.h"
#include "core/Types.h"
#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <atomic>

struct AVFormatContext;
struct AVStream;

namespace railshot {

/// FFmpeg FLV/RTMP (or local .flv file) output with queued writer + reconnect.
class RtmpOutput : public QObject {
    Q_OBJECT
public:
    explicit RtmpOutput(QObject* parent = nullptr);
    ~RtmpOutput() override;

    bool connectTo(const QString& url,
                   const QString& streamKey,
                   const OutputProfile& profile,
                   const QByteArray& videoExtradata,
                   const QByteArray& audioExtradata,
                   int audioSampleRate,
                   int audioChannels,
                   QString* error = nullptr);
    void disconnectFrom();
    bool isConnected() const { return m_connected.load(); }
    ConnectionState state() const { return m_state; }

    void pushVideo(const EncodedPacket& pkt);
    void pushAudio(const EncodedPacket& pkt);

    qint64 bytesSent() const { return m_bytesSent.load(); }
    qint64 droppedPackets() const { return m_dropped.load(); }

signals:
    void stateChanged(ConnectionState state);
    void networkError(const QString& message);
    void reconnecting(int attempt);

private:
    void writerLoop();
    void setState(ConnectionState s);
    bool openMux(QString* error);
    void closeMux(bool writeTrailer);
    bool writePacketUnlocked(const EncodedPacket& pkt);
    bool reconnect();
    QString buildOutputUrl() const;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    std::atomic<qint64> m_bytesSent{0};
    std::atomic<qint64> m_dropped{0};
    ConnectionState m_state = ConnectionState::Disconnected;

    QString m_url;
    QString m_key;
    QString m_outputUrl;
    OutputProfile m_profile;
    QByteArray m_videoExtra;
    QByteArray m_audioExtra;
    int m_audioSampleRate = 48000;
    int m_audioChannels = 2;
    int m_reconnectAttempt = 0;

    QMutex m_queueMutex;
    QWaitCondition m_queueNotEmpty;
    QQueue<EncodedPacket> m_queue;
    static constexpr int kMaxQueue = 180;

    QMutex m_muxMutex;
#if RAILSHOT_HAS_FFMPEG
    AVFormatContext* m_fmt = nullptr;
    AVStream* m_videoStream = nullptr;
    AVStream* m_audioStream = nullptr;
#endif

    QThread* m_thread = nullptr;
};

} // namespace railshot
