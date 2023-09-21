#include "utility.h"

#include <stdint.h>
#include <Wininet.h>
#pragma comment(lib, "Wininet.lib")

#define my_static_assert(cond) _Static_assert((cond), #cond)

BOOL(WINAPI *GetWindowBand)(HWND hWnd, PDWORD pdwBand);
BOOL(WINAPI *SetWindowBand)(HWND hWnd, HWND hwndInsertAfter, DWORD dwBand);
INT64 (*SetWindowCompositionAttribute)(HWND, void *);
SHRegGetValueFromHKCUHKLMFunc_t SHRegGetValueFromHKCUHKLMFunc;
RTL_OSVERSIONINFOW global_rovi;
DWORD32            global_ubr;

extern _ACRTIMP errno_t __cdecl rand_s(_Out_ unsigned int* _RandomValue);


#pragma region "Weird stuff"
struct IActivationFactoryAA {
    CONST_VTBL IActivationFactoryVtbl *lpVtbl1;
    CONST_VTBL IActivationFactoryVtbl *lpVtbl2;
    CONST_VTBL IActivationFactoryVtbl *lpVtbl3;
};

typedef HRESULT (STDMETHODCALLTYPE *IAtblCB_QueryInterface_t)     (IActivationFactory *, IID const *, void **);
typedef ULONG   (STDMETHODCALLTYPE *IAtblCB_Base_t)               (IActivationFactory *);
typedef HRESULT (STDMETHODCALLTYPE *IAtblCB_GetIids_t)            (IActivationFactory *, ULONG *, IID **);
typedef HRESULT (STDMETHODCALLTYPE *IAtblCB_GetRuntimeClassName_t)(IActivationFactory *, HSTRING *);
typedef HRESULT (STDMETHODCALLTYPE *IAtblCB_GetTrustLevel_t)      (IActivationFactory *, TrustLevel *);
typedef HRESULT (STDMETHODCALLTYPE *IAtblCB_ActivateInstance_t)   (IActivationFactory *, IInspectable **);

/*--------------------------------------------------------------------------------------*/

DEFINE_GUID(uuid_xamlActivationFactory,
            0x34A95314, 0xCA5C, 0x5FAD,
            0xAE, 0x7C, 0x1A, 0x90, 0x18, 0x11, 0x66, 0xC1);

static HRESULT STDMETHODCALLTYPE
vtable_impl_AddRef(IActivationFactory *_this)
{
    return 1;
}

static HRESULT STDMETHODCALLTYPE
vtable_impl_Release(IActivationFactory *_this)
{
    return 1;
}

static HRESULT STDMETHODCALLTYPE
impl_IActivationFactory_GetIids(IActivationFactory *_this, ULONG *a2, IID **a3)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
impl_IActivationFactory_GetRuntimeClassName(IActivationFactory *_this, HSTRING *hString)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
impl_IActivationFactory_GetTrustLevel(IActivationFactory *_this, TrustLevel *trust)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
vtable_impl_QueryInterface_1(IActivationFactory *_this, IID const *a2, void **a3)
{
    void *v4 = _this; // rcx

    if (!IsEqualIID(a2, &uuid_xamlActivationFactory))
        return E_NOTIMPL;

    *a3 = v4;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
vtable_impl_QueryInterface_2(IActivationFactory *_this, IID const *a2, void **a3)
{
    void *v4 = (void *)((uintptr_t)_this - sizeof(uintptr_t)); // rcx

    if (!IsEqualIID(a2, &uuid_xamlActivationFactory))
        return E_NOTIMPL;

    *a3 = v4;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
vtable_impl_QueryInterface_3(IActivationFactory *_this, IID const *a2, void **a3)
{
    void *v4 = (void *)((uintptr_t)_this - 2 * sizeof(uintptr_t)); // rcx

    if (!IsEqualIID(a2, &uuid_xamlActivationFactory))
        return E_NOTIMPL;

    *a3 = v4;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
vtable_impl_ActivateInstance_1(IActivationFactory *_this, IInspectable **a2)
{
    // rax
    void *v2 = (void *)((uintptr_t)_this + 8);
    if (!_this)
        v2 = NULL;;
    *a2 = v2;
    return 0i64;
}

static HRESULT STDMETHODCALLTYPE
vtable_impl_ActivateInstance_2(IActivationFactory *_this, IInspectable **a2)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
vtable_impl_ActivateInstance_3(IActivationFactory *_this, IInspectable **a2, INT64 a3, BYTE *a4)
{
    *a4 = 0;
    return 0;
}

/*--------------------------------------------------------------------------------------*/

static const IActivationFactoryVtbl IActivationFactoryVtbl1 = {
    .QueryInterface      = &vtable_impl_QueryInterface_1,
    .AddRef              = &vtable_impl_AddRef,
    .Release             = &vtable_impl_Release,
    .GetIids             = &impl_IActivationFactory_GetIids,
    .GetRuntimeClassName = &impl_IActivationFactory_GetRuntimeClassName,
    .GetTrustLevel       = &impl_IActivationFactory_GetTrustLevel,
    .ActivateInstance    = &vtable_impl_ActivateInstance_1,
};

static const IActivationFactoryVtbl IActivationFactoryVtbl2 = {
    .QueryInterface      = &vtable_impl_QueryInterface_2,
    .AddRef              = &vtable_impl_AddRef,
    .Release             = &vtable_impl_Release,
    .GetIids             = &impl_IActivationFactory_GetIids,
    .GetRuntimeClassName = &impl_IActivationFactory_GetRuntimeClassName,
    .GetTrustLevel       = &impl_IActivationFactory_GetTrustLevel,
    .ActivateInstance    = &vtable_impl_ActivateInstance_2,
};

static const IActivationFactoryVtbl IActivationFactoryVtbl3 = {
    .QueryInterface      = &vtable_impl_QueryInterface_3,
    .AddRef              = &vtable_impl_AddRef,
    .Release             = &vtable_impl_Release,
    .GetIids             = &impl_IActivationFactory_GetIids,
    .GetRuntimeClassName = &impl_IActivationFactory_GetRuntimeClassName,
    .GetTrustLevel       = &impl_IActivationFactory_GetTrustLevel,
    .ActivateInstance    = (IAtblCB_ActivateInstance_t)&vtable_impl_ActivateInstance_3,
};

const IActivationFactoryAA XamlExtensionsFactory = {
    .lpVtbl1 = (IActivationFactoryVtbl *)&IActivationFactoryVtbl1,
    .lpVtbl2 = (IActivationFactoryVtbl *)&IActivationFactoryVtbl2,
    .lpVtbl3 = (IActivationFactoryVtbl *)&IActivationFactoryVtbl3
};
#pragma endregion


int FileExistsW(wchar_t const* file)
{
    DWORD const dwAttrib = GetFileAttributesW(file);
    return dwAttrib != INVALID_FILE_ATTRIBUTES;
}

void printf_guid(GUID guid) 
{
    // https://stackoverflow.com/questions/1672677/print-a-guid-variable

    printf("Guid = {%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}\n",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

LRESULT CALLBACK BalloonWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
    if (msg == WM_CREATE) {
        LPCREATESTRUCTW lpCs = (void *)lParam;

        NOTIFYICONDATAW ni = {
            .cbSize      = sizeof(ni),
            .hWnd        = hWnd,
            .uID         = 1,
            .uFlags      = NIF_INFO,
            .dwInfoFlags = NIIF_INFO,
            .uTimeout    = 2000,
        };
        wcscpy_s(ni.szInfo, _countof(ni.szInfo), lpCs->lpCreateParams),
        wcscpy_s(ni.szInfoTitle, _countof(ni.szInfoTitle), L"ExplorerPatcher");

        Shell_NotifyIconW(NIM_ADD, &ni);
        free(lpCs->lpCreateParams);
        exit(0);
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

__declspec(dllexport) int CALLBACK
ZZTestBalloon(HWND hWnd, HINSTANCE hInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    size_t   size         = strlen(lpszCmdLine) + 1;
    wchar_t *lpwszCmdLine = malloc(size * sizeof *lpwszCmdLine);
    if (!lpwszCmdLine)
        exit(1);
    int ret = MultiByteToWideChar(CP_UTF8, 0, lpszCmdLine, (int)size, lpwszCmdLine, (int)size);
    if (ret != (int)size)
        exit(1);

    MSG        msg;
    WNDCLASSEXW wc = {
        .cbSize        = sizeof(WNDCLASSEX),
        .style         = 0,
        .lpfnWndProc   = BalloonWndProc,
        .cbClsExtra    = 0,
        .cbWndExtra    = 0,
        .hInstance     = hInstance,
        .hIcon         = LoadIconW(NULL, IDI_APPLICATION),
        .hCursor       = LoadCursorW(NULL, IDC_ARROW),
        .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
        .lpszMenuName  = NULL,
        .lpszClassName = L"ExplorerPatcherBalloon",
        .hIconSm       = LoadIconW(NULL, IDI_APPLICATION),
    };

    if (!RegisterClassExW(&wc))
        return 0;

    HWND hwnd = CreateWindowExW(0, L"ExplorerPatcherBalloon", L"",
                                0, 0, 0, 0, 0, HWND_MESSAGE, NULL,
                                hInstance, lpwszCmdLine);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 1;
}

static wchar_t const TestToastXML[] =
L"<toast scenario=\"reminder\" "
L"activationType=\"protocol\" launch=\"https://github.com/valinet/ExplorerPatcher\" duration=\"%ls\">\r\n"
L"	<visual>\r\n"
L"		<binding template=\"ToastGeneric\">\r\n"
L"			<text><![CDATA[%ls]]></text>\r\n"
L"			<text placement=\"attribution\"><![CDATA[ExplorerPatcher]]></text>\r\n"
L"		</binding>\r\n"
L"	</visual>\r\n"
L"	<audio src=\"ms-winsoundevent:Notification.Default\" loop=\"false\" silent=\"false\"/>\r\n"
L"</toast>\r\n";

__declspec(dllexport) void CALLBACK
ZZTestToast(HWND hWnd, HINSTANCE hInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    WCHAR* lpwszCmdLine = calloc((strlen(lpszCmdLine) + 1), sizeof(WCHAR));
    if (!lpwszCmdLine) exit(0);
    size_t numChConv = 0;
    mbstowcs_s(&numChConv, lpwszCmdLine, strlen(lpszCmdLine) + 1, lpszCmdLine, strlen(lpszCmdLine) + 1);

    size_t const buflen = _countof(TestToastXML) + strlen(lpszCmdLine) + 10;
    WCHAR* buffer = malloc(buflen * sizeof(WCHAR));

    if (buffer)
    {
        swprintf_s(buffer, buflen, TestToastXML, L"short", lpwszCmdLine);
        HRESULT hr = S_OK;
        __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocument* inputXml = NULL;
        hr = String2IXMLDocument(
            buffer,
            wcslen(buffer),
            &inputXml,
#ifdef DEBUG
            stdout
#else
            NULL
#endif
        );
        hr = ShowToastMessage(
            inputXml,
            APPID,
            sizeof(APPID) / sizeof(WCHAR) - 1,
#ifdef DEBUG
            stdout
#else
            NULL
#endif
        );
        free(buffer);
    }
    free(lpwszCmdLine);
}

__declspec(dllexport) void CALLBACK
ZZLaunchExplorer(HWND hWnd, HINSTANCE hInstance, PWCHAR lpszCmdLine, int nCmdShow)
{
    WCHAR wszExplorerPath[MAX_PATH + 1];

    Sleep(100);
    GetWindowsDirectoryW(wszExplorerPath, MAX_PATH + 1);
    wcscat_s(wszExplorerPath, MAX_PATH + 1, L"\\explorer.exe");

    PROCESS_INFORMATION pi;
    STARTUPINFOW        si = {.cb = sizeof si};
    CreateProcessW(NULL, wszExplorerPath, NULL, NULL, TRUE,
                   CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
    FreeConsole();
    TerminateProcess(OpenProcess(PROCESS_TERMINATE, FALSE, GetCurrentProcessId()), 0);
}

__declspec(dllexport) void CALLBACK
ZZLaunchExplorerDelayed(HWND hWnd, HINSTANCE hInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    Sleep(2000);
    ZZLaunchExplorer(hWnd, hInstance, lpszCmdLine, nCmdShow);
}

__declspec(dllexport) void CALLBACK
ZZRestartExplorer(HWND hWnd, HINSTANCE hInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    BeginExplorerRestart();
    FinishExplorerRestart();
}

LSTATUS SHRegGetValueFromHKCUHKLMWithOpt(
    PCWSTR  pwszKey,
    PCWSTR  pwszValue,
    REGSAM  samDesired,
    void   *pvData,
    DWORD  *pcbData)
{
    LSTATUS lRes = ERROR_FILE_NOT_FOUND;
    HKEY    hKey = NULL;

    RegOpenKeyExW(HKEY_CURRENT_USER, pwszKey, 0, samDesired, &hKey);
    if (hKey == NULL || hKey == INVALID_HANDLE_VALUE)
        hKey = NULL;
    if (hKey) {
        lRes = RegQueryValueExW(hKey, pwszValue, 0, NULL, pvData, pcbData);
        RegCloseKey(hKey);
        if (lRes == ERROR_SUCCESS || lRes == ERROR_MORE_DATA)
            return lRes;
    }

    RegOpenKeyExW(HKEY_LOCAL_MACHINE, pwszKey, 0, samDesired, &hKey);
    if (hKey == NULL || hKey == INVALID_HANDLE_VALUE)
        hKey = NULL;
    if (hKey) {
        lRes = RegQueryValueExW(hKey, pwszValue, 0, NULL, pvData, pcbData);
        RegCloseKey(hKey);
        if (lRes == ERROR_SUCCESS || lRes == ERROR_MORE_DATA)
            return lRes;
    }

    return lRes;
}

void *ReadFromFile(wchar_t const *wszFileName, DWORD *dwSize)
{
    void  *ok = NULL;
    HANDLE hImage = CreateFileW(wszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hImage) {
        LARGE_INTEGER dwFileSize;
        GetFileSizeEx(hImage, &dwFileSize);
        if (dwFileSize.QuadPart > UINT32_MAX)
            return NULL;
        if (dwFileSize.LowPart) {
            void *pImage = malloc(dwFileSize.LowPart);
            if (pImage) {
                DWORD dwNumberOfBytesRead = 0;
                ReadFile(hImage, pImage, dwFileSize.LowPart, &dwNumberOfBytesRead, NULL);
                if (dwFileSize.LowPart == dwNumberOfBytesRead) {
                    ok      = pImage;
                    *dwSize = dwNumberOfBytesRead;
                }
            }
        }
        CloseHandle(hImage);
    }
    return ok;
}

DWORD ComputeFileHash(LPCWSTR filename, LPSTR hash, DWORD dwHash)
{
    static WCHAR const rgbDigits[] = L"0123456789abcdef";

    // Logic to check usage goes here.

    HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (INVALID_HANDLE_VALUE == hFile)
        return GetLastError();

    LARGE_INTEGER dwFileSize;
    GetFileSizeEx(hFile, &dwFileSize);
    if (dwFileSize.QuadPart > UINT32_MAX) {
        CloseHandle(hFile);
        return ERROR_FILE_TOO_LARGE;
    }
    if (!dwFileSize.LowPart) {
        DWORD dwStatus = GetLastError();
        CloseHandle(hFile);
        return dwStatus;
    }

    BYTE *rgbFile = malloc(dwFileSize.LowPart);
    if (!rgbFile) {
        DWORD dwStatus = E_OUTOFMEMORY;
        CloseHandle(hFile);
        return dwStatus;
    }

    HCRYPTPROV hProv = 0;
    // Get handle to the crypto provider
    if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        DWORD dwStatus = GetLastError();
        CloseHandle(hFile);
        free(rgbFile);
        return dwStatus;
    }

    HCRYPTHASH hHash = 0;
    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
        DWORD dwStatus = GetLastError();
        CloseHandle(hFile);
        CryptReleaseContext(hProv, 0);
        free(rgbFile);
        return dwStatus;
    }

    DWORD cbRead = 0;
    BOOL  bResult;
    while ((bResult = ReadFile(hFile, rgbFile, dwFileSize.LowPart, &cbRead, NULL)))
    {
        if (cbRead == 0)
            break;
        if (!CryptHashData(hHash, rgbFile, cbRead, 0)) {
            DWORD dwStatus = GetLastError();
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            free(rgbFile);
            return dwStatus;
        }
    }

    if (!bResult) {
        DWORD dwStatus = GetLastError();
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        CloseHandle(hFile);
        free(rgbFile);
        return dwStatus;
    }

    BYTE  rgbHash[16];
    DWORD dwStatus;
    DWORD cbHash = 16;
    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
        dwStatus = 0;
        for (size_t i = 0; i < cbHash; i++) {
            sprintf_s(hash + (i * 2), dwHash - (i * 2), "%c%c",
                      rgbDigits[(rgbHash[i] >> 4) & 0xF],
                      rgbDigits[rgbHash[i] & 0xF]);
        }
    } else {
        dwStatus = GetLastError();
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);
    free(rgbFile);

    return dwStatus;
}

#if 0
int ComputeFileHash2(HMODULE hModule, LPCWSTR filename, LPSTR hash, DWORD dwHash)
{
    if (dwHash < 33)
        return ERROR_BUFFER_OVERFLOW;
    if (!hModule)
        return ERROR_INVALID_ADDRESS;

    DWORD dwLeftMost = 0;
    DWORD dwSecondLeft = 0;
    DWORD dwSecondRight = 0;
    DWORD dwRightMost = 0;
    QueryVersionInfo(hModule, VS_VERSION_INFO, &dwLeftMost, &dwSecondLeft, &dwSecondRight, &dwRightMost);

    (void)sprintf_s(hash, 33, "%lu.%lu.%lu.%lu.",
                    dwLeftMost == 22621 ? 22622 : dwLeftMost,
                    dwSecondLeft, dwSecondRight, dwRightMost);

    char real_hash[33];
    ComputeFileHash(filename, real_hash, 33);
    strncpy_s(hash + strlen(hash), dwHash - strlen(hash), real_hash, 32 - strlen(hash));
    if (dwLeftMost == 22622)
        *(strchr(strchr(strchr(strchr(hash, '.') + 1, '.') + 1, '.') + 1, '.') + 1) = '!';
    hash[33] = 0;

    return ERROR_SUCCESS;
}
#endif

int ComputeFileHash2(HMODULE hModule, LPCWSTR filename, LPSTR hash, DWORD dwHash)
{
#define HASH_SIZE 33
    if (!hash || !filename)
        return ERROR_BAD_ARGUMENTS;
    if (!hModule)
        return ERROR_INVALID_ADDRESS;
    if (dwHash < HASH_SIZE + 1)
        return ERROR_BUFFER_OVERFLOW;

    DWORD dwLeftMost    = 0;
    DWORD dwSecondLeft  = 0;
    DWORD dwSecondRight = 0;
    DWORD dwRightMost   = 0;
    QueryVersionInfo(hModule, VS_VERSION_INFO, &dwLeftMost,
                     &dwSecondLeft, &dwSecondRight, &dwRightMost);

    int res = sprintf_s(hash, HASH_SIZE + 1, "%lu.%lu.%lu.%lu.",
                        dwLeftMost == 22621 ? 22622 : dwLeftMost,
                        dwSecondLeft, dwSecondRight, dwRightMost);
    if (res != 0)
        return res; // Return the error from sprintf_s

    char real_hash[HASH_SIZE];
    ComputeFileHash(filename, real_hash, HASH_SIZE);

    size_t currentLen = strlen(hash);
    if (currentLen >= dwHash)
        return ERROR_BUFFER_OVERFLOW;

    res = strncat_s(hash, dwHash, real_hash, dwHash - currentLen - 1);
    if (res != 0)
        return res; // Return the error from strncat_s

    if (dwLeftMost == 22622) {
        char *ptr = hash;
        for (int i = 0; i < 4; ++i) { // Find the 4th dot
            ptr = strchr(ptr, '.');
            if (!ptr)
                return ERROR_INVALID_DATA;
        }
        ptr[1] = '!';
    }

    return ERROR_SUCCESS;
#undef HASH_SIZE
}

void LaunchPropertiesGUI(HMODULE hModule)
{
    //CreateThread(0, 0, ZZGUI, 0, 0, 0);
    wchar_t wszPath[MAX_PATH * 2] = {0};
    wszPath[0] = '\"';
    GetSystemDirectoryW(wszPath + 1, MAX_PATH);
    wcscat_s(wszPath, MAX_PATH * 2, L"\\rundll32.exe\" \"");
    GetModuleFileNameW(hModule, wszPath + wcslen(wszPath), MAX_PATH);
    wcscat_s(wszPath, MAX_PATH * 2, L"\",ZZGUI");
    wprintf(L"Launching : %ls\n", wszPath);

    STARTUPINFO si = {.cb = sizeof(si)};
    PROCESS_INFORMATION pi;

    if (CreateProcessW(
        NULL, wszPath, NULL, NULL, FALSE,
        CREATE_UNICODE_ENVIRONMENT,
        NULL, NULL, &si, &pi
    ))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}


BOOL SystemShutdown(BOOL reboot)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // Get a token for this process. 

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return FALSE;

    // Get the LUID for the shutdown privilege. 

    LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set    
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get the shutdown privilege for this process. 

    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);

    if (GetLastError() != ERROR_SUCCESS)
        return FALSE;

    // Shut down the system and force all applications to close. 

    if (!ExitWindowsEx((reboot ? EWX_REBOOT : EWX_SHUTDOWN) | EWX_FORCE,
        SHTDN_REASON_MAJOR_OPERATINGSYSTEM |
        SHTDN_REASON_MINOR_UPGRADE |
        SHTDN_REASON_FLAG_PLANNED))
        return FALSE;

    //shutdown was successful
    return TRUE;
}

HRESULT FindDesktopFolderView(REFIID riid, void** ppv)
{
    HRESULT hr = E_FAIL;
    IShellWindows* spShellWindows = NULL;
    hr = CoCreateInstance(&CLSID_ShellWindows, NULL, CLSCTX_ALL, &IID_IShellWindows, &spShellWindows);
    if (spShellWindows)
    {
        VARIANT vtEmpty;
        ZeroMemory(&vtEmpty, sizeof(VARIANT));
        VARIANT vtLoc;
        ZeroMemory(&vtLoc, sizeof(VARIANT));
        vtLoc.vt = VT_INT;
        vtLoc.intVal = CSIDL_DESKTOP;
        long lhwnd = 0;
        IDispatch* spdisp = NULL;
        hr = spShellWindows->lpVtbl->FindWindowSW(
            spShellWindows,
            &vtLoc,
            &vtEmpty,
            SWC_DESKTOP,
            &lhwnd,
            SWFO_NEEDDISPATCH,
            &spdisp
        );
        if (spdisp)
        {
            IServiceProvider* spdisp2 = NULL;
            hr = spdisp->lpVtbl->QueryInterface(spdisp, &IID_IServiceProvider, &spdisp2);
            if (spdisp2)
            {
                IShellBrowser* spBrowser = NULL;
                hr = spdisp2->lpVtbl->QueryService(spdisp2, &SID_STopLevelBrowser, &IID_IShellBrowser, &spBrowser);
                if (spBrowser)
                {
                    IShellView* spView = NULL;
                    hr = spBrowser->lpVtbl->QueryActiveShellView(spBrowser, &spView);
                    if (spView)
                    {
                        hr = spView->lpVtbl->QueryInterface(spView, riid, ppv);
                        spView->lpVtbl->Release(spView);
                    }
                    spBrowser->lpVtbl->Release(spBrowser);
                }
                spdisp2->lpVtbl->Release(spdisp2);
            }
            spdisp->lpVtbl->Release(spdisp);
        }
        spShellWindows->lpVtbl->Release(spShellWindows);
    }
    return hr;
}

HRESULT GetDesktopAutomationObject(REFIID riid, void** ppv)
{
    HRESULT hr = E_FAIL;
    IShellView* spsv = NULL;
    hr = FindDesktopFolderView(&IID_IShellView, &spsv);
    if (spsv)
    {
        IDispatch* spdispView = NULL;
        hr = spsv->lpVtbl->GetItemObject(spsv, SVGIO_BACKGROUND, &IID_IDispatch, &spdispView);
        if (spdispView)
        {
            hr = spdispView->lpVtbl->QueryInterface(spdispView, riid, ppv);
            spdispView->lpVtbl->Release(spdispView);
        }
        spsv->lpVtbl->Release(spsv);
    }
    return hr;
}

HRESULT ShellExecuteFromExplorer(
    PCWSTR pszFile,
    PCWSTR pszParameters,
    PCWSTR pszDirectory,
    PCWSTR pszOperation,
    int nShowCmd
)
{
    HRESULT hr = E_FAIL;
    IShellFolderViewDual* spFolderView = NULL;
    GetDesktopAutomationObject(&IID_IShellFolderViewDual, &spFolderView);
    if (spFolderView)
    {
        IDispatch* spdispShell = NULL;
        spFolderView->lpVtbl->get_Application(spFolderView, &spdispShell);
        if (spdispShell)
        {
            IShellDispatch2* spdispShell2 = NULL;
            spdispShell->lpVtbl->QueryInterface(spdispShell, &IID_IShellDispatch2, &spdispShell2);
            if (spdispShell2)
            {
                BSTR a_pszFile = pszFile ? SysAllocString(pszFile): SysAllocString(L"");
                VARIANT a_pszParameters, a_pszDirectory, a_pszOperation, a_nShowCmd;
                ZeroMemory(&a_pszParameters, sizeof(VARIANT));
                ZeroMemory(&a_pszDirectory, sizeof(VARIANT));
                ZeroMemory(&a_pszOperation, sizeof(VARIANT));
                ZeroMemory(&a_nShowCmd, sizeof(VARIANT));
                a_pszParameters.vt = VT_BSTR;
                a_pszParameters.bstrVal = pszParameters ? SysAllocString(pszParameters) : SysAllocString(L"");
                a_pszDirectory.vt = VT_BSTR;
                a_pszDirectory.bstrVal = pszDirectory ? SysAllocString(pszDirectory) : SysAllocString(L"");
                a_pszOperation.vt = VT_BSTR;
                a_pszOperation.bstrVal = pszOperation ? SysAllocString(pszOperation) : SysAllocString(L"");
                a_nShowCmd.vt = VT_INT;
                a_nShowCmd.intVal = nShowCmd;
                hr = spdispShell2->lpVtbl->ShellExecuteW(spdispShell2, a_pszFile, a_pszParameters, a_pszDirectory, a_pszOperation, a_nShowCmd);
                SysFreeString(a_pszOperation.bstrVal);
                SysFreeString(a_pszDirectory.bstrVal);
                SysFreeString(a_pszParameters.bstrVal);
                SysFreeString(a_pszFile);
                spdispShell2->lpVtbl->Release(spdispShell2);
            }
            spdispShell->lpVtbl->Release(spdispShell);
        }
        spFolderView->lpVtbl->Release(spFolderView);
    }
    return hr;
}

void ToggleTaskbarAutohide(void)
{
    APPBARDATA abd;
    abd.cbSize = sizeof(APPBARDATA);
    if (SHAppBarMessage(ABM_GETSTATE, &abd) == ABS_AUTOHIDE) {
        abd.lParam = 0;
        SHAppBarMessage(ABM_SETSTATE, &abd);
    } else {
        abd.lParam = ABS_AUTOHIDE;
        SHAppBarMessage(ABM_SETSTATE, &abd);
    }
}

LSTATUS RegisterDWMService(DWORD dwDesiredState, DWORD dwOverride)
{
    WCHAR wszPath[MAX_PATH];
    GetSystemDirectoryW(wszPath, MAX_PATH);
    wcscat_s(wszPath, MAX_PATH, L"\\cmd.exe");

    WCHAR wszSCPath[MAX_PATH];
    GetSystemDirectoryW(wszSCPath, MAX_PATH);
    wcscat_s(wszSCPath, MAX_PATH, L"\\sc.exe");

    WCHAR wszRundll32[MAX_PATH];
    SHGetFolderPathW(NULL, SPECIAL_FOLDER, NULL, SHGFP_TYPE_CURRENT, wszRundll32);
    wcscat_s(wszRundll32, MAX_PATH, L"" APP_RELATIVE_PATH);
    wcscat_s(wszRundll32, MAX_PATH, L"\\ep_dwm.exe");

    WCHAR wszEP[MAX_PATH];
    GetWindowsDirectoryW(wszEP, MAX_PATH);
    wcscat_s(wszEP, MAX_PATH, L"\\dxgi.dll");

    WCHAR wszTaskkill[MAX_PATH];
    GetSystemDirectoryW(wszTaskkill, MAX_PATH);
    wcscat_s(wszTaskkill, MAX_PATH, L"\\taskkill.exe");

    WCHAR wszArgumentsRegister[MAX_PATH * 10];
    swprintf_s(
        wszArgumentsRegister,
        MAX_PATH * 10,
        L"/c \""
        L"\"%s\" create " EP_DWM_SERVICENAME
        L" binPath= \"\\\"%s\\\" %s\" DisplayName= \"ExplorerPatcher Desktop Window Manager Service\" start= auto & "
        L"\"%s\" description " EP_DWM_SERVICENAME
        L" \"Service for managing aspects related to the Desktop Window Manager.\" & "
        L"\"%s\" %s " EP_DWM_SERVICENAME L"\"",
        wszSCPath,
        wszRundll32,
        EP_DWM_SERVICENAME L" " EP_DWM_EVENTNAME,
        wszSCPath,
        wszSCPath,
        (!dwOverride || dwOverride == 3) ? L"start" : L"query"
    );

    WCHAR wszArgumentsUnRegister[MAX_PATH * 10];
    swprintf_s(
        wszArgumentsUnRegister,
        MAX_PATH * 10,
        L"/c \""
        L"\"%s\" stop " EP_DWM_SERVICENAME L" & "
        L"\"%s\" delete " EP_DWM_SERVICENAME L" & "
        L"\"",
        wszSCPath,
        wszSCPath
    );
    wprintf(L"%s\n", wszArgumentsRegister);

    BOOL bAreRoundedCornersDisabled;
    if (dwOverride) {
        bAreRoundedCornersDisabled = !(dwOverride - 1);
    } else {
        HANDLE h_exists = CreateEventW(NULL, FALSE, FALSE, L"" EP_DWM_EVENTNAME);
        if (h_exists) {
            bAreRoundedCornersDisabled = GetLastError() == ERROR_ALREADY_EXISTS;
            CloseHandle(h_exists);
        } else {
            bAreRoundedCornersDisabled = GetLastError() == ERROR_ACCESS_DENIED;
        }
        if ((bAreRoundedCornersDisabled && dwDesiredState) || (!bAreRoundedCornersDisabled && !dwDesiredState))
            return FALSE;
    }

    SHELLEXECUTEINFO ShExecInfo = {
        .cbSize       = sizeof(SHELLEXECUTEINFO),
        .fMask        = SEE_MASK_NOCLOSEPROCESS,
        .lpFile       = wszPath,
        .lpParameters = !bAreRoundedCornersDisabled ? wszArgumentsRegister : wszArgumentsUnRegister,
        .lpVerb       = L"runas",
        .nShow        = SW_HIDE,
    };

    if (ShellExecuteExW(&ShExecInfo) && ShExecInfo.hProcess)
    {
        WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
        DWORD dwExitCode = 0;
        GetExitCodeProcess(ShExecInfo.hProcess, &dwExitCode);
        CloseHandle(ShExecInfo.hProcess);
    }
    return TRUE;
}

char *StrReplaceAllA(const char *s, const char *oldW, const char *newW, DWORD *dwNewSize)
{
    char* result;
    size_t i;
    size_t cnt = 0;
    size_t newWlen = strlen(newW);
    size_t oldWlen = strlen(oldW);

    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], oldW) == &s[i]) {
            cnt++;
            i += oldWlen - 1;
        }
    }
    result = malloc(i + cnt * (newWlen - oldWlen) + 1);
    i = 0;
    while (*s) {
        if (strstr(s, oldW) == s) {
            strcpy_s(&result[i], strlen(newW) + 1, newW);
            i += newWlen;
            s += oldWlen;
        }
        else {
            result[i++] = *s++;
        }
    }

    result[i] = '\0';
    if (dwNewSize)
        *dwNewSize = (DWORD)i;
    return result;
}

WCHAR *StrReplaceAllW(const WCHAR *s, const WCHAR *oldW, const WCHAR *newW, DWORD *dwNewSize)
{
    WCHAR* result;
    size_t i, cnt = 0;
    size_t newWlen = wcslen(newW);
    size_t oldWlen = wcslen(oldW);

    for (i = 0; s[i] != L'\0'; i++) {
        if (wcsstr(&s[i], oldW) == &s[i]) {
            cnt++;
            i += oldWlen - 1;
        }
    }
    result = (WCHAR*)malloc((i + cnt * (newWlen - oldWlen) + 1) * sizeof(WCHAR));
    i = 0;
    while (*s) {
        if (wcsstr(s, oldW) == s) {
            wcscpy_s(&result[i], newWlen + 1, newW);
            i += newWlen;
            s += oldWlen;
        }
        else {
            result[i++] = *s++;
        }
    }
    result[i] = L'\0';
    if (dwNewSize)
        *dwNewSize = (DWORD)i;
    return result;
}

HWND InputBox_HWND;

HRESULT getEngineGuid(LPCTSTR extension, GUID* guidBuffer)
{
    wchar_t   buffer[100];
    HKEY      hk;
    DWORD     size;
    HKEY      subKey;
    DWORD     type;

    // See if this file extension is associated
    // with an ActiveX script engine
    if (!RegOpenKeyExW(HKEY_CLASSES_ROOT, extension, 0, KEY_QUERY_VALUE | KEY_READ, &hk))
    {
        type = REG_SZ;
        size = sizeof buffer;
        size = RegQueryValueExW(hk, 0, 0, &type, (LPBYTE)&buffer[0], &size);
        RegCloseKey(hk);
        if (!size)
        {
            // The engine set an association.
            // We got the Language string in buffer[]. Now
            // we can use it to look up the engine's GUID

            // Open HKEY_CLASSES_ROOT\{LanguageName}
        again:  
            size = sizeof buffer;
            if (!RegOpenKeyExW(HKEY_CLASSES_ROOT, &buffer[0], 0, KEY_QUERY_VALUE | KEY_READ, &hk))
            {
                // Read the GUID (in string format)
                // into buffer[] by querying the value of CLSID
                if (!RegOpenKeyExW(hk, L"CLSID", 0, KEY_QUERY_VALUE | KEY_READ, &subKey))
                {
                    size = RegQueryValueExW(subKey, 0, 0, &type,
                        (LPBYTE)&buffer[0], &size);
                    RegCloseKey(subKey);
                }
                else if (extension && !RegOpenKeyExW(hk, L"ScriptEngine", 0, KEY_QUERY_VALUE | KEY_READ, &subKey))
                {
                    // If an error, see if we have a "ScriptEngine"
                    // key under here that contains
                    // the real language name
                    size = RegQueryValueExW(subKey, 0, 0, &type,
                                           (LPBYTE)&buffer[0], &size);
                    RegCloseKey(subKey);
                    if (!size)
                    {
                        RegCloseKey(hk);
                        extension = 0;
                        goto again;
                    }
                }
            }

            RegCloseKey(hk);

            if (!size)
            {
                // Convert the GUID string to a GUID
                // and put it in caller's guidBuffer
                if ((size = CLSIDFromString(&buffer[0], guidBuffer)))
                    return E_FAIL;
                return (HRESULT)size;
            }
        }
    }

    return E_FAIL;
}

ULONG STDMETHODCALLTYPE ep_static_AddRefRelease(void* _this)
{
    return 1;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSite_QueryInterface(void* _this, REFIID riid, void** ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IActiveScriptSite))
        *ppv = _this;
    else if (IsEqualIID(riid, &IID_IActiveScriptSiteWindow))
        *ppv = ((unsigned char*)_this + 8);
    else
    {
        *ppv = 0;
        return(E_NOINTERFACE);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSiteWindow_QueryInterface(void* _this, REFIID riid, void** ppv)
{
    return IActiveScriptSite_QueryInterface((char*)_this - 8, riid, ppv);
}

HRESULT STDMETHODCALLTYPE IActiveScriptSite_GetLCID(void* _this, LCID* plcid)
{
    *plcid = LOCALE_USER_DEFAULT;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSite_GetItemInfo(void* _this, LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown** ppiunkItem, ITypeInfo** ppti)
{
    return TYPE_E_ELEMENTNOTFOUND;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSite_GetDocVersionString(void* _this, BSTR* pbstrVersion)
{
    *pbstrVersion = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSite_OnScriptTerminate(void* _this, const void* pvarResult, const EXCEPINFO* pexcepinfo)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSite_OnStateChange(void* _this, SCRIPTSTATE ssScriptState)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSite_OnScriptError(void* _this, IActiveScriptError* scriptError)
{
    ULONG        lineNumber;
    BSTR         desc;
    EXCEPINFO    ei;
    OLECHAR      wszOutput[1024];

    // Call GetSourcePosition() to retrieve the line # where
    // the error occurred in the script
    scriptError->lpVtbl->GetSourcePosition(scriptError, 0, &lineNumber, 0);

    // Call GetSourceLineText() to retrieve the line in the script that
    // has an error.
    desc = 0;
    scriptError->lpVtbl->GetSourceLineText(scriptError, &desc);

    // Call GetExceptionInfo() to fill in our EXCEPINFO struct with more
    // information.
    ZeroMemory(&ei, sizeof(EXCEPINFO));
    scriptError->lpVtbl->GetExceptionInfo(scriptError, &ei);

    // Format the message we'll display to the user
    swprintf_s(wszOutput, ARRAYSIZE(wszOutput), L"%ls\nLine %u: %ls\n%ls", ei.bstrSource, lineNumber + 1, ei.bstrDescription, desc ? desc : L"");

    // Free what we got from the IActiveScriptError functions
    SysFreeString(desc);
    SysFreeString(ei.bstrSource);
    SysFreeString(ei.bstrDescription);
    SysFreeString(ei.bstrHelpFile);

    // Display the message
    MessageBoxW(NULL, &wszOutput[0], L"Error", MB_SETFOREGROUND | MB_OK | MB_ICONEXCLAMATION);

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE IActiveScriptSite_OnEnterScript(void* _this)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSite_OnLeaveScript(void* _this)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSiteWindow_GetWindow(void* _this, HWND* phWnd)
{
    *phWnd = InputBox_HWND;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSiteWindow_EnableModeless(void* _this, BOOL fEnable)
{
    return S_OK;
}

static const IActiveScriptSiteVtbl IActiveScriptSite_Vtbl = {
    .QueryInterface      = IActiveScriptSite_QueryInterface,
    .AddRef              = ep_static_AddRefRelease,
    .Release             = ep_static_AddRefRelease,
    .GetLCID             = IActiveScriptSite_GetLCID,
    .GetItemInfo         = IActiveScriptSite_GetItemInfo,
    .GetDocVersionString = IActiveScriptSite_GetDocVersionString,
    .OnScriptTerminate   = IActiveScriptSite_OnScriptTerminate,
    .OnStateChange       = IActiveScriptSite_OnStateChange,
    .OnScriptError       = IActiveScriptSite_OnScriptError,
    .OnEnterScript       = IActiveScriptSite_OnEnterScript,
    .OnLeaveScript       = IActiveScriptSite_OnLeaveScript,
};

static const IActiveScriptSiteWindowVtbl IActiveScriptSiteWindow_Vtbl = {
    .QueryInterface = IActiveScriptSiteWindow_QueryInterface,
    .AddRef         = ep_static_AddRefRelease,
    .Release        = ep_static_AddRefRelease,
    .GetWindow      = IActiveScriptSiteWindow_GetWindow,
    .EnableModeless = IActiveScriptSiteWindow_EnableModeless,
};

typedef struct CSimpleScriptSite
{
    IActiveScriptSiteVtbl* lpVtbl;
    IActiveScriptSiteWindowVtbl* lpVtbl1;
} CSimpleScriptSite;

static const CSimpleScriptSite CSimpleScriptSite_Instance = {
    .lpVtbl = &IActiveScriptSite_Vtbl,
    .lpVtbl1 = &IActiveScriptSiteWindow_Vtbl
};

static BOOL HideInput = FALSE;
static LRESULT CALLBACK InputBoxProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < HC_ACTION)
        return CallNextHookEx(0, nCode, wParam, lParam);
    if ((nCode = HCBT_ACTIVATE)) {
        if (HideInput == TRUE) {
            HWND TextBox = FindWindowExA((HWND)wParam, NULL, "Edit", NULL);
            SendDlgItemMessageW((HWND)wParam, GetDlgCtrlID(TextBox), EM_SETPASSWORDCHAR, L'\x25cf', 0);
        }
    }
    if ((nCode = HCBT_CREATEWND)) {
        if (!(GetWindowLongPtrW((HWND)wParam, GWL_STYLE) & WS_CHILD))
            SetWindowLongPtrW((HWND)wParam, GWL_EXSTYLE, GetWindowLongPtrW((HWND)wParam, GWL_EXSTYLE) | WS_EX_DLGMODALFRAME);
    }
    return CallNextHookEx(0, nCode, wParam, lParam);
}

HRESULT InputBox(BOOL bPassword, HWND hWnd, LPCWSTR wszPrompt, LPCWSTR wszTitle, LPCWSTR wszDefault, LPWSTR wszAnswer, DWORD cbAnswer, BOOL* bCancelled)
{
    HRESULT hr = S_OK;

    if (!wszPrompt || !wszTitle || !wszDefault || !wszAnswer || !cbAnswer || !bCancelled)
    {
        return E_FAIL;
    }

    GUID guidBuffer;
    hr = getEngineGuid(L".vbs", &guidBuffer);

    DWORD cchPromptSafe = 0, cchTitleSafe = 0, cchDefaultSafe = 0;
    LPWSTR wszPromptSafe = StrReplaceAllW(wszPrompt, L"\"", L"\"\"", &cchPromptSafe);
    LPWSTR wszTitleSafe = StrReplaceAllW(wszTitle, L"\"", L"\"\"", &cchTitleSafe);
    LPWSTR wszDefaultSafe = StrReplaceAllW(wszDefault, L"\"", L"\"\"", &cchDefaultSafe);
    if (!wszPromptSafe || !wszTitleSafe || !wszDefaultSafe)
    {
        free(wszPromptSafe);
        free(wszTitleSafe);
        free(wszDefaultSafe);
        return E_OUTOFMEMORY;
    }

    IActiveScript* pActiveScript = NULL;
    hr = CoCreateInstance(FAILED(hr) ? &CLSID_VBScript : &guidBuffer, 0, CLSCTX_ALL,
        &IID_IActiveScript,
        (void**)&pActiveScript);
    if (SUCCEEDED(hr) && pActiveScript)
    {
        hr = pActiveScript->lpVtbl->SetScriptSite(pActiveScript, &CSimpleScriptSite_Instance);
        if (SUCCEEDED(hr))
        {
            IActiveScriptParse* pActiveScriptParse = NULL;
            hr = pActiveScript->lpVtbl->QueryInterface(pActiveScript, &IID_IActiveScriptParse, &pActiveScriptParse);
            if (SUCCEEDED(hr) && pActiveScriptParse)
            {
                hr = pActiveScriptParse->lpVtbl->InitNew(pActiveScriptParse);
                if (SUCCEEDED(hr))
                {
                    LPWSTR wszEvaluation = malloc(sizeof(WCHAR) * (cchPromptSafe + cchTitleSafe + cchDefaultSafe + 100));
                    if (wszEvaluation)
                    {
                        swprintf_s(wszEvaluation, cchPromptSafe + cchTitleSafe + cchDefaultSafe + 100, L"InputBox(\"%s\", \"%s\", \"%s\")", wszPromptSafe, wszTitleSafe, wszDefaultSafe);
                        DWORD cchEvaluation2 = 0;
                        LPWSTR wszEvaluation2 = StrReplaceAllW(wszEvaluation, L"\n", L"\" + vbNewLine + \"", &cchEvaluation2);
                        if (wszEvaluation2)
                        {
                            EXCEPINFO ei;
                            ZeroMemory(&ei, sizeof(EXCEPINFO));
                            DWORD dwThreadId = GetCurrentThreadId();
                            HINSTANCE hInstance = GetModuleHandleW(NULL);

                            InputBox_HWND = hWnd ? hWnd : GetAncestor(GetActiveWindow(), GA_ROOTOWNER);

                            HHOOK hHook = SetWindowsHookExW(WH_CBT, &InputBoxProc, hInstance, dwThreadId);

                            VARIANT result;
                            VariantInit(&result);

                            HideInput = bPassword;
                            hr = pActiveScriptParse->lpVtbl->ParseScriptText(pActiveScriptParse, wszEvaluation2, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &result, &ei);

                            *bCancelled = (result.vt == VT_EMPTY);

                            UnhookWindowsHookEx(hHook);

                            free(wszEvaluation2);

                            if (result.bstrVal)
                                memcpy(wszAnswer, result.bstrVal, cbAnswer * sizeof(WCHAR));
                            else if (result.vt != VT_EMPTY)
                                wszAnswer[0] = 0;

                            VariantClear(&result);
                        }
                        free(wszEvaluation);
                    }
                }
                pActiveScriptParse->lpVtbl->Release(pActiveScriptParse);
            }
            pActiveScript->lpVtbl->Release(pActiveScript);
        }
    }

    free(wszPromptSafe);
    free(wszTitleSafe);
    free(wszDefaultSafe);

    return hr;
}

BOOL IsHighContrast(void)
{
    HIGHCONTRASTW highContrast;
    ZeroMemory(&highContrast, sizeof(HIGHCONTRASTW));
    highContrast.cbSize = sizeof(highContrast);
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, FALSE))
        return highContrast.dwFlags & HCF_HIGHCONTRASTON;
    return FALSE;
}

WCHAR *ep_generate_random_wide_string(WCHAR *str, size_t size)
{
//#define RAND(val) ((val) = ExplorerPatcher_cxx_random_engine_get_random_val_32())
#define RAND(val) rand_s(&(val))

    static wchar_t const repertoir[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@$";
    my_static_assert(sizeof(repertoir) / sizeof(wchar_t) - 1 == 64);

    wchar_t *ptr = str;

    for (size_t i = 0; i < size; ++i) {
        uint32_t val;
        RAND(val);
        *ptr++ = repertoir[val % 64U];
    }

    *ptr = L'\0';
    return str;

#undef RAND
}

ULONGLONG milliseconds_now(void)
{
    LARGE_INTEGER s_frequency;
    BOOL          s_use_qpc = QueryPerformanceFrequency(&s_frequency);
    if (s_use_qpc) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (1000LL * now.QuadPart) / s_frequency.QuadPart;
    } else {
        return GetTickCount64();
    }
}

BOOL IsAppRunningAsAdminMode(void)
{
    BOOL  fIsRunAsAdmin        = FALSE;
    DWORD dwError              = ERROR_SUCCESS;
    PSID  pAdministratorsGroup = NULL;

    // Allocate and initialize a SID of the administrators group.
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(
        &NtAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pAdministratorsGroup))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Determine whether the SID of administrators group is enabled in 
    // the primary access token of the process.
    if (!CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin))
        dwError = GetLastError();

Cleanup:
    // Centralized cleanup for all allocated resources.
    if (pAdministratorsGroup)
    {
        FreeSid(pAdministratorsGroup);
        pAdministratorsGroup = NULL;
    }

    // Throw the error if something failed in the function.
    if (ERROR_SUCCESS != dwError)
        return FALSE;

    return fIsRunAsAdmin;
}

BOOL IsDesktopWindowAlreadyPresent(void)
{
    return (FindWindowExW(NULL, NULL, L"Progman", NULL) || FindWindowExW(NULL, NULL, L"Proxy Desktop", NULL));
}

UINT PleaseWaitTimeout = 0;
HHOOK PleaseWaitHook = NULL;
HWND PleaseWaitHWND = NULL;
void* PleaseWaitCallbackData = NULL;
BOOL (*PleaseWaitCallbackFunc)(void* data) = NULL;

BOOL PleaseWait_UpdateTimeout(int timeout)
{
    uint32_t fourChars;
    memcpy(&fourChars, "EPPW", 4);

    if (PleaseWaitHWND)
    {
        KillTimer(PleaseWaitHWND, fourChars);
        PleaseWaitTimeout = timeout;
        return SetTimer(PleaseWaitHWND, fourChars, PleaseWaitTimeout, PleaseWait_TimerProc);
    }
    return FALSE;
}

void CALLBACK PleaseWait_TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    uint32_t fourChars;
    memcpy(&fourChars, "EPPW", 4);

    if (idEvent == fourChars) {
        if (PleaseWaitCallbackFunc) {
            if (PleaseWaitCallbackFunc(PleaseWaitCallbackData))
                return;
            PleaseWaitCallbackData = NULL;
            PleaseWaitCallbackFunc = NULL;
        }
        KillTimer(hWnd, fourChars);
        SetTimer(hWnd, fourChars, 0, NULL); // <- this closes the message box
        PleaseWaitHWND = NULL;
        PleaseWaitTimeout = 0;
    }
}

LRESULT CALLBACK PleaseWait_HookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
    {
        return CallNextHookEx(NULL, code, wParam, lParam);
    }

    CWPSTRUCT* msg = (CWPSTRUCT*)lParam;
    /*if (msg->message == WM_CREATE)
    {
        CREATESTRUCT* pCS = (CREATESTRUCT*)msg->lParam;
        if (pCS->lpszClass == RegisterWindowMessageW(L"Button"))
        {
        }
    }*/
    LRESULT result = CallNextHookEx(NULL, code, wParam, lParam);

    if (msg->message == WM_INITDIALOG)
    {
        uint32_t fourChars;
        memcpy(&fourChars, "EPPW", 4);

        PleaseWaitHWND = msg->hwnd;
        EnableWindow(PleaseWaitHWND, FALSE);
        LONG_PTR style = GetWindowLongPtrW(PleaseWaitHWND, GWL_STYLE);
        SetWindowLongPtrW(PleaseWaitHWND, GWL_STYLE, style & ~WS_SYSMENU);
        RECT rc;
        GetWindowRect(PleaseWaitHWND, &rc);
        SetWindowPos(PleaseWaitHWND, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top - MulDiv(50, GetDpiForWindow(PleaseWaitHWND), 96), SWP_NOMOVE | SWP_FRAMECHANGED);
        SetTimer(PleaseWaitHWND, fourChars, PleaseWaitTimeout, PleaseWait_TimerProc);
        UnhookWindowsHookEx(PleaseWaitHook);
        PleaseWaitHook = NULL;
    }
    return result;
}

BOOL DownloadAndInstallWebView2Runtime()
{
    BOOL bOK = FALSE;
    HINTERNET hInternet = NULL;
    if ((hInternet = InternetOpenA(
        "ExplorerPatcher",
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL,
        NULL,
        0
    )))
    {
        HINTERNET hConnect = InternetOpenUrlA(
            hInternet,
            "https://go.microsoft.com/fwlink/p/?LinkId=2124703",
            NULL,
            0,
            INTERNET_FLAG_RAW_DATA |
            INTERNET_FLAG_RELOAD |
            INTERNET_FLAG_RESYNCHRONIZE |
            INTERNET_FLAG_NO_COOKIES |
            INTERNET_FLAG_NO_UI |
            INTERNET_FLAG_NO_CACHE_WRITE |
            INTERNET_FLAG_DONT_CACHE,
            NULL
        );
        if (hConnect)
        {
            char* exe_buffer = NULL;
            DWORD dwSize = 2 * 1024 * 1024;
            DWORD dwRead = dwSize;
            exe_buffer = calloc(dwSize, sizeof(char));
            if (exe_buffer)
            {
                BOOL bRet = FALSE;
                if ((bRet = InternetReadFile(
                    hConnect,
                    exe_buffer,
                    dwSize - 1,
                    &dwRead
                )))
                {
                    WCHAR wszPath[MAX_PATH];
                    ZeroMemory(wszPath, MAX_PATH * sizeof(WCHAR));
                    SHGetFolderPathW(NULL, SPECIAL_FOLDER_LEGACY, NULL, SHGFP_TYPE_CURRENT, wszPath);
                    wcscat_s(wszPath, MAX_PATH, L"" APP_RELATIVE_PATH);
                    BOOL bRet = CreateDirectoryW(wszPath, NULL);
                    if (bRet || (!bRet && GetLastError() == ERROR_ALREADY_EXISTS))
                    {
                        wcscat_s(wszPath, MAX_PATH, L"\\MicrosoftEdgeWebview2Setup.exe");
                        FILE* f = NULL;
                        _wfopen_s(&f, wszPath, L"wb");
                        if (f)
                        {
                            fwrite(exe_buffer, 1, dwRead, f);
                            fclose(f);
                            SHELLEXECUTEINFOW sei = {
                                .cbSize = sizeof(sei),
                                .fMask = SEE_MASK_NOCLOSEPROCESS,
                                .hwnd = NULL,
                                .hInstApp = NULL,
                                .lpVerb = NULL,
                                .lpFile = wszPath,
                                .lpParameters = L"",
                                .hwnd = NULL,
                                .nShow = SW_SHOWMINIMIZED,
                            };

                            if (ShellExecuteExW(&sei) && sei.hProcess)
                            {
                                WaitForSingleObject(sei.hProcess, INFINITE);
                                CloseHandle(sei.hProcess);
                                Sleep(100);
                                DeleteFileW(wszPath);
                                bOK = TRUE;
                            }
                        }
                    }
                }
                free(exe_buffer);
            }
            InternetCloseHandle(hConnect);
        }
        InternetCloseHandle(hInternet);
    }
    return bOK;
}

BOOL DownloadFile(LPCWSTR wszURL, DWORD dwSize, LPCWSTR wszPath)
{
    BOOL bOK = FALSE;
    HINTERNET hInternet = NULL;
    if ((hInternet = InternetOpenW(
        L"ExplorerPatcher",
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL,
        NULL,
        0
    )))
    {
        HINTERNET hConnect = InternetOpenUrlW(
            hInternet,
            wszURL,
            NULL,
            0,
            INTERNET_FLAG_RAW_DATA |
            INTERNET_FLAG_RELOAD |
            INTERNET_FLAG_RESYNCHRONIZE |
            INTERNET_FLAG_NO_COOKIES |
            INTERNET_FLAG_NO_UI |
            INTERNET_FLAG_NO_CACHE_WRITE |  // NOLINT(misc-redundant-expression)
            INTERNET_FLAG_DONT_CACHE,
            (DWORD_PTR)NULL
        );
        if (hConnect)
        {
            char* exe_buffer = NULL;
            DWORD dwRead = dwSize;
            exe_buffer = calloc(dwSize, sizeof(char));
            if (exe_buffer)
            {
                BOOL bRet = FALSE;
                if ((bRet = InternetReadFile(
                    hConnect,
                    exe_buffer,
                    dwSize - 1,
                    &dwRead
                )))
                {
                    FILE* f = NULL;
                    _wfopen_s(&f, wszPath, L"wb");
                    if (f)
                    {
                        fwrite(exe_buffer, 1, dwRead, f);
                        fclose(f);
                    }
                }
                free(exe_buffer);
            }
            InternetCloseHandle(hConnect);
        }
        InternetCloseHandle(hInternet);
    }
    return bOK;
}

BOOL IsConnectedToInternet(void)
{
    BOOL connectedStatus = FALSE;
    HRESULT hr = S_FALSE;

    hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        INetworkListManager* pNetworkListManager;
        hr = CoCreateInstance(&CLSID_NetworkListManager, NULL, CLSCTX_ALL, &IID_NetworkListManager, (LPVOID*)&pNetworkListManager);
        if (SUCCEEDED(hr))
        {
            NLM_CONNECTIVITY nlmConnectivity = NLM_CONNECTIVITY_DISCONNECTED;
            VARIANT_BOOL isConnected = VARIANT_FALSE;
            hr = pNetworkListManager->lpVtbl->get_IsConnectedToInternet(pNetworkListManager, &isConnected);
            if (SUCCEEDED(hr))
            {
                if (isConnected == VARIANT_TRUE)
                    connectedStatus = TRUE;
                else
                    connectedStatus = FALSE;
            }
            if (isConnected == VARIANT_FALSE && SUCCEEDED(pNetworkListManager->lpVtbl->GetConnectivity(pNetworkListManager, &nlmConnectivity)))
            {
                if (nlmConnectivity & (NLM_CONNECTIVITY_IPV4_LOCALNETWORK | NLM_CONNECTIVITY_IPV4_SUBNET | NLM_CONNECTIVITY_IPV6_LOCALNETWORK | NLM_CONNECTIVITY_IPV6_SUBNET))
                {
                    connectedStatus = 2;
                }
            }
            pNetworkListManager->lpVtbl->Release(pNetworkListManager);
        }
        CoUninitialize();
    }
    return connectedStatus;
}

BOOL DoesOSBuildSupportSpotlight()
{
    return (global_rovi.dwBuildNumber == 22000 && global_ubr >= 706) || (global_rovi.dwBuildNumber >= 22598);
}

BOOL IsSpotlightEnabled()
{
    HKEY hKey = NULL;
    BOOL bRet = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\{2CC5CA98-6485-489A-920E-B3E88A6CCCE3}", 0, KEY_READ, &hKey) == ERROR_SUCCESS;
    if (bRet) RegCloseKey(hKey);
    return bRet;
}

void SpotlightHelper(DWORD dwOp, HWND hWnd, HMENU hMenu, LPPOINT pPt)
{
    static const int spop_insertmenu_ops[] = {SPOP_INSERTMENU_OPEN, SPOP_INSERTMENU_NEXTPIC, 0,
                                              SPOP_INSERTMENU_LIKE, SPOP_INSERTMENU_DISLIKE};

    HRESULT hr = S_OK;
    LPITEMIDLIST pidl = NULL;
    SFGAOF sfgao = 0;
    if (SUCCEEDED(hr = SHParseDisplayName(L"::{2CC5CA98-6485-489A-920E-B3E88A6CCCE3}", NULL, &pidl, 0, &sfgao)))
    {
        IShellFolder* psf = NULL;
        LPCITEMIDLIST pidlChild;
        if (SUCCEEDED(hr = SHBindToParent(pidl, &IID_IShellFolder, (void**)&psf, &pidlChild)))
        {
            IContextMenu* pcm = NULL;
            if (SUCCEEDED(hr = psf->lpVtbl->GetUIObjectOf(psf, hWnd, 1, &pidlChild, &IID_IContextMenu, NULL, &pcm)))
            {
                HMENU hMenu2 = CreatePopupMenu();
                if (hMenu2)
                {
                    if (SUCCEEDED(hr = pcm->lpVtbl->QueryContextMenu(pcm, hMenu2, 0, SCRATCH_QCM_FIRST, SCRATCH_QCM_LAST, CMF_NORMAL)))
                    {
                        if (dwOp == SPOP_OPENMENU)
                        {
                            int iCmd = TrackPopupMenuEx(hMenu2, TPM_RETURNCMD, pPt->x, pPt->y, hWnd, NULL);
                            if (iCmd > 0)
                            {
                                CMINVOKECOMMANDINFOEX info = { 
                                    .cbSize   = sizeof(info),
                                    .fMask    = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE,
                                    .hwnd     = hWnd,
                                    .lpVerb   = MAKEINTRESOURCEA(iCmd - SCRATCH_QCM_FIRST),
                                    .lpVerbW  = MAKEINTRESOURCEW(iCmd - SCRATCH_QCM_FIRST),
                                    .nShow    = SW_SHOWNORMAL,
                                    .ptInvoke = *pPt,
                                };
                                pcm->lpVtbl->InvokeCommand(pcm, &info);
                            }
                        }
                        else if (!(dwOp & ~SPOP_INSERTMENU_ALL))
                        {
                            MENUITEMINFOW mii;
                            int i = ARRAYSIZE(spop_insertmenu_ops) - 1;
                            while (1)
                            {
                                if (i == -1 ? ((dwOp & SPOP_INSERTMENU_INFOTIP1) || (dwOp & SPOP_INSERTMENU_INFOTIP2)) : (dwOp & spop_insertmenu_ops[i]))
                                {
                                    mii.cbSize = sizeof(MENUITEMINFOW);
                                    mii.fMask = MIIM_FTYPE | MIIM_STRING;
                                    mii.cch = 0;
                                    mii.dwTypeData = NULL;
                                    if (i <= 0 ?
                                        (i == 0 ?
                                            !RegQueryValueW(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\{2cc5ca98-6485-489a-920e-b3e88a6ccce3}", NULL, &mii.cch) :
                                            !RegGetValueW(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\{2cc5ca98-6485-489a-920e-b3e88a6ccce3}", L"InfoTip", RRF_RT_REG_SZ, NULL, NULL, &mii.cch)
                                            ) :
                                        GetMenuItemInfoW(hMenu2, i, TRUE, &mii))
                                    {
                                        WCHAR* buf = malloc(++mii.cch * sizeof(WCHAR));
                                        if (buf)
                                        {
                                            mii.dwTypeData = buf;
                                            if (i <= 0 ?
                                                (i == 0 ?
                                                    !RegQueryValueW(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\{2cc5ca98-6485-489a-920e-b3e88a6ccce3}", mii.dwTypeData, &mii.cch) :
                                                    !RegGetValueW(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\{2cc5ca98-6485-489a-920e-b3e88a6ccce3}", L"InfoTip", RRF_RT_REG_SZ, NULL, mii.dwTypeData, &mii.cch)
                                                    ) :
                                                GetMenuItemInfoW(hMenu2, i, TRUE, &mii))
                                            {
                                                if (i == -1)
                                                {
                                                    WCHAR* pCInit = mii.dwTypeData;
                                                    WCHAR* pC = wcschr(mii.dwTypeData, L'\r');
                                                    if (pC)
                                                    {
                                                        pC[0] = 0;

                                                        pC++;
                                                        WCHAR* pC2 = wcschr(pC, L'\r');
                                                        if (pC2)
                                                            pC2[0] = 0;
                                                        mii.dwTypeData = pC;

                                                        mii.fMask = MIIM_ID | MIIM_STRING | MIIM_DATA | MIIM_STATE;
                                                        mii.wID = 3999 + i - 1;
                                                        mii.dwItemData = SPOP_CLICKMENU_FIRST + i - 1;
                                                        mii.fType = MFT_STRING;
                                                        mii.fState = MFS_DISABLED;
                                                        if (dwOp & SPOP_INSERTMENU_INFOTIP2)
                                                            InsertMenuItemW(hMenu, 3, TRUE, &mii);

                                                        mii.dwTypeData = pCInit;
                                                    }
                                                }
                                                mii.fMask = MIIM_ID | MIIM_STRING | MIIM_DATA | (i == -1 ? MIIM_STATE : 0);
                                                mii.wID = 3999 + i;
                                                mii.dwItemData = SPOP_CLICKMENU_FIRST + i;
                                                mii.fType = MFT_STRING;
                                                if (i == -1)
                                                    mii.fState = MFS_DISABLED;
                                                if (i != -1 || (i == -1 && (dwOp & SPOP_INSERTMENU_INFOTIP1)))
                                                    InsertMenuItemW(hMenu, 3, TRUE, &mii);
                                            }
                                            free(buf);
                                        }
                                    }
                                }
                                i--;
                                if (i < -1)
                                    break;
                            }
                            mii.fMask = MIIM_FTYPE | MIIM_DATA;
                            mii.dwItemData = 0;
                            mii.fType = MFT_SEPARATOR;
                            InsertMenuItemW(hMenu, 3, TRUE, &mii);
                        }
                        else if (dwOp >= SPOP_CLICKMENU_FIRST && dwOp <= SPOP_CLICKMENU_LAST)
                        {
                            MENUITEMINFOW mii;
                            mii.cbSize = sizeof(MENUITEMINFOW);
                            mii.fMask = MIIM_ID;
                            if (GetMenuItemInfoW(hMenu2, dwOp - SPOP_CLICKMENU_FIRST, TRUE, &mii))
                            {
                                CMINVOKECOMMANDINFOEX info = { 0 };
                                info.cbSize = sizeof(info);
                                info.fMask = CMIC_MASK_UNICODE;
                                info.hwnd = hWnd;
                                info.lpVerb = MAKEINTRESOURCEA(mii.wID - SCRATCH_QCM_FIRST);
                                info.lpVerbW = MAKEINTRESOURCEW(mii.wID - SCRATCH_QCM_FIRST);
                                info.nShow = SW_SHOWNORMAL;
                                pcm->lpVtbl->InvokeCommand(pcm, &info);
                            }
                        }
                    }
                    DestroyMenu(hMenu2);
                }
                pcm->lpVtbl->Release(pcm);
            }
            psf->lpVtbl->Release(psf);
        }
        CoTaskMemFree(pidl);
    }
}

BOOL ExtractMonitorByIndex(HMONITOR hMonitor, HDC hDC, LPRECT lpRect, MonitorOverrideData* mod)
{
    POINT pt; pt.x = 0; pt.y = 0;
    if (MonitorFromPoint(pt, MONITOR_DEFAULTTONULL) == hMonitor)
        return TRUE;
    if (mod->cbIndex == mod->dwIndex)
    {
        mod->hMonitor = hMonitor;
        return FALSE;
    }
    mod->cbIndex++;
    return TRUE;
}

RM_UNIQUE_PROCESS GetExplorerApplication(void)
{
    // https://jiangsheng.net/2013/01/22/how-to-restart-windows-explorer-programmatically-using-restart-manager/

    HWND  hwnd = FindWindow(L"Shell_TrayWnd", NULL);
    DWORD pid  = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    RM_UNIQUE_PROCESS out = { 0, { -1, -1 } };
    DWORD             bytesReturned;
    DWORD             processIds[2048]; // max 2048 processes (more than enough)

    // enumerate all running processes (usually around 60-70)
    EnumProcesses(processIds, sizeof(processIds), &bytesReturned);
    int count = bytesReturned / sizeof(DWORD); // number of processIds returned

    for (int i = 0; i < count; ++i)
    {
        DWORD  processId = processIds[i];
        HANDLE hProc;
        if (processId == pid && ((hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId))))
        {
            WCHAR imageName[MAX_PATH];
            GetProcessImageFileNameW(hProc, imageName, MAX_PATH);
            FILETIME ftStart, ftExit, ftKernel, ftUser;
            GetProcessTimes(hProc, &ftStart, &ftExit, &ftKernel, &ftUser);

            if (ftStart.dwLowDateTime < out.ProcessStartTime.dwLowDateTime)
            {
                out.dwProcessId      = processId;
                out.ProcessStartTime = ftStart;
            }
            CloseHandle(hProc);
        }
    }
    return out; // return count in pResults
}

static DWORD RmSession = -1;
static wchar_t RmSessionKey[CCH_RM_SESSION_KEY + 1];

void BeginExplorerRestart(void)
{
    if (RmStartSession(&RmSession, 0, RmSessionKey) == ERROR_SUCCESS)
    {
        RM_UNIQUE_PROCESS rgApplications[] = { GetExplorerApplication() };
        RmRegisterResources(RmSession, 0, NULL, 1, rgApplications, 0, NULL);

        DWORD           rebootReason;
        UINT            nProcInfoNeeded, nProcInfo = 16;
        RM_PROCESS_INFO affectedApps[16];
        RmGetList(RmSession, &nProcInfoNeeded, &nProcInfo, affectedApps, &rebootReason);

        if (rebootReason == RmRebootReasonNone) // no need for reboot?
        {
            // shutdown explorer
            RmShutdown(RmSession, RmForceShutdown, 0);
        }
    }
}

void FinishExplorerRestart(void)
{
    DWORD dwError;
    if ((dwError = RmRestart(RmSession, 0, NULL)))
        printf("\nRmRestart error: 0x%lX\n\n", dwError);

    RmEndSession(RmSession);
    RmSession       = -1;
    RmSessionKey[0] = 0;
}

BOOL ExitExplorer(void)
{
    // https://stackoverflow.com/questions/5689904/gracefully-exit-explorer-programmatically

    HWND hWndTray = FindWindowW(L"Shell_TrayWnd", NULL);
    return PostMessageW(hWndTray, 0x5B4, 0, 0);
}

void StartExplorerWithDelay(int delay, HANDLE userToken)
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
            startupInfo.cb      = sizeof(startupInfo);
            BOOL processCreated = CreateProcessWithTokenW(
                primaryUserToken, LOGON_WITH_PROFILE, wszPath, NULL, 0, NULL, NULL, &startupInfo, &processInfo) != 0;
            CloseHandle(primaryUserToken);
            if (processInfo.hProcess != INVALID_HANDLE_VALUE)
                CloseHandle(processInfo.hProcess);
            if (processInfo.hThread != INVALID_HANDLE_VALUE)
                CloseHandle(processInfo.hThread);
            if (processCreated)
                return;
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

BOOL StartExplorer(void)
{
#if 0
    PROCESSENTRY32 pe32 = {0};
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS,
        0
    );
    if (Process32First(hSnapshot, &pe32) == TRUE)
    {
        do
        {
            if (!wcscmp(pe32.szExeFile, L"explorer.exe"))
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
#endif
#define EXPLORER_EXECUTABLE L"\\explorer.exe"

    // Three cheers for unhelpful micro-optimizations!
    wchar_t wszPath[MAX_PATH];
    UINT    lendir = GetWindowsDirectoryW(wszPath, MAX_PATH);
    memcpy_s(wszPath + lendir, (MAX_PATH - lendir) * sizeof(wchar_t),
             EXPLORER_EXECUTABLE, sizeof(EXPLORER_EXECUTABLE));

    PROCESS_INFORMATION pi;
    STARTUPINFO         si;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(si);

    BOOL const b = CreateProcessW(
        NULL, wszPath, NULL, NULL, TRUE,
        CREATE_UNICODE_ENVIRONMENT,
        NULL, NULL, &si, &pi);
    if (b) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    return b;

#undef EXPLORER_EXECUTABLE
}

BOOL IncrementDLLReferenceCount(HINSTANCE hinst)
{
    HMODULE hMod;
    return GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCWSTR)hinst,
        &hMod);
}

BOOL PatchContextMenuOfNewMicrosoftIME(BOOL* bFound)
{
    // huge thanks to @Simplestas: https://github.com/valinet/ExplorerPatcher/issues/598
    const DWORD patch_from = 0x50653844u;
    const DWORD patch_to   = 0x54653844u; // cmp byte ptr [rbp+50h], r12b

    if (bFound)
        *bFound = FALSE;

    HMODULE hInputSwitch = NULL;
    if (!GetModuleHandleExW(0, L"InputSwitch.dll", &hInputSwitch))
        return FALSE;
    PIMAGE_DOS_HEADER     dosHeader      = (PIMAGE_DOS_HEADER)hInputSwitch;
    PIMAGE_NT_HEADERS     pNTHeader      = (PIMAGE_NT_HEADERS)((DWORD_PTR)dosHeader + dosHeader->e_lfanew);
    PIMAGE_SECTION_HEADER pSectionHeader = (PIMAGE_SECTION_HEADER)(pNTHeader + 1);

    char * mod = 0;
    int    i;
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
        return FALSE;

    for (size_t off = 0; off < pSectionHeader[i].Misc.VirtualSize - sizeof(DWORD); ++off)
    {
        DWORD* ptr = (DWORD*)(mod + off);
        if (*ptr == patch_from) {
            if (bFound)
                *bFound = TRUE;
            DWORD prot;
            if (VirtualProtect(ptr, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &prot)) {
                *ptr = patch_to;
                VirtualProtect(ptr, sizeof(DWORD), prot, &prot);
            }
            break;
        }
    }
    return TRUE;
}
