#include "Common/Common.h"

namespace ExplorerPatcher {
/****************************************************************************************/


static std::atomic_bool console_opened;
static std::mutex       console_mutex;

static void OpenConsoleWindow()
{
    FreeConsole();
    if (!AllocConsole()) {
        DWORD err = GetLastError();
        WCHAR buf[512];
        swprintf_s(buf, std::size(buf),
                   L"Failed to allocate a console (error %lX). "
                   L"If this happens your computer is probably on fire.",
                   err);
        MessageBoxW(nullptr, buf, L"Fatal Error", MB_OK | MB_ICONERROR);
        exit(1);
    }

    FILE *conout = nullptr;
    _wfreopen_s(&conout, L"CONOUT$", L"w", stdout);
    conout = nullptr;
    _wfreopen_s(&conout, L"CONOUT$", L"w", stderr);

    // Ensure we set both streams to be byte-oriented.
    fputs("Opened standard output on newly created console.\n", stdout);
    fflush(stdout);
    fputs("Opened standard error on newly created console.\n", stderr);
    fflush(stderr);
}


extern "C" void
ExplorerPatcher_OpenConsoleWindow(void)
{
    std::lock_guard lock(console_mutex);
    if (!console_opened.exchange(true, std::memory_order::relaxed))
        OpenConsoleWindow();
}

extern "C" void
ExplorerPatcher_CloseConsoleWindow(void)
{
    std::lock_guard lock(console_mutex);
    if (console_opened.exchange(false, std::memory_order::relaxed))
        ::FreeConsole();
}


/****************************************************************************************/
} // namespace ExplorerPatcher
