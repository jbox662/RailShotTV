#include "compositor/D3D11Device.h"
#include "platform/windows/DxgiHelpers.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>
#endif

namespace railshot {

D3D11Device::D3D11Device() = default;

D3D11Device::~D3D11Device()
{
    shutdown();
}

bool D3D11Device::initialize(QString* error)
{
    if (m_device) return true;
    return DxgiHelpers::createDevice(&m_device, &m_context, error);
}

void D3D11Device::shutdown()
{
#ifdef _WIN32
    if (m_context) { m_context->Release(); m_context = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
#endif
}

} // namespace railshot
