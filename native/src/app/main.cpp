#include "app/Application.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

int main(int argc, char* argv[])
{
#ifdef _WIN32
    // Prefer discrete GPU on hybrid laptops
    SetProcessDPIAware();
#endif
    railshot::Application app(argc, argv);
    return app.run();
}
