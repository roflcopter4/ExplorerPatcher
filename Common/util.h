#pragma once
#ifndef PhFT9L0Mlvc1BrZXApY5f1Uc2vzLSILbGOHMVRvb4ZvTz6jhQRFQlkkCTpIUWJB
#define PhFT9L0Mlvc1BrZXApY5f1Uc2vzLSILbGOHMVRvb4ZvTz6jhQRFQlkkCTpIUWJB

#include "Common/Common.h"
EXTERN_C_START
/****************************************************************************************/

size_t ExplorerPatcher_TempString(
    _Out_writes_z_(bufSize) char *__restrict buf,
    _In_                    size_t           bufSize);

void ExplorerPatcher_OpenConsoleWindow(void);
void ExplorerPatcher_CloseConsoleWindow(void);

NTSTATUS ExplorerPatcher_ComputeFileHash(
    _In_                     LPCWSTR filename,
    _Out_writes_z_(hashSize) LPSTR   hash,
    _In_                     SIZE_T  hashSize);

_Check_return_ char *ExplorerPatcher_GetWin32ErrorMessage(DWORD error);

/****************************************************************************************/
EXTERN_C_END
#endif