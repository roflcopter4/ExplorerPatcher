#pragma once
#ifndef PvNJIgfbhLnrWGNwoQrF0Kk9HZDwzqbP
#define PvNJIgfbhLnrWGNwoQrF0Kk9HZDwzqbP
/****************************************************************************************/

#include <Windows.h>
#include <sal.h>

#ifdef __cplusplus
# include <cassert>
# include <cctype>
# include <cerrno>
# include <cinttypes>
# include <climits>
# include <cstdarg>
# include <cstddef>
# include <cstdint>
# include <cstdio>
# include <cstdlib>
# include <cstring>
# include <ctime>
# include <cwchar>
#else
# include <assert.h>
# include <ctype.h>
# include <errno.h>
# include <inttypes.h>
# include <limits.h>
# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>
# include <wchar.h>
# include <wctype.h>
#endif

#ifdef __attribute__
# undef __attribute__
#endif
#if defined __INTELLISENSE__ || defined __RESHARPER__ || \
    (!defined __GNUC__ && !defined __clang__ && !defined __INTEL_COMPILER && !defined __INTEL_LLVM_COMPILER)
# define __attribute__(x)
#endif

#ifndef __cplusplus
# define StrEq(a, b)  (strcmp((a), (b)) == 0)
# define WStrEq(a, b) (wcscmp((a), (b)) == 0)
# define StrIEq(a, b)  (_stricmp((a), (b)) == 0)
# define WStrIEq(a, b) (_wcsicmp((a), (b)) == 0)
# define lWStrEq(a, b)  (lstrcmpW((a), (b)) == 0)
# define lWStrIEq(a, b) (lstrcmpiW((a), (b)) == 0)
#endif

EXTERN_C_START

size_t ExplorerPatcher_TempString(
    _Out_writes_z_(bufSize) char *__restrict buf,
    _In_                    size_t           bufSize);

void ExplorerPatcher_OpenConsoleWindow(void);
void ExplorerPatcher_CloseConsoleWindow(void);

NTSTATUS ExplorerPatcher_ComputeFileHash(
    _In_                     LPCWSTR filename,
    _Out_writes_z_(hashSize) LPSTR   hash,
    _In_                     SIZE_T  hashSize);

char *ExplorerPatcher_GetWin32ErrorMessage(DWORD error);

EXTERN_C_END

/****************************************************************************************/
#endif