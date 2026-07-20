#pragma once

#include "encoding/IEncoder.h"
#include <QObject>
#include <QMutex>
#include <atomic>

struct AVFormatContext;
struct AVStream;

namespace railshot {

/// Crash-safe Matroska recorder via FFmpeg libavformat (LGPL).
class MkvRecorder : public QObject {
    Q_OBJECT
public:
    explicit MkvRecorder(QObject* parent = nullptr);
    ~MkvRecorder() override;

    bool open(const QString& path,
              const OutputProfile& profile,
              const QByteArray& videoExtradata,
              const QByteArray& audioExtradata,
              int audioSampleRate,
              int audioChannels,
              QString* error = nullptr);
    void close();
    bool isOpen() const { return m_open; }

    bool writeVideo(const EncodedPacket& pkt);
    bool writeAudio(const EncodedPacket& pkt);

    QString path() const { return m_path; }
    qint64 bytesWritten() const { return m_bytes.load(); }

signals:
    void writeError(const QString& message);

private:
    bool writePacket(const EncodedPacket& pkt, bool video);
    void flushIo();

    QString m_path;
    OutputProfile m_profile;
    QMutex m_mutex;
    bool m_open = false;
    bool m_headerWritten = false;
    int m_writeCounter = 0;
    std::atomic<qint64> m_bytes{0};

#if RAILSHOT_HAS_FFMPEG
    AVFormatContext* m_fmt = nullptr;
    AVStream* m_videoStream = nullptr;
    AVStream* m_audioStream = nullptr;
#endif
};

} // namespace railshot
