#pragma once

#include <QString>
#include <QVector>
#include <cstdint>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGIFactory1;
struct IDXGIAdapter1;

namespace railshot {

struct DxgiAdapterInfo {
    QString name;
    qint64 dedicatedVideoMemory = 0;
    quint64 luid = 0;
};

class DxgiHelpers {
public:
    static bool createDevice(ID3D11Device** device,
                             ID3D11DeviceContext** context,
                             QString* error = nullptr);
    static QVector<DxgiAdapterInfo> enumerateAdapters();
};

} // namespace railshot
