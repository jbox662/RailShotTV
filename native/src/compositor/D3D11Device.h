#pragma once

#include <QString>
#include <memory>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace railshot {

class D3D11Device {
public:
    D3D11Device();
    ~D3D11Device();

    bool initialize(QString* error = nullptr);
    void shutdown();

    ID3D11Device* device() const { return m_device; }
    ID3D11DeviceContext* context() const { return m_context; }
    bool ok() const { return m_device != nullptr; }

private:
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
};

} // namespace railshot
