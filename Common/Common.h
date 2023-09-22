#pragma once
#ifndef KhMVV15rjzCiBYtsopGsqAzUxOYyrdOrffzmR7Q9Nxa2FWCUJxl5GHV0jjZNoy9k
#define KhMVV15rjzCiBYtsopGsqAzUxOYyrdOrffzmR7Q9Nxa2FWCUJxl5GHV0jjZNoy9k
/****************************************************************************************/

#if defined _DEBUG && !defined DEBUG
# define DEBUG
#endif
#if defined DEBUG && !defined _DEBUG
# define _DEBUG
#endif
#ifdef _DEBUG
# define _LIBVALINET_DEBUG_HOOKING_IATPATCH
#endif

#include <Windows.h>
#include <sal.h>
#include <io.h>

#ifdef __cplusplus
# include <algorithm>
# include <atomic>
# include <filesystem>
# include <iostream>
# include <memory>
# include <mutex>

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
# include <cwctype>
#else
# include <assert.h>
# include <ctype.h>
# include <errno.h>
# include <inttypes.h>
# include <limits.h>
# include <math.h>
# include <stdarg.h>
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

#include "Common/util.h"

#ifdef __attribute__
# undef __attribute__
#endif
#if defined __INTELLISENSE__ || defined __RESHARPER__ || \
    (!defined __GNUC__ && !defined __clang__ &&          \
     !defined __INTEL_COMPILER && !defined __INTEL_LLVM_COMPILER)
# define __attribute__(x)
#endif

#ifndef __cplusplus
# define StrEq(a, b) (strcmp((a), (b)) == 0)
# define StrIEq(a, b) (_stricmp((a), (b)) == 0)
# define WStrEq(a, b) (wcscmp((a), (b)) == 0)
# define WStrIEq(a, b) (_wcsicmp((a), (b)) == 0)
# define lWStrEq(a, b)  (lstrcmpW((a), (b)) == 0)
# define lWStrIEq(a, b) (lstrcmpiW((a), (b)) == 0)
# define StrNEq(a, b, n) (strncmp((a), (b), (n)) == 0)
# define WStrNEq(a, b, n) (wcsncmp((a), (b), (n)) == 0)
#endif

/****************************************************************************************/
#endif