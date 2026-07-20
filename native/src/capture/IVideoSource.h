#pragma once

#include <QString>
#include <QSize>
#include <cstdint>
#include <memory>

struct ID3D11Texture2D;
struct ID3D11Device;

namespace railshot {

struct VideoFrame {
    ID3D11Texture2D* texture = nullptr; // non-owning; lifetime managed by source
    qint64 ptsUs = 0;                   // microseconds, monotonic
    int width = 0;
    int height = 0;
    QString sourceId;
    bool opaque = true;
};

class IVideoSource {
public:
    virtual ~IVideoSource() = default;
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual bool start(ID3D11Device* device, QString* error = nullptr) = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    virtual bool acquireLatest(VideoFrame& out) = 0;
    virtual QSize size() const = 0;
};

} // namespace railshot
