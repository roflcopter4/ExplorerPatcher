#include "Common/Common.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>

using namespace ::std::literals;

namespace ExplorerPatcher {
/****************************************************************************************/


static std::atomic_bool console_opened;
static std::mutex       console_mutex;

static void OpenConsoleWindow()
{
    FreeConsole();
    if (!::AllocConsole()) {
        WCHAR buf[512];
        swprintf_s(buf, std::size(buf),
                   L"Failed to allocate a console (error %lX). "
                   L"If this happens your computer is probably on fire.",
                   ::GetLastError());
        ::MessageBoxW(nullptr, buf, L"Fatal Error", MB_OK | MB_ICONERROR);
        ::ExitProcess(1);
    }
    FILE *conout = nullptr;
    _wfreopen_s(&conout, L"CONOUT$", L"w", stdout);
    conout = nullptr;
    _wfreopen_s(&conout, L"CONOUT$", L"w", stderr);
}

extern "C" void ExplorerPatcher_OpenConsoleWindow(void)
{
    std::lock_guard lock(console_mutex);
    if (!console_opened.exchange(true))
        OpenConsoleWindow();
}

extern "C" void ExplorerPatcher_CloseConsoleWindow(void)
{
    std::lock_guard lock(console_mutex);
    if (console_opened.exchange(false))
        ::FreeConsole();
}


/****************************************************************************************/
} // namespace ExplorerPatcher
