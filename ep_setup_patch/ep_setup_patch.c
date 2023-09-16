#include <Windows.h>
#include <Shlwapi.h>
#include "ExplorerPatcher/utility.h"

#pragma comment(lib, "Shlwapi.lib")

int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nShowCmd
) 
{
    WCHAR wszPath[MAX_PATH];
    GetModuleFileNameW(GetModuleHandle(NULL), wszPath, MAX_PATH);
    PathRemoveFileSpecW(wszPath);
    wcscat_s(wszPath, MAX_PATH, L"\\" _T(PRODUCT_NAME) L".amd64.dll");
    HMODULE hModule = LoadLibraryExW(wszPath, NULL, LOAD_LIBRARY_AS_DATAFILE);

    CHAR hash[128];
    ZeroMemory(hash, sizeof hash);
    ComputeFileHash2(hModule, wszPath, hash, sizeof hash);
    FreeLibrary(hModule);

    PathRemoveFileSpecW(wszPath);
    wcscat_s(wszPath, MAX_PATH, L"\\" _T(SETUP_UTILITY_NAME));

    HANDLE hFile = CreateFileW(wszPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return 1;

    HANDLE hFileMapping = CreateFileMappingW(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (hFileMapping == NULL) {
        CloseHandle(hFile);
        return 2;
    }

    char *lpFileBase = MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (lpFileBase == NULL) {
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return 3;
    }

    memcpy(lpFileBase + DOSMODE_OFFSET, hash, strlen(hash)+1);
    UnmapViewOfFile(lpFileBase);
    CloseHandle(hFileMapping);
    CloseHandle(hFile);

    if (__argc > 1) {
        SHELLEXECUTEINFO ShExecInfo = {
            .cbSize       = sizeof(SHELLEXECUTEINFO),
            .fMask        = SEE_MASK_NOCLOSEPROCESS,
            .hwnd         = NULL,
            .lpVerb       = L"runas",
            .lpFile       = wszPath,
            .lpParameters = NULL,
            .lpDirectory  = NULL,
            .nShow        = SW_SHOW,
            .hInstApp     = NULL,
        };

        if (ShellExecuteExW(&ShExecInfo) && ShExecInfo.hProcess) {
            WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
            DWORD dwExitCode = 0;
            GetExitCodeProcess(ShExecInfo.hProcess, &dwExitCode);
            CloseHandle(ShExecInfo.hProcess);
            return (int)dwExitCode;
        }
    }

    return 0;
}