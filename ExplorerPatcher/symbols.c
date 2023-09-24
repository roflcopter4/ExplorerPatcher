#include "symbols.h"
#include "Common/Common.h"

#include <valinet/pdb/pdb.h>


const char* twinui_pcshell_SN[TWINUI_PCSHELL_SB_CNT] = {
    TWINUI_PCSHELL_SB_0,
    TWINUI_PCSHELL_SB_1,
    TWINUI_PCSHELL_SB_2,
    TWINUI_PCSHELL_SB_3,
    TWINUI_PCSHELL_SB_4,
    TWINUI_PCSHELL_SB_5,
    TWINUI_PCSHELL_SB_6,
    TWINUI_PCSHELL_SB_7,
    TWINUI_PCSHELL_SB_8
};
const char* startdocked_SN[STARTDOCKED_SB_CNT] = {
    STARTDOCKED_SB_0,
    STARTDOCKED_SB_1,
    STARTDOCKED_SB_2,
    STARTDOCKED_SB_3,
    STARTDOCKED_SB_4
};
const char* startui_SN[STARTUI_SB_CNT] = {
    STARTUI_SB_0
};

const wchar_t DownloadSymbolsXML[] =
L"<toast scenario=\"reminder\" "
L"activationType=\"protocol\" launch=\"https://github.com/valinet/ExplorerPatcher\" duration=\"short\">\r\n"
L"	<visual>\r\n"
L"		<binding template=\"ToastGeneric\">\r\n"
L"			<text><![CDATA[Unable to find symbols for OS build %s]]></text>\r\n"
L"			<text><![CDATA[Downloading and applying symbol information, please wait...]]></text>\r\n"
L"			<text placement=\"attribution\"><![CDATA[ExplorerPatcher]]></text>\r\n"
L"		</binding>\r\n"
L"	</visual>\r\n"
L"	<audio src=\"ms-winsoundevent:Notification.Default\" loop=\"false\" silent=\"false\"/>\r\n"
L"</toast>\r\n";

const wchar_t DownloadOKXML[] =
L"<toast scenario=\"reminder\" "
L"activationType=\"protocol\" launch=\"https://github.com/valinet/ExplorerPatcher\" duration=\"long\">\r\n"
L"	<visual>\r\n"
L"		<binding template=\"ToastGeneric\">\r\n"
L"			<text><![CDATA[Successfully downloaded symbols for OS build %s]]></text>\r\n"
L"			<text><![CDATA[Please restart File Explorer to apply the changes and enable additional functionality.]]></text>\r\n"
L"			<text placement=\"attribution\"><![CDATA[ExplorerPatcher]]></text>\r\n"
L"		</binding>\r\n"
L"	</visual>\r\n"
L"	<audio src=\"ms-winsoundevent:Notification.Default\" loop=\"false\" silent=\"false\"/>\r\n"
L"</toast>\r\n";

const wchar_t InstallOK[] =
L"<toast scenario=\"reminder\" "
L"activationType=\"protocol\" launch=\"https://github.com/valinet/ExplorerPatcher\" duration=\"long\">\r\n"
L"	<visual>\r\n"
L"		<binding template=\"ToastGeneric\">\r\n"
L"			<text><![CDATA[Installation succeeded!]]></text>\r\n"
L"			<text><![CDATA[This notification will not show again until the next OS build update.]]></text>\r\n"
L"			<text placement=\"attribution\"><![CDATA[ExplorerPatcher]]></text>\r\n"
L"		</binding>\r\n"
L"	</visual>\r\n"
L"	<audio src=\"ms-winsoundevent:Notification.Default\" loop=\"false\" silent=\"false\"/>\r\n"
L"</toast>\r\n";


static void string_to_lowercase(char *str)
{
    for (size_t i = 0, len = strlen(str); i < len; ++i)
        str[i] = (char)tolower(str[i]);
}


DWORD DownloadSymbols(DownloadSymbolsParams* params)
{
    RTL_OSVERSIONINFOW rovi;
    char    hash[128];
    WCHAR   wszPath[MAX_PATH];
    DWORD   dwDisposition;
    HKEY    hKey    = NULL;
    HMODULE hModule = params->hModule;
    DWORD32 ubr     = VnGetOSVersionAndUBR(&rovi);

    Sleep(6000);
    wprintf(L"[Symbols] Started \"Download symbols\" thread.\n");

    WCHAR szReportedVersion[MAX_PATH] = {0};
    swprintf_s(
        szReportedVersion, _countof(szReportedVersion),
        L"%ld.%ld.%ld.%ld",
        rovi.dwMajorVersion, rovi.dwMinorVersion, rovi.dwBuildNumber, ubr);

    WCHAR buffer[1000];
    memset(buffer, 0, 1000);
    swprintf_s(buffer, _countof(buffer), DownloadSymbolsXML, szReportedVersion);

    if (params->bVerbose)
    {
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
    }
    wprintf(
        L"[Symbols] "
        L"Attempting to download symbols for unknown OS version %s.\n",
        szReportedVersion
    );


    char szSettingsPath[MAX_PATH];
    ZeroMemory(
        szSettingsPath,
        MAX_PATH * sizeof(char)
    );
    SHGetFolderPathA(
        NULL,
        SPECIAL_FOLDER_LEGACY,
        NULL,
        SHGFP_TYPE_CURRENT,
        szSettingsPath
    );
    strcat_s(
        szSettingsPath,
        MAX_PATH,
        APP_RELATIVE_PATH
    );
    CreateDirectoryA(szSettingsPath, NULL);
    strcat_s(
        szSettingsPath,
        MAX_PATH,
        "\\"
    );
    wprintf(L"[Symbols] Downloading to \"%hs\".\n", szSettingsPath);

    symbols_addr symbols_PTRS = {0};
    char twinui_pcshell_sb_dll[MAX_PATH] = {0};
    memset(hash, 0, sizeof hash);
    memset(wszPath, 0, sizeof wszPath);

    GetSystemDirectoryA(twinui_pcshell_sb_dll, MAX_PATH);
    strcat_s(twinui_pcshell_sb_dll, MAX_PATH, "\\");
    strcat_s(twinui_pcshell_sb_dll, MAX_PATH, TWINUI_PCSHELL_SB_NAME);
    strcat_s(twinui_pcshell_sb_dll, MAX_PATH, ".dll");
    RegCreateKeyExW(HKEY_CURRENT_USER, REGPATH L"\\" TWINUI_PCSHELL_SB_NAME,
                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition);

    if (!hKey || hKey == INVALID_HANDLE_VALUE) {
        if (params->bVerbose)
            FreeLibraryAndExitThread(hModule, 9);
        return 9;
    }

    GetSystemDirectoryW(wszPath, MAX_PATH);
    wcscat_s(wszPath, MAX_PATH, L"\\" _T(TWINUI_PCSHELL_SB_NAME) L".dll");
    ExplorerPatcher_ComputeFileHash(wszPath, hash, 100);
    wprintf(L"[Symbols] Downloading symbols for \"%hs\" (\"%hs\")...\n", twinui_pcshell_sb_dll, hash);

    if (VnDownloadSymbols(
        NULL,
        twinui_pcshell_sb_dll,
        szSettingsPath,
        MAX_PATH
    ))
    {
        wprintf(L"[Symbols] Symbols for \"%hs\" are not available - unable to download.\n"
                L"[Symbols] Please refer to \"https://github.com/valinet/ExplorerPatcher/wiki/Symbols\" for more information.\n",
                twinui_pcshell_sb_dll);
        if (params->bVerbose)
        {
            FreeLibraryAndExitThread(hModule, 4);
        }
        return 4;
    }
    wprintf(L"[Symbols] Reading symbols...\n");
    if (!IsWindows11())
    {
        DWORD flOldProtect = 0;
        if (VirtualProtect(twinui_pcshell_SN, sizeof(twinui_pcshell_SN), PAGE_EXECUTE_READWRITE, &flOldProtect))
        {
            twinui_pcshell_SN[1] = twinui_pcshell_SN[0];
            VirtualProtect(twinui_pcshell_SN, sizeof(twinui_pcshell_SN), flOldProtect, &flOldProtect);
        }
    }
    if (IsWindows11Version22H2OrHigher())
    {
        DWORD flOldProtect = 0;
        if (VirtualProtect(twinui_pcshell_SN, sizeof(twinui_pcshell_SN), PAGE_EXECUTE_READWRITE, &flOldProtect))
        {
            twinui_pcshell_SN[7] = "CMultitaskingViewManager::_CreateXamlMTVHost";
            twinui_pcshell_SN[TWINUI_PCSHELL_SB_CNT - 1] = "CMultitaskingViewManager::_CreateDCompMTVHost";
            VirtualProtect(twinui_pcshell_SN, sizeof(twinui_pcshell_SN), flOldProtect, &flOldProtect);
        }
    }
    if (VnGetSymbols(
        szSettingsPath,
        symbols_PTRS.twinui_pcshell_PTRS,
        twinui_pcshell_SN,
        IsWindows11() ? TWINUI_PCSHELL_SB_CNT : 4
    ))
    {
        if (IsWindows11())
        {
            //wprintf(L"[Symbols] Hooking Win+C is not available in this build.\n");
            DWORD dwZero = 0;
            RegSetValueExW(hKey, L"" TWINUI_PCSHELL_SB_8, 0, REG_DWORD, &dwZero, sizeof(DWORD));
            if (VnGetSymbols(
                szSettingsPath,
                symbols_PTRS.twinui_pcshell_PTRS,
                twinui_pcshell_SN,
                TWINUI_PCSHELL_SB_CNT - 1
            ))
            {
                wprintf(L"[Symbols] Windows 10 window switcher style is not available in this build.\n");
                DWORD dwZero = 0;
                RegSetValueExW(
                    hKey,
                    L"" TWINUI_PCSHELL_SB_7,
                    0,
                    REG_DWORD,
                    &dwZero,
                    sizeof(DWORD)
                );
                if (VnGetSymbols(
                    szSettingsPath,
                    symbols_PTRS.twinui_pcshell_PTRS,
                    twinui_pcshell_SN,
                    TWINUI_PCSHELL_SB_CNT - 2
                ))
                {
                    wprintf(L"[Symbols] Failure in reading symbols for \"%hs\".\n", twinui_pcshell_sb_dll);
                    if (params->bVerbose)
                    {
                        FreeLibraryAndExitThread(
                            hModule,
                            5
                        );
                    }
                    return 5;
                }
            }
        }
        else
        {
            wprintf(L"[Symbols] Failure in reading symbols for \"%hs\".\n", twinui_pcshell_sb_dll);
            if (params->bVerbose)
            {
                FreeLibraryAndExitThread(
                    hModule,
                    5
                );
            }
            return 5;
        }
    }

    if (!IsWindows11())
        symbols_PTRS.twinui_pcshell_PTRS[1] = 0;

    RegSetValueExW(hKey, L"" TWINUI_PCSHELL_SB_0, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.twinui_pcshell_PTRS[0], sizeof(DWORD));
    RegSetValueExW(hKey, L"" TWINUI_PCSHELL_SB_1, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.twinui_pcshell_PTRS[1], sizeof(DWORD));
    RegSetValueExW(hKey, L"" TWINUI_PCSHELL_SB_2, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.twinui_pcshell_PTRS[2], sizeof(DWORD));
    RegSetValueExW(hKey, L"" TWINUI_PCSHELL_SB_3, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.twinui_pcshell_PTRS[3], sizeof(DWORD));
    RegSetValueExW(hKey, L"" TWINUI_PCSHELL_SB_4, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.twinui_pcshell_PTRS[4], sizeof(DWORD));
    RegSetValueExW(hKey, L"" TWINUI_PCSHELL_SB_5, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.twinui_pcshell_PTRS[5], sizeof(DWORD));
    RegSetValueExW(hKey, L"" TWINUI_PCSHELL_SB_6, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.twinui_pcshell_PTRS[6], sizeof(DWORD));
    RegSetValueExW(hKey, L"" TWINUI_PCSHELL_SB_7, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.twinui_pcshell_PTRS[7], sizeof(DWORD));
    RegSetValueExW(hKey, L"" TWINUI_PCSHELL_SB_8, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.twinui_pcshell_PTRS[8], sizeof(DWORD));

    if (hKey)
        RegCloseKey(hKey);

    if (IsWindows11()) {
        char startdocked_sb_dll[MAX_PATH] = {0};
        memset(hash, 0, sizeof hash);
        memset(wszPath, 0, sizeof wszPath);

        GetWindowsDirectoryA(startdocked_sb_dll, MAX_PATH);
        strcat_s(startdocked_sb_dll, MAX_PATH, "\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\");
        strcat_s(startdocked_sb_dll, MAX_PATH, STARTDOCKED_SB_NAME);
        strcat_s(startdocked_sb_dll, MAX_PATH, ".dll");
        GetWindowsDirectoryW(wszPath, MAX_PATH);
        wcscat_s(wszPath, _countof(wszPath), L"\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\" _T(STARTDOCKED_SB_NAME) L".dll");
        ExplorerPatcher_ComputeFileHash(wszPath, hash, 100);
        wprintf(L"[Symbols] Downloading symbols for \"%hs\" (\"%hs\")...\n", startdocked_sb_dll, hash);

        if (VnDownloadSymbols(NULL, startdocked_sb_dll, szSettingsPath, MAX_PATH)) {
            wprintf(L"[Symbols] Symbols for \"%hs\" are not available - unable to download.\n"
                    L"[Symbols] Please refer to \"https://github.com/valinet/ExplorerPatcher/wiki/Symbols\" for more information.\n",
                    startdocked_sb_dll);
            if (params->bVerbose)
                FreeLibraryAndExitThread(hModule, 6);
            return 6;
        }
        wprintf(L"[Symbols] Reading symbols...\n");
        if (VnGetSymbols(szSettingsPath, symbols_PTRS.startdocked_PTRS, startdocked_SN, STARTDOCKED_SB_CNT)) {
            wprintf(L"[Symbols] Failure in reading symbols for \"%hs\".\n", startdocked_sb_dll);
            if (params->bVerbose)
                FreeLibraryAndExitThread(hModule, 7);
            return 7;
        }

        RegCreateKeyExW(HKEY_CURRENT_USER, REGPATH_STARTMENU L"\\" STARTDOCKED_SB_NAME,
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition);
        if (!hKey || hKey == INVALID_HANDLE_VALUE) {
            if (params->bVerbose)
                FreeLibraryAndExitThread(hModule, 8);
            return 8;
        }

        RegSetValueExW(hKey, L"" STARTDOCKED_SB_0, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.startdocked_PTRS[0], sizeof(DWORD));
        RegSetValueExW(hKey, L"" STARTDOCKED_SB_1, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.startdocked_PTRS[1], sizeof(DWORD));
        RegSetValueExW(hKey, L"" STARTDOCKED_SB_2, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.startdocked_PTRS[2], sizeof(DWORD));
        RegSetValueExW(hKey, L"" STARTDOCKED_SB_3, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.startdocked_PTRS[3], sizeof(DWORD));
        RegSetValueExW(hKey, L"" STARTDOCKED_SB_4, 0, REG_DWORD, (BYTE const *)&symbols_PTRS.startdocked_PTRS[4], sizeof(DWORD));

        if (hKey)
            RegCloseKey(hKey);
    }

    if (rovi.dwBuildNumber >= 18362) {
        char startui_sb_dll[MAX_PATH] = {0};
        memset(hash, 0, sizeof hash);
        memset(wszPath, 0, sizeof wszPath);

        GetWindowsDirectoryA(startui_sb_dll, MAX_PATH);
        strcat_s(startui_sb_dll, MAX_PATH, "\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\");
        strcat_s(startui_sb_dll, MAX_PATH, STARTUI_SB_NAME);
        strcat_s(startui_sb_dll, MAX_PATH, ".dll");
        GetWindowsDirectoryW(wszPath, MAX_PATH);
        wcscat_s(wszPath, MAX_PATH, L"\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\" _T(STARTUI_SB_NAME) L".dll");
        ExplorerPatcher_ComputeFileHash(wszPath, hash, 100);
        wprintf(L"[Symbols] Downloading symbols for \"%hs\" (\"%hs\")...\n", startui_sb_dll, hash);

        if (VnDownloadSymbols(
            NULL,
            startui_sb_dll,
            szSettingsPath,
            MAX_PATH
        ))
        {
            wprintf(L"[Symbols] Symbols for \"%hs\" are not available - unable to download.\n"
                    L"[Symbols] Please refer to \"https://github.com/valinet/ExplorerPatcher/wiki/Symbols\" for more information.\n",
                    startui_sb_dll);
            if (params->bVerbose)
            {
                FreeLibraryAndExitThread(
                    hModule,
                    6
                );
            }
            return 6;
        }
        wprintf(L"[Symbols] Reading symbols...\n");
        if (VnGetSymbols(
            szSettingsPath,
            symbols_PTRS.startui_PTRS,
            startui_SN,
            STARTUI_SB_CNT
        ))
        {
            wprintf(L"[Symbols] Failure in reading symbols for \"%hs\".\n", startui_sb_dll);
            if (params->bVerbose)
            {
                FreeLibraryAndExitThread(
                    hModule,
                    7
                );
            }
            return 7;
        }
        RegCreateKeyExW(
            HKEY_CURRENT_USER,
            L"" REGPATH_STARTMENU L"\\" L"" STARTUI_SB_NAME,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_WRITE,
            NULL,
            &hKey,
            &dwDisposition
        );
        if (!hKey || hKey == INVALID_HANDLE_VALUE)
        {
            if (params->bVerbose)
            {
                FreeLibraryAndExitThread(
                    hModule,
                    8
                );
            }
            return 8;
        }
        RegSetValueExW(
            hKey,
            L"" STARTUI_SB_0,
            0,
            REG_DWORD,
            &(symbols_PTRS.startui_PTRS[0]),
            sizeof(DWORD)
        );
        if (hKey) RegCloseKey(hKey);
    }







    RegCreateKeyExW(
        HKEY_CURRENT_USER,
        L"" REGPATH,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &hKey,
        &dwDisposition
    );
    if (!hKey || hKey == INVALID_HANDLE_VALUE)
    {
        if (params->bVerbose)
        {
            FreeLibraryAndExitThread(
                hModule,
                10
            );
        }
        return 10;
    }
    RegSetValueExW(
        hKey,
        TEXT("OSBuild"),
        0,
        REG_SZ,
        szReportedVersion,
        wcslen(szReportedVersion) * sizeof(WCHAR)
    );
    if (hKey) RegCloseKey(hKey);


    wprintf(L"[Symbols] Finished gathering symbol data.\n");

    if (params->bVerbose)
    {
        __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocument* inputXml = NULL;
        HRESULT hr = String2IXMLDocument(
            InstallOK,
            wcslen(InstallOK),
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
        Sleep(4000);
        exit(0);
    }
    else
    {
        swprintf_s(buffer, _countof(buffer), DownloadOKXML, szReportedVersion);
        __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocument *inputXml2 = NULL;
        HRESULT hr = String2IXMLDocument(
            buffer,
            wcslen(buffer),
            &inputXml2,
#ifdef DEBUG
            stdout
#else
            NULL
#endif
        );
        hr = ShowToastMessage(
            inputXml2,
            APPID,
            sizeof(APPID) / sizeof(WCHAR) - 1,
#ifdef DEBUG
            stdout
#else
            NULL
#endif
        );
    }

    wprintf(L"[Symbols] Finished \"Download symbols\" thread.\n");
    return 0;
}


BOOL LoadSymbols(symbols_addr* symbols_PTRS, HMODULE hModule)
{
    RTL_OSVERSIONINFOW rovi;
    DWORD   dwDisposition;
    DWORD   dwSize = sizeof(DWORD);
    HKEY    hKey   = NULL;
    DWORD32 ubr    = VnGetOSVersionAndUBR(&rovi);
    WCHAR szStoredVersion[MAX_PATH] = {0};
    WCHAR szReportedVersion[MAX_PATH];

    swprintf_s(
        szReportedVersion, _countof(szReportedVersion),
        L"%ld.%ld.%ld.%ld",
        rovi.dwMajorVersion, rovi.dwMinorVersion,
        rovi.dwBuildNumber, ubr
    );

    BOOL  bIsStartHardcoded         = FALSE;
    BOOL  bIsTwinuiPcshellHardcoded = FALSE;
    WCHAR wszPath[MAX_PATH];
    CHAR  hash[128] = {0};

    GetSystemDirectoryW(wszPath, _countof(wszPath));
    wcscat_s(wszPath, _countof(wszPath), L"\\" TWINUI_PCSHELL_SB_NAME L".dll");

    ExplorerPatcher_ComputeFileHash(wszPath, hash, _countof(hash));
    string_to_lowercase(hash);

    if (StrEq(hash, "8b23b02962856e89b8d8a3956de1d76c")) // 282, 318
    {
        symbols_PTRS->twinui_pcshell_PTRS[0] = 0x217CE6;
        symbols_PTRS->twinui_pcshell_PTRS[1] = 0x5CC570;
        symbols_PTRS->twinui_pcshell_PTRS[2] = 0x5F5E88;
        symbols_PTRS->twinui_pcshell_PTRS[3] = 0x5F6690;
        symbols_PTRS->twinui_pcshell_PTRS[4] = 0x5DAC08;
        symbols_PTRS->twinui_pcshell_PTRS[5] = 0x5DA8C4;
        symbols_PTRS->twinui_pcshell_PTRS[6] = 0x5CD9C0;
        symbols_PTRS->twinui_pcshell_PTRS[7] = 0x5f744c;
        symbols_PTRS->twinui_pcshell_PTRS[8] = 0x52980;
        bIsTwinuiPcshellHardcoded = TRUE;
    } else if (StrEq(hash, "03487ccd5bc5a194fad61b616b0a2b28") ||
               StrEq(hash, "3f6ef12a59a2f84a3296771ea7753e01")) // 346, 348, 376, 434, 438
    {
        symbols_PTRS->twinui_pcshell_PTRS[0] = 0x21B036;
        symbols_PTRS->twinui_pcshell_PTRS[1] = 0x5CD740;
        symbols_PTRS->twinui_pcshell_PTRS[2] = 0x5F7058;
        symbols_PTRS->twinui_pcshell_PTRS[3] = 0x5F7860;
        symbols_PTRS->twinui_pcshell_PTRS[4] = 0x5DBDD8;
        symbols_PTRS->twinui_pcshell_PTRS[5] = 0x5DBA94;
        symbols_PTRS->twinui_pcshell_PTRS[6] = 0x5CEB90;
        symbols_PTRS->twinui_pcshell_PTRS[7] = 0x5f861c;
        symbols_PTRS->twinui_pcshell_PTRS[8] = 0x4D780;
        bIsTwinuiPcshellHardcoded = TRUE;
    } else if (StrEq(hash, "6399b5913a7048c4422e3cfb03860da2") ||
               StrEq(hash, "99dea5939a2b1945b2d3fd65433ca401")) // 466, 469
    {
        symbols_PTRS->twinui_pcshell_PTRS[0] = 0x229fa6;
        symbols_PTRS->twinui_pcshell_PTRS[1] = 0x5dc500;
        symbols_PTRS->twinui_pcshell_PTRS[2] = 0x5fa868;
        symbols_PTRS->twinui_pcshell_PTRS[3] = 0x5fb070;
        symbols_PTRS->twinui_pcshell_PTRS[4] = 0x5df640;
        symbols_PTRS->twinui_pcshell_PTRS[5] = 0x5df2f4;
        symbols_PTRS->twinui_pcshell_PTRS[6] = 0x5dd910;
        symbols_PTRS->twinui_pcshell_PTRS[7] = 0x5fbe2c;
        symbols_PTRS->twinui_pcshell_PTRS[8] = 0x5dd910;
        bIsTwinuiPcshellHardcoded = TRUE;
    } else if (StrEq(hash, "5cd249a3b9cc1f1a6c0e9e699fb8ab74")) // 527
    {
        symbols_PTRS->twinui_pcshell_PTRS[0] = 0x22b3b6;
        symbols_PTRS->twinui_pcshell_PTRS[1] = 0x5ddaf0;
        symbols_PTRS->twinui_pcshell_PTRS[2] = 0x5fbc08;
        symbols_PTRS->twinui_pcshell_PTRS[3] = 0x5fc410;
        symbols_PTRS->twinui_pcshell_PTRS[4] = 0x5e0c30;
        symbols_PTRS->twinui_pcshell_PTRS[5] = 0x5e08e4;
        symbols_PTRS->twinui_pcshell_PTRS[6] = 0x5def00;
        symbols_PTRS->twinui_pcshell_PTRS[7] = 0x5fd1cc;
        symbols_PTRS->twinui_pcshell_PTRS[8] = 0x4da10;
        bIsTwinuiPcshellHardcoded = TRUE;
    } else if (StrEq(hash, "2466e0f424dcdc3498ce0236f0911554")) // 556
    {
        symbols_PTRS->twinui_pcshell_PTRS[0] = 0x22b776;
        symbols_PTRS->twinui_pcshell_PTRS[1] = 0x5ddeb0;
        symbols_PTRS->twinui_pcshell_PTRS[2] = 0x5fbfc8;
        symbols_PTRS->twinui_pcshell_PTRS[3] = 0x5fc7d0;
        symbols_PTRS->twinui_pcshell_PTRS[4] = 0x5e0ff0;
        symbols_PTRS->twinui_pcshell_PTRS[5] = 0x5e0ca4;
        symbols_PTRS->twinui_pcshell_PTRS[6] = 0x5df2c0;
        symbols_PTRS->twinui_pcshell_PTRS[7] = 0x5fd58c;
        symbols_PTRS->twinui_pcshell_PTRS[8] = 0x4da10;
        bIsTwinuiPcshellHardcoded = TRUE;
    } else if (StrEq(hash, "068b6012bc825f178d3418870422871b")) // 613
    {
        symbols_PTRS->twinui_pcshell_PTRS[0] = 0x227696;
        symbols_PTRS->twinui_pcshell_PTRS[1] = 0x5cd590;
        symbols_PTRS->twinui_pcshell_PTRS[2] = 0x5eb6d8;
        symbols_PTRS->twinui_pcshell_PTRS[3] = 0x5ebee0;
        symbols_PTRS->twinui_pcshell_PTRS[4] = 0x5d06d0;
        symbols_PTRS->twinui_pcshell_PTRS[5] = 0x5d0384;
        symbols_PTRS->twinui_pcshell_PTRS[6] = 0x5ce9a0;
        symbols_PTRS->twinui_pcshell_PTRS[7] = 0x5ecc9c;
        symbols_PTRS->twinui_pcshell_PTRS[8] = 0x3bc70;
        bIsTwinuiPcshellHardcoded = TRUE;
    } else if (StrEq(hash, "18ae53a66cb941f5bba8411a8f245e0c")) // 675
    {
        symbols_PTRS->twinui_pcshell_PTRS[0] = 0x227fa6;
        symbols_PTRS->twinui_pcshell_PTRS[1] = 0x5cd4b0;
        symbols_PTRS->twinui_pcshell_PTRS[2] = 0x5eb5f8;
        symbols_PTRS->twinui_pcshell_PTRS[3] = 0x5ebe00;
        symbols_PTRS->twinui_pcshell_PTRS[4] = 0x5d05f0;
        symbols_PTRS->twinui_pcshell_PTRS[5] = 0x5d02a4;
        symbols_PTRS->twinui_pcshell_PTRS[6] = 0x5ce8c0;
        symbols_PTRS->twinui_pcshell_PTRS[7] = 0x5ecbbc;
        symbols_PTRS->twinui_pcshell_PTRS[8] = 0x4a7e0;
        bIsTwinuiPcshellHardcoded = TRUE;
    } else if (StrEq(hash, "b6d42f3599df7caf5b0da775e725d963")) // 708
    {
        symbols_PTRS->twinui_pcshell_PTRS[0] = 0x227cb6;
        symbols_PTRS->twinui_pcshell_PTRS[1] = 0x5cd600;
        symbols_PTRS->twinui_pcshell_PTRS[2] = 0x5eb748;
        symbols_PTRS->twinui_pcshell_PTRS[3] = 0x5ebf50;
        symbols_PTRS->twinui_pcshell_PTRS[4] = 0x5d0740;
        symbols_PTRS->twinui_pcshell_PTRS[5] = 0x5d03f4;
        symbols_PTRS->twinui_pcshell_PTRS[6] = 0x5cea10;
        symbols_PTRS->twinui_pcshell_PTRS[7] = 0x5ecd0c;
        symbols_PTRS->twinui_pcshell_PTRS[8] = 0x4a7b0;
        bIsTwinuiPcshellHardcoded = TRUE;
    } else if (StrEq(hash, "93dbd7bfcb21d2449bc0eb10d6e3f6ab")) // 778
    {
        symbols_PTRS->twinui_pcshell_PTRS[0] = 0x2291b6;
        symbols_PTRS->twinui_pcshell_PTRS[1] = 0x5ce700;
        symbols_PTRS->twinui_pcshell_PTRS[2] = 0x5ec688;
        symbols_PTRS->twinui_pcshell_PTRS[3] = 0x5ece90;
        symbols_PTRS->twinui_pcshell_PTRS[4] = 0x5d1684;
        symbols_PTRS->twinui_pcshell_PTRS[5] = 0x5d1338;
        symbols_PTRS->twinui_pcshell_PTRS[6] = 0x5cf890;
        symbols_PTRS->twinui_pcshell_PTRS[7] = 0x5edc4c;
        symbols_PTRS->twinui_pcshell_PTRS[8] = 0x49de0;
        bIsTwinuiPcshellHardcoded = TRUE;
    }

    if (bIsTwinuiPcshellHardcoded)
        wprintf(L"[Symbols] Identified known \"" TWINUI_PCSHELL_SB_NAME ".dll\" with hash %s.\n", hash);

    GetWindowsDirectoryW(wszPath, MAX_PATH);
    wcscat_s(wszPath, MAX_PATH, L"\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\" STARTDOCKED_SB_NAME L".dll");
    ExplorerPatcher_ComputeFileHash(wszPath, hash, _countof(hash));
    string_to_lowercase(hash);

    if (StrEq(hash, "b57bb94a48d2422de9a78c5fcba28f98")) // 282, 318
    {
        symbols_PTRS->startdocked_PTRS[0] = 0x188EBC;
        symbols_PTRS->startdocked_PTRS[1] = 0x188EBC;
        symbols_PTRS->startdocked_PTRS[2] = 0x187120;
        symbols_PTRS->startdocked_PTRS[3] = 0x3C10;
        symbols_PTRS->startdocked_PTRS[4] = 0x160AEC;
        bIsStartHardcoded = TRUE;
    } else if (StrEq(hash, "e9c1c45a659dafabf671cb0ae195f8d9") ||
               StrEq(hash, "7e652d78661ba62e33d41ad1d3180344") ||
               StrEq(hash, "72c07045d99ec3bf2cf4479aa324281a")) // 346, 348, 376, 434, 438, 466, 527, 556
    {
        symbols_PTRS->startdocked_PTRS[0] = 0x18969C;
        symbols_PTRS->startdocked_PTRS[1] = 0x18969C;
        symbols_PTRS->startdocked_PTRS[2] = 0x187900;
        symbols_PTRS->startdocked_PTRS[3] = 0x3C00;
        symbols_PTRS->startdocked_PTRS[4] = 0x1612CC;
        bIsStartHardcoded = TRUE;
    } else if (StrEq(hash, "45d43542e694713bffd217862721109a")) // 613
    {
        symbols_PTRS->startdocked_PTRS[0] = 0x18993c;
        symbols_PTRS->startdocked_PTRS[1] = 0x18993c;
        symbols_PTRS->startdocked_PTRS[2] = 0x187ba0;
        symbols_PTRS->startdocked_PTRS[3] = 0x3c00;
        symbols_PTRS->startdocked_PTRS[4] = 0x16156c;
        bIsStartHardcoded = TRUE;
    } else if (StrEq(hash, "7f6d03e316dfca4ee61a89b51b453d82") ||
               StrEq(hash, "1b727beb3cafc0ce0e928ccfae4b8e8f")) // 675, 708
    {
        symbols_PTRS->startdocked_PTRS[0] = 0x189a7c;
        symbols_PTRS->startdocked_PTRS[1] = 0x189a7c;
        symbols_PTRS->startdocked_PTRS[2] = 0x187ce0;
        symbols_PTRS->startdocked_PTRS[3] = 0x3c00;
        symbols_PTRS->startdocked_PTRS[4] = 0x1616ac;
        bIsStartHardcoded = TRUE;
    } else if (StrEq(hash, "a7745c7fabca519e865a559bb7e13ed9")) // 778
    {
        symbols_PTRS->startdocked_PTRS[0] = 0x19a910;
        symbols_PTRS->startdocked_PTRS[1] = 0x19a910;
        symbols_PTRS->startdocked_PTRS[2] = 0x198a70;
        symbols_PTRS->startdocked_PTRS[3] = 0x4480;
        symbols_PTRS->startdocked_PTRS[4] = 0x170ddc;
        bIsStartHardcoded = TRUE;
    }

    if (bIsStartHardcoded) {
        wprintf(L"[Symbols] Identified known \"" STARTDOCKED_SB_NAME ".dll\" with hash \"%s\".\n", hash);

        RegCreateKeyExW(HKEY_CURRENT_USER, REGPATH_STARTMENU L"\\" STARTDOCKED_SB_NAME,
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition);
        if (hKey) {
            RegSetValueExW(hKey, L"" STARTDOCKED_SB_0, 0, REG_DWORD, (BYTE const *)&symbols_PTRS->startdocked_PTRS[0], sizeof(DWORD));
            RegSetValueExW(hKey, L"" STARTDOCKED_SB_1, 0, REG_DWORD, (BYTE const *)&symbols_PTRS->startdocked_PTRS[1], sizeof(DWORD));
            RegSetValueExW(hKey, L"" STARTDOCKED_SB_2, 0, REG_DWORD, (BYTE const *)&symbols_PTRS->startdocked_PTRS[2], sizeof(DWORD));
            RegSetValueExW(hKey, L"" STARTDOCKED_SB_3, 0, REG_DWORD, (BYTE const *)&symbols_PTRS->startdocked_PTRS[3], sizeof(DWORD));
            RegSetValueExW(hKey, L"" STARTDOCKED_SB_4, 0, REG_DWORD, (BYTE const *)&symbols_PTRS->startdocked_PTRS[4], sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }

    bIsStartHardcoded = FALSE;
    GetWindowsDirectoryW(wszPath, MAX_PATH);
    wcscat_s(wszPath, MAX_PATH, L"\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\" STARTUI_SB_NAME L".dll");
    ExplorerPatcher_ComputeFileHash(wszPath, hash, _countof(hash));
    string_to_lowercase(hash);

    if (StrEq(hash, "2768cc6cc7f686b2aa084cb5c8cce65d") ||
        StrEq(hash, "a7c82cb9a9fd6f87897fc8a737d6b4d7")) // 493, 527, 556, 613, 675
    {
        symbols_PTRS->startui_PTRS[0] = 0x37180;
        bIsStartHardcoded             = TRUE;
    } else if (StrEq(hash, "bab11b2d1dca6b167f313f4d54de2b7d") ||
               StrEq(hash, "0c1b88f888d9073c505d7f47724132c8")) // 708, 778
    {
        symbols_PTRS->startui_PTRS[0] = 0x37120;
        bIsStartHardcoded             = TRUE;
    }
    if (bIsStartHardcoded) {
        wprintf(L"[Symbols] Identified known \"" STARTUI_SB_NAME ".dll\" with hash %hs.\n", hash);

        RegCreateKeyExW(HKEY_CURRENT_USER, REGPATH_STARTMENU L"\\" STARTUI_SB_NAME,
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition);
        if (hKey) {
            RegSetValueExW(hKey, L"" STARTUI_SB_0, 0, REG_DWORD, &(symbols_PTRS->startui_PTRS[0]), sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }

    if (!bIsTwinuiPcshellHardcoded || !bIsStartHardcoded) {
        RegCreateKeyExW(HKEY_CURRENT_USER, REGPATH L"\\" TWINUI_PCSHELL_SB_NAME,
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, &dwDisposition);
        if (!hKey || hKey == INVALID_HANDLE_VALUE) {
            FreeLibraryAndExitThread(hModule, 1);
            return 1;
        }
        RegQueryValueExW(hKey, L"" TWINUI_PCSHELL_SB_0, NULL, NULL, (LPBYTE)&symbols_PTRS->twinui_pcshell_PTRS[0], &dwSize);
        RegQueryValueExW(hKey, L"" TWINUI_PCSHELL_SB_1, NULL, NULL, (LPBYTE)&symbols_PTRS->twinui_pcshell_PTRS[1], &dwSize);
        RegQueryValueExW(hKey, L"" TWINUI_PCSHELL_SB_2, NULL, NULL, (LPBYTE)&symbols_PTRS->twinui_pcshell_PTRS[2], &dwSize);
        RegQueryValueExW(hKey, L"" TWINUI_PCSHELL_SB_3, NULL, NULL, (LPBYTE)&symbols_PTRS->twinui_pcshell_PTRS[3], &dwSize);
        RegQueryValueExW(hKey, L"" TWINUI_PCSHELL_SB_4, NULL, NULL, (LPBYTE)&symbols_PTRS->twinui_pcshell_PTRS[4], &dwSize);
        RegQueryValueExW(hKey, L"" TWINUI_PCSHELL_SB_5, NULL, NULL, (LPBYTE)&symbols_PTRS->twinui_pcshell_PTRS[5], &dwSize);
        RegQueryValueExW(hKey, L"" TWINUI_PCSHELL_SB_6, NULL, NULL, (LPBYTE)&symbols_PTRS->twinui_pcshell_PTRS[6], &dwSize);
        RegQueryValueExW(hKey, L"" TWINUI_PCSHELL_SB_7, NULL, NULL, (LPBYTE)&symbols_PTRS->twinui_pcshell_PTRS[7], &dwSize);
        if (IsWindows11Version22H2OrHigher())
            RegQueryValueExW(hKey, L"" TWINUI_PCSHELL_SB_LAST, NULL, NULL,
                             (LPBYTE)&symbols_PTRS->twinui_pcshell_PTRS[TWINUI_PCSHELL_SB_CNT - 1], &dwSize);
        RegCloseKey(hKey);

        if (IsWindows11()) {
            RegCreateKeyExW(HKEY_CURRENT_USER, REGPATH_STARTMENU L"\\" STARTDOCKED_SB_NAME,
                            0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL,
                            &hKey, &dwDisposition);

            RegQueryValueExW(hKey, L"" STARTDOCKED_SB_0, NULL, NULL, (LPBYTE)&symbols_PTRS->startdocked_PTRS[0], &dwSize);
            RegQueryValueExW(hKey, L"" STARTDOCKED_SB_1, NULL, NULL, (LPBYTE)&symbols_PTRS->startdocked_PTRS[1], &dwSize);
            RegQueryValueExW(hKey, L"" STARTDOCKED_SB_2, NULL, NULL, (LPBYTE)&symbols_PTRS->startdocked_PTRS[2], &dwSize);
            RegQueryValueExW(hKey, L"" STARTDOCKED_SB_3, NULL, NULL, (LPBYTE)&symbols_PTRS->startdocked_PTRS[3], &dwSize);
            RegQueryValueExW(hKey, L"" STARTDOCKED_SB_4, NULL, NULL, (LPBYTE)&symbols_PTRS->startdocked_PTRS[4], &dwSize);
            if (hKey)
                RegCloseKey(hKey);
        }

        RTL_OSVERSIONINFOW rovi2;
        if (VnGetOSVersion(&rovi2) && rovi2.dwBuildNumber >= 18362) {
            RegCreateKeyExW(HKEY_CURRENT_USER, REGPATH_STARTMENU L"\\" STARTUI_SB_NAME,
                            0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, &dwDisposition);
            if (hKey) {
                RegQueryValueExW(hKey, L"" STARTUI_SB_0, NULL, NULL, (LPBYTE)&symbols_PTRS->startui_PTRS[0], &dwSize);
                RegCloseKey(hKey);
            }
        }
    }

    BOOL bNeedToDownload = FALSE;
    if (IsWindows11()) {
        for (UINT i = 0; i < sizeof(symbols_addr) / sizeof(DWORD); ++i) {
            if (!((DWORD *)symbols_PTRS)[i] &&
                  (DWORD *)symbols_PTRS + i != symbols_PTRS->twinui_pcshell_PTRS + TWINUI_PCSHELL_SB_CNT - 1 &&
                  (DWORD *)symbols_PTRS + i != symbols_PTRS->twinui_pcshell_PTRS + TWINUI_PCSHELL_SB_CNT - 2)
            {
                bNeedToDownload = TRUE;
            }
        }
    } else if (!symbols_PTRS->twinui_pcshell_PTRS[0] || !symbols_PTRS->twinui_pcshell_PTRS[2] ||
               !symbols_PTRS->twinui_pcshell_PTRS[3]) {
        bNeedToDownload = TRUE;
    }

    RegCreateKeyExW(HKEY_CURRENT_USER, L"" REGPATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, &dwDisposition);
    dwSize = MAX_PATH;
    RegQueryValueExW(hKey, L"OSBuild", NULL, NULL, szStoredVersion, &dwSize);
    RegCloseKey(hKey);

    if (!bNeedToDownload && (!bIsTwinuiPcshellHardcoded || !bIsStartHardcoded)) {
        bNeedToDownload = wcscmp(szReportedVersion, szStoredVersion);
        if (bNeedToDownload)
            ZeroMemory(symbols_PTRS, sizeof(symbols_addr));
    }
    return bNeedToDownload;
}
