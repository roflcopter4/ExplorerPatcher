#include "Common/Common.h"

#include <system_error>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

static constexpr size_t error_buffer_size = 2048;

extern "C" {

#if 0
/*
 * UNSPEAKABLY EVIL MACRO EVERYONE PANIC AND SCREAM
 */
#define GET_ERROR_BUFFER(WHAT, SUFFIX)                                   \
      do {                                                               \
            auto const code = std::error_code{static_cast<int>(errval),  \
                                              std::system_category()};   \
            swprintf_s(buf, std::size(buf),                              \
                       L"\"" WHAT L"\" failed with error %lu:\n"         \
                       L"\u2002\u2002\u2002\u2002%hs" SUFFIX,            \
                       msg, errval, code.message().c_str());             \
      } while (0)

void
win32_warning_box_message(_In_z_ wchar_t const *msg)
{
      ::MessageBoxW(nullptr, msg, L"Non-Fatal Error", MB_OK);
}

void
win32_warning_box_explicit(_In_z_ wchar_t const *msg, _In_ DWORD const errval)
{
      wchar_t buf[error_buffer_size];
      GET_ERROR_BUFFER(L"%s", L"\nAttempting to continue...");
      win32_warning_box_message(buf);
}


__declspec(noreturn) void
win32_error_exit_message(_In_z_ wchar_t const *msg)
{
      ::MessageBoxW(nullptr, msg, L"Error", MB_OK);
      ::exit(EXIT_FAILURE);
}

__declspec(noreturn) void
win32_error_exit_explicit(_In_z_ wchar_t const *msg, _In_ DWORD const errval)
{
      wchar_t buf[error_buffer_size];
      GET_ERROR_BUFFER(L"%s", L"\nCannot continue. Press OK to exit.");
      win32_error_exit_message(buf);
}
#endif

char *
ExplorerPatcher_GetWin32ErrorMessage(DWORD error)
{
    auto const code = std::error_code{static_cast<int>(error), std::system_category()};
    return _strdup(code.message().c_str());
}

} // extern "C"
