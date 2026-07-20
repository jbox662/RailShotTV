#include "platform/windows/ComInitializer.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objbase.h>
#endif

namespace railshot {

ComInitializer::ComInitializer()
{
#ifdef _WIN32
    const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) {
        m_ok = true;
        m_initializedHere = true;
    } else if (hr == RPC_E_CHANGED_MODE || hr == S_FALSE) {
        // Already initialized on this thread
        m_ok = true;
        m_initializedHere = false;
    }
#else
    m_ok = true;
#endif
}

ComInitializer::~ComInitializer()
{
#ifdef _WIN32
    if (m_initializedHere)
        CoUninitialize();
#endif
}

} // namespace railshot
