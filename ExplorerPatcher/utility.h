#pragma once
#ifndef _H_UTILITY_H_
#define _H_UTILITY_H_

#if __has_include("ep_private.h")
//#define USE_PRIVATE_INTERFACES
#endif
#define _CRT_RAND_S 1
#ifdef _DEBUG
# define DEBUG 1
#endif

#include <Windows.h>
#include <Shlwapi.h>
#include <Shlobj_core.h>
#include <Shobjidl.h>
#include <accctrl.h>
#include <aclapi.h>
#include <activscp.h>
#include <netlistmgr.h>
#include <psapi.h>
#include <restartmanager.h>
#include <sddl.h>
#include <tchar.h>
#include <windows.data.xml.dom.h>

#define _LIBVALINET_INCLUDE_UNIVERSAL
#include <valinet/universal/toast/toast.h>

#include "osutility.h"
#include "queryversion.h"
#include "def.h"

#pragma comment(lib, "Rstrtmgr.lib")
#pragma comment(lib, "Psapi.lib")


#include "Common/Common.h"

#define WM_MSG_GUI_SECTION WM_USER + 1
#define WM_MSG_GUI_SECTION_GET 1

DEFINE_GUID(CLSID_ImmersiveShell,
    0xc2f03a33,
    0x21f5, 0x47fa, 0xb4, 0xbb,
    0x15, 0x63, 0x62, 0xa2, 0xf2, 0x39
);

DEFINE_GUID(IID_OpenControlPanel,
    0xD11AD862,
    0x66De, 0x4DF4, 0xBf, 0x6C,
    0x1F, 0x56, 0x21, 0x99, 0x6A, 0xF1
);

DEFINE_GUID(CLSID_VBScript,
    0xB54F3741, 
    0x5B07, 0x11CF, 0xA4, 0xB0, 
    0x00, 0xAA, 0x00, 0x4A, 0x55, 0xE8
);

DEFINE_GUID(CLSID_NetworkListManager,
    0xDCB00C01, 0x570F, 0x4A9B, 0x8D, 0x69, 0x19, 0x9F, 0xDB, 0xA5, 0x72, 0x3B);

DEFINE_GUID(IID_NetworkListManager,
    0xDCB00000, 0x570F, 0x4A9B, 0x8D, 0x69, 0x19, 0x9F, 0xDB, 0xA5, 0x72, 0x3B);

typedef struct StuckRectsData
{
    int pvData[6];
    RECT rc;
    POINT pt;
} StuckRectsData;

extern HRESULT FindDesktopFolderView(REFIID riid, void** ppv);
extern HRESULT GetDesktopAutomationObject(REFIID riid, void** ppv);
extern HRESULT ShellExecuteFromExplorer(
    PCWSTR pszFile,
    PCWSTR pszParameters,
    PCWSTR pszDirectory,
    PCWSTR pszOperation,
    int nShowCmd
);

#pragma region "Weird stuff"
struct IActivationFactoryAA;
typedef struct IActivationFactoryAA IActivationFactoryAA;
extern const IActivationFactoryAA XamlExtensionsFactory;
#pragma endregion

extern void ToggleTaskbarAutohide(void);
extern int  FileExistsW(wchar_t const *file);
extern void printf_guid(GUID guid);

extern LRESULT CALLBACK BalloonWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

__declspec(dllexport) extern int  CALLBACK ZZTestBalloon(HWND hWnd, HINSTANCE hInstance, LPSTR lpszCmdLine, int nCmdShow);
__declspec(dllexport) extern void CALLBACK ZZTestToast(HWND hWnd, HINSTANCE hInstance, LPSTR lpszCmdLine, int nCmdShow);
__declspec(dllexport) extern void CALLBACK ZZLaunchExplorer(HWND hWnd, HINSTANCE hInstance, PTCHAR lpszCmdLine, int nCmdShow);
__declspec(dllexport) extern void CALLBACK ZZLaunchExplorerDelayed(HWND hWnd, HINSTANCE hInstance, LPSTR lpszCmdLine, int nCmdShow);
__declspec(dllexport) extern void CALLBACK ZZRestartExplorer(HWND hWnd, HINSTANCE hInstance, LPSTR lpszCmdLine, int nCmdShow);

#ifndef MIN
# define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif
#ifndef MAX
# define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif

typedef LSTATUS (*SHRegGetValueFromHKCUHKLMFunc_t)(
    _In_ PCWSTR      pwszKey,
    _In_opt_ PCWSTR  pwszValue,
    _In_ SRRF        srrfFlags,
    _Out_opt_ DWORD *pdwType,
    _Out_writes_bytes_to_opt_(*pcbData, *pcbData)  void  *pvData,
    _Inout_opt_ _When_(pvData != 0, _Pre_notnull_) DWORD *pcbData
);
extern SHRegGetValueFromHKCUHKLMFunc_t SHRegGetValueFromHKCUHKLMFunc;


extern LSTATUS SHRegGetValueFromHKCUHKLMWithOpt(
    PCWSTR  pwszKey,
    PCWSTR  pwszValue,
    REGSAM  samDesired,
    void   *pvData,
    DWORD  *pcbData);

static HWND(WINAPI* CreateWindowInBand)(
    _In_ DWORD dwExStyle,
    _In_opt_ ATOM atom,
    _In_opt_ LPCWSTR lpWindowName,
    _In_ DWORD dwStyle,
    _In_ int X,
    _In_ int Y,
    _In_ int nWidth,
    _In_ int nHeight,
    _In_opt_ HWND hWndParent,
    _In_opt_ HMENU hMenu,
    _In_opt_ HINSTANCE hInstance,
    _In_opt_ LPVOID lpParam,
    DWORD band
    );

extern BOOL  (WINAPI *GetWindowBand)(HWND hWnd, PDWORD pdwBand);
extern BOOL  (WINAPI *SetWindowBand)(HWND hWnd, HWND hwndInsertAfter, DWORD dwBand);
extern INT64 (*SetWindowCompositionAttribute)(HWND, void *);

__attribute__((const)) static __forceinline
BOOL AppsShouldUseDarkMode(void)
{
    return TRUE;
}

extern void     * ReadFromFile(wchar_t const* wszFileName, DWORD * dwSize);
extern DWORD      ComputeFileHash(LPCWSTR filename, LPSTR hash, DWORD dwHash);
extern int        ComputeFileHash2(HMODULE hModule, LPCWSTR filename, LPSTR hash, DWORD dwHash);
extern void       LaunchPropertiesGUI(HMODULE hModule);
extern BOOL       SystemShutdown(BOOL reboot);
extern LSTATUS    RegisterDWMService(DWORD dwDesiredState, DWORD dwOverride);
extern char     * StrReplaceAllA(const char * s, const char * oldW, const char * newW, DWORD * dwNewSize);
extern WCHAR    * StrReplaceAllW(const WCHAR * s, const WCHAR * oldW, const WCHAR * newW, DWORD * dwNewSize);
extern BOOL       IsHighContrast(void);
extern WCHAR    * ep_generate_random_wide_string(WCHAR * str, size_t size);
extern ULONGLONG  milliseconds_now(void);
extern HRESULT    InputBox(BOOL bPassword, HWND hWnd, LPCWSTR wszPrompt, LPCWSTR wszTitle, LPCWSTR wszDefault, LPWSTR wszAnswer, DWORD cbAnswer, BOOL* bCancelled);

// shuts down the explorer and is ready for explorer restart
extern void BeginExplorerRestart(void);
// restarts the explorer
extern void FinishExplorerRestart(void);

extern RM_UNIQUE_PROCESS GetExplorerApplication(void);
extern BOOL IsAppRunningAsAdminMode(void);
extern BOOL IsDesktopWindowAlreadyPresent(void);
extern BOOL ExitExplorer(void);
extern void StartExplorerWithDelay(int delay, HANDLE userToken);
extern BOOL StartExplorer(void);
extern BOOL IncrementDLLReferenceCount(HINSTANCE hinst);
extern BOOL WINAPI PatchContextMenuOfNewMicrosoftIME(BOOL *bFound);

// https://stackoverflow.com/questions/5689904/gracefully-exit-explorer-programmatically
inline BOOL ExitExplorer()
{
    HWND hWndTray = FindWindowW(L"Shell_TrayWnd", NULL);
    return PostMessageW(hWndTray, 0x5B4, 0, 0);
}

inline void StartExplorerWithDelay(int delay, HANDLE userToken)
{
    WCHAR wszPath[MAX_PATH];
    ZeroMemory(wszPath, MAX_PATH * sizeof(WCHAR));
    GetWindowsDirectoryW(wszPath, MAX_PATH);
    wcscat_s(wszPath, MAX_PATH, L"\\explorer.exe");
    Sleep(delay);
    if (userToken != INVALID_HANDLE_VALUE)
    {
        HANDLE primaryUserToken = INVALID_HANDLE_VALUE;
        if (ImpersonateLoggedOnUser(userToken))
        {
            DuplicateTokenEx(userToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &primaryUserToken);
            RevertToSelf();
        }
        if (primaryUserToken != INVALID_HANDLE_VALUE)
        {
            PROCESS_INFORMATION processInfo;
            ZeroMemory(&processInfo, sizeof(processInfo));
            STARTUPINFOW startupInfo;
            ZeroMemory(&startupInfo, sizeof(startupInfo));
            startupInfo.cb = sizeof(startupInfo);
            BOOL processCreated = CreateProcessWithTokenW(
                primaryUserToken, LOGON_WITH_PROFILE, wszPath, NULL, 0, NULL, NULL, &startupInfo, &processInfo) != 0;
            CloseHandle(primaryUserToken);
            if (processInfo.hProcess != INVALID_HANDLE_VALUE)
            {
                CloseHandle(processInfo.hProcess);
            }
            if (processInfo.hThread != INVALID_HANDLE_VALUE)
            {
                CloseHandle(processInfo.hThread);
            }
            if (processCreated)
            {
                return;
            }
        }
    }
    ShellExecuteW(
        NULL,
        L"open",
        wszPath,
        NULL,
        NULL,
        SW_SHOWNORMAL
    );
}

inline void StartExplorer()
{

    /*PROCESSENTRY32 pe32 = {0};
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS,
        0
    );
    if (Process32First(hSnapshot, &pe32) == TRUE)
    {
        do
        {
            if (!wcscmp(pe32.szExeFile, TEXT("explorer.exe")))
            {
                HANDLE hSihost = OpenProcess(
                    PROCESS_TERMINATE,
                    FALSE,
                    pe32.th32ProcessID
                );
                TerminateProcess(hSihost, 1);
                CloseHandle(hSihost);
            }
        } while (Process32Next(hSnapshot, &pe32) == TRUE);
    }
    CloseHandle(hSnapshot);
    */
    wchar_t wszPath[MAX_PATH];
    ZeroMemory(
        wszPath,
        (MAX_PATH) * sizeof(wchar_t)
    );
    GetWindowsDirectoryW(
        wszPath,
        MAX_PATH
    );
    wcscat_s(
        wszPath,
        MAX_PATH,
        L"\\explorer.exe"
    );
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    if (CreateProcessW(
        NULL,
        wszPath,
        NULL,
        NULL,
        TRUE,
        CREATE_UNICODE_ENVIRONMENT,
        NULL,
        NULL,
        &si,
        &pi
    ))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

inline BOOL IncrementDLLReferenceCount(HINSTANCE hinst)
{
    HMODULE hMod;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        hinst,
        &hMod);
}

inline BOOL WINAPI PatchContextMenuOfNewMicrosoftIME(BOOL* bFound)
{
    // huge thanks to @Simplestas: https://github.com/valinet/ExplorerPatcher/issues/598
    if (bFound) *bFound = FALSE;
    DWORD patch_from, patch_to;
    if (IsWindows11Version22H2OrHigher())
    {
        // cmp byte ptr [rbp+40h+arg_0], r13b
        patch_from = 0x506D3844;
        patch_to = 0x546D3844;
    }
    else
    {
        // cmp byte ptr [rbp+50h], r12b
        patch_from = 0x50653844;
        patch_to = 0x54653844;
    }
    HMODULE hInputSwitch = NULL;
    if (!GetModuleHandleExW(0, L"InputSwitch.dll", &hInputSwitch))
    {
        return FALSE;
    }
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hInputSwitch;
    PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)dosHeader + dosHeader->e_lfanew);
    PIMAGE_SECTION_HEADER pSectionHeader = (PIMAGE_SECTION_HEADER)(pNTHeader + 1);
    char* mod = 0;
    int i;
    for (i = 0; i < pNTHeader->FileHeader.NumberOfSections; i++)
    {
        //if (strcmp((char*)pSectionHeader[i].Name, ".text") == 0)
        if ((pSectionHeader[i].Characteristics & IMAGE_SCN_CNT_CODE) && pSectionHeader[i].SizeOfRawData)
        {
            mod = (char*)dosHeader + pSectionHeader[i].VirtualAddress;
            break;
        }
    }
    if (!mod)
    {
        return FALSE;
    }
    for (size_t off = 0; off < pSectionHeader[i].Misc.VirtualSize - sizeof(DWORD); ++off)
    {
        DWORD* ptr = (DWORD*)(mod + off);
        if (*ptr == patch_from)
        {
            if (bFound) *bFound = TRUE;
            DWORD prot;
            if (VirtualProtect(ptr, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &prot))
            {
                *ptr = patch_to;
                VirtualProtect(ptr, sizeof(DWORD), prot, &prot);
            }
            break;
        }
    }
    return TRUE;
}

extern UINT PleaseWaitTimeout;
extern HHOOK PleaseWaitHook;
extern HWND  PleaseWaitHWND;
extern void *PleaseWaitCallbackData;
extern BOOL (*PleaseWaitCallbackFunc)(void *data);

extern BOOL             PleaseWait_UpdateTimeout(int timeout);
extern void CALLBACK    PleaseWait_TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
extern LRESULT CALLBACK PleaseWait_HookProc(int code, WPARAM wParam, LPARAM lParam);
extern BOOL             DownloadAndInstallWebView2Runtime(void);
extern BOOL             DownloadFile(LPCWSTR wszURL, DWORD dwSize, LPCWSTR wszPath);
extern BOOL             IsConnectedToInternet(void);

#define SCRATCH_QCM_FIRST 1
#define SCRATCH_QCM_LAST  0x7FFF

#define SPOP_OPENMENU            1
#define SPOP_INSERTMENU_ALL      0b1111110000
#define SPOP_INSERTMENU_OPEN     0b0000010000
#define SPOP_INSERTMENU_NEXTPIC  0b0000100000
#define SPOP_INSERTMENU_LIKE     0b0001000000
#define SPOP_INSERTMENU_DISLIKE  0b0010000000
#define SPOP_INSERTMENU_INFOTIP1 0b0100000000
#define SPOP_INSERTMENU_INFOTIP2 0b1000000000
#define SPOP_CLICKMENU_FIRST     40000
#define SPOP_CLICKMENU_OPEN      40000
#define SPOP_CLICKMENU_NEXTPIC   40001
#define SPOP_CLICKMENU_LIKE      40002
#define SPOP_CLICKMENU_DISLIKE   40003
#define SPOP_CLICKMENU_LAST      40003

extern BOOL DoesOSBuildSupportSpotlight(void);
extern BOOL IsSpotlightEnabled(void);
extern void SpotlightHelper(DWORD dwOp, HWND hWnd, HMENU hMenu, LPPOINT pPt);

typedef struct MonitorOverrideData
{
    DWORD cbIndex;
    DWORD dwIndex;
    HMONITOR hMonitor;
} MonitorOverrideData;

BOOL ExtractMonitorByIndex(HMONITOR hMonitor, HDC hDC, LPRECT lpRect, MonitorOverrideData* mod);

inline BOOL MaskCompare(PVOID pBuffer, LPCSTR lpPattern, LPCSTR lpMask)
{
    for (PBYTE value = pBuffer; *lpMask; ++lpPattern, ++lpMask, ++value)
    {
        if (*lpMask == 'x' && *(LPCBYTE)lpPattern != *value)
            return FALSE;
    }

    return TRUE;
}

inline PVOID FindPattern(PVOID pBase, SIZE_T dwSize, LPCSTR lpPattern, LPCSTR lpMask)
{
    dwSize -= strlen(lpMask);

    for (SIZE_T index = 0; index < dwSize; ++index)
    {
        PBYTE pAddress = (PBYTE)pBase + index;

        if (MaskCompare(pAddress, lpPattern, lpMask))
            return pAddress;
    }

    return NULL;
}
#endif
