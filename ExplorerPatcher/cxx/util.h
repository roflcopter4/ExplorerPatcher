#pragma once
#ifndef QiCsc9e7CNZHTO58zCW5yj3QbaUwxx
#define QiCsc9e7CNZHTO58zCW5yj3QbaUwxx

/****************************************************************************************/

#include <Windows.h>
#include <sal.h>

#if 0
#include <errno.h>

EXTERN_C_START

void win32_warning_box_explicit(_In_z_ wchar_t const *msg, _In_ DWORD errval);
void win32_warning_box_message (_In_z_ wchar_t const *msg);

__declspec(noreturn) void win32_error_exit_explicit(_In_z_ wchar_t const *msg, _In_ DWORD errval);
__declspec(noreturn) void win32_error_exit_message (_In_z_ wchar_t const *msg);

__declspec(noreturn) __forceinline void win32_error_exit      (wchar_t const *msg) { win32_error_exit_explicit(msg, GetLastError());    }
__declspec(noreturn) __forceinline void win32_error_exit_errno(wchar_t const *msg) { win32_error_exit_explicit(msg, errno);             }

EXTERN_C_END
#endif

/****************************************************************************************/
#endif
