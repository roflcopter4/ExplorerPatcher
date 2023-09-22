#define COBJMACROS
#include "Common/Common.h"

#include <Windows.h>
#include <DbgHelp.h>
#include <Psapi.h>
#include <ShellScalingApi.h>
#include <Shlobj_core.h>
#include <Shlwapi.h>
#include <UIAutomationClient.h>
#include <Uxtheme.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <featurestagingapi.h>
#include <initguid.h>
#include <propvarutil.h>
#include <roapi.h>
#include <tlhelp32.h>
#include <windowsx.h>

#include "cxx/offload.h"
#include "HideExplorerSearchBar.h"
#include "ImmersiveFlyouts.h"
#include "SettingsMonitor.h"
#include "lvt.h"
#include "osutility.h"
#include "resource.h"
#include "updates.h"
#include "utility.h"
#include "ep_weather_host/ep_weather.h"

#include <valinet/hooking/iatpatch.h>
#include <valinet/utility/memmem.h>
#ifdef _WIN64
# include <valinet/pdb/pdb.h>
# include "ep_weather_host/ep_weather_host_h.h"
# include "libs/sws/SimpleWindowSwitcher/sws_WindowSwitcher.h"
# include "ArchiveMenu.h"
# include "GUI.h"
# include "StartMenu.h"
# include "StartupSound.h"
# include "TaskbarCenter.h"
# include "dxgi_imp.h"
# include "hooking.h"
# include "symbols.h"
#endif
#ifdef USE_PRIVATE_INTERFACES
# include "ep_private.h"
#endif

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Propsys.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "UxTheme.lib")

#define WINX_ADJUST_X 5
#define WINX_ADJUST_Y 5

#define CHECKFOREGROUNDELAPSED_TIMEOUT 300
#define POPUPMENU_SAFETOREMOVE_TIMEOUT 300
#define POPUPMENU_BLUETOOTH_TIMEOUT    700
#define POPUPMENU_PNIDUI_TIMEOUT       300
#define POPUPMENU_SNDVOLSSO_TIMEOUT    300
#define POPUPMENU_INPUTSWITCH_TIMEOUT  700
#define POPUPMENU_WINX_TIMEOUT         700
#define POPUPMENU_EX_ELAPSED           300

// Only use this for developing fixes for 22621.2134+ using 22621.1413-1992.
#define USE_MOMENT_3_FIXES_ON_MOMENT_2 0

BOOL bIsExplorerProcess = FALSE;
BOOL bInstanced = FALSE;
HWND archivehWnd;
DWORD bOldTaskbar = TRUE;
DWORD bWasOldTaskbarSet = FALSE;
DWORD bAllocConsole = FALSE;
DWORD bHideExplorerSearchBar = FALSE;
DWORD bShrinkExplorerAddressBar = FALSE;
DWORD bMicaEffectOnTitlebar = FALSE;
DWORD bHideIconAndTitleInExplorer = FALSE;
DWORD bHideControlCenterButton = FALSE;
DWORD bFlyoutMenus = TRUE;
DWORD bCenterMenus = TRUE;
DWORD bSkinMenus = TRUE;
DWORD bSkinIcons = TRUE;
DWORD bReplaceNetwork = FALSE;
DWORD dwExplorerReadyDelay = 0;
DWORD bEnableArchivePlugin = FALSE;
DWORD bMonitorOverride = TRUE;
DWORD bOpenAtLogon = FALSE;
DWORD bClockFlyoutOnWinC = FALSE;
DWORD bUseClassicDriveGrouping = FALSE;
DWORD dwFileExplorerCommandUI = 9999;
DWORD bLegacyFileTransferDialog = FALSE;
DWORD bDisableImmersiveContextMenu = FALSE;
DWORD bClassicThemeMitigations = FALSE;
DWORD bWasClassicThemeMitigationsSet = FALSE;
DWORD bHookStartMenu = TRUE;
DWORD bPropertiesInWinX = FALSE;
DWORD bNoMenuAccelerator = FALSE;
DWORD dwIMEStyle = 0;
DWORD dwTaskbarAl = 1;
DWORD bShowUpdateToast = FALSE;
DWORD bToolbarSeparators = FALSE;
DWORD bTaskbarAutohideOnDoubleClick = FALSE;
DWORD dwOrbStyle = 0;
DWORD bEnableSymbolDownload = TRUE;
DWORD dwAltTabSettings = 0;
DWORD bDisableAeroSnapQuadrants = FALSE;
DWORD dwSnapAssistSettings = 0;
DWORD dwStartShowClassicMode = 0;
BOOL bDoNotRedirectSystemToSettingsApp = FALSE;
BOOL bDoNotRedirectProgramsAndFeaturesToSettingsApp = FALSE;
BOOL bDoNotRedirectDateAndTimeToSettingsApp = FALSE;
BOOL bDoNotRedirectNotificationIconsToSettingsApp = FALSE;
BOOL bDisableOfficeHotkeys = FALSE;
BOOL bDisableWinFHotkey = FALSE;
DWORD bNoPropertiesInContextMenu = FALSE;
#define TASKBARGLOMLEVEL_DEFAULT 2
#define MMTASKBARGLOMLEVEL_DEFAULT 2
#define MAX_NUM_MONITORS 30

#define ORB_STYLE_WINDOWS10   0
#define ORB_STYLE_WINDOWS11   1
#define ORB_STYLE_TRANSPARENT 2

typedef struct _OrbInfo {
    HTHEME hTheme;
    UINT   dpi;
} OrbInfo;

typedef int(__stdcall *PeopleBandDrawTextWithGlowHook_CB_t)
(HDC, uint16_t *, INT, RECT *, UINT, INT64);

typedef interface IInputSwitchControl IInputSwitchControl;
typedef interface EnumExplorerCommand EnumExplorerCommand;
typedef interface UICommand           UICommand;
typedef interface ITaskBtnGroup       ITaskBtnGroup;

/*--------------------------------------------------------------------------------------*/

HMODULE hModule                = NULL;
HANDLE  hServiceWindowThread   = NULL;
DWORD   bMonitorOverride       = TRUE;
DWORD   bOpenAtLogon           = FALSE;
DWORD   dwWeatherToLeft        = 0;
DWORD   dwOldTaskbarAl         = 0b110;
DWORD   dwMMOldTaskbarAl       = 0b110;
DWORD   dwSearchboxTaskbarMode = FALSE;
BOOL    g_bIsDesktopRaised     = FALSE;

#ifdef _WIN64
HWND PeopleButton_LastHWND = NULL;
#else
RTL_OSVERSIONINFOW global_rovi;
DWORD32            global_ubr;
#endif

int (*SHWindowsPolicy)(REFIID);

/*--------------------------------------------------------------------------------------*/

static BOOL bDoNotRedirectDateAndTimeToSettingsApp         = FALSE;
static BOOL bDoNotRedirectNotificationIconsToSettingsApp   = FALSE;
static BOOL bDoNotRedirectProgramsAndFeaturesToSettingsApp = FALSE;
static BOOL bDoNotRedirectSystemToSettingsApp              = FALSE;
static BOOL bDisableOfficeHotkeys                          = FALSE;
static BOOL bDisableWinFHotkey                             = FALSE;
static BOOL bInstanced                                     = FALSE;
static BOOL bIsExplorerProcess                             = FALSE;
static BOOL bIsImmersiveMenu                               = FALSE;
static BOOL bPeopleHasEllipsed                             = FALSE;
static BOOL bSpotlightIsDesktopContextMenu                 = FALSE;

static DWORD bAllocConsole                       = FALSE;
static DWORD bCenterMenus                        = TRUE;
static DWORD bClassicThemeMitigations            = FALSE;
static DWORD bClockFlyoutOnWinC                  = FALSE;
static DWORD bDisableAeroSnapQuadrants           = FALSE;
static DWORD bDisableImmersiveContextMenu        = FALSE;
static DWORD bDisableSpotlightIcon               = FALSE;
static DWORD bEnableArchivePlugin                = FALSE;
static DWORD bEnableSymbolDownload               = TRUE;
static DWORD bFlyoutMenus                        = TRUE;
static DWORD bHideControlCenterButton            = FALSE;
static DWORD bHideExplorerSearchBar              = FALSE;
static DWORD bHideIconAndTitleInExplorer         = FALSE;
static DWORD bHookStartMenu                      = TRUE;
static DWORD bLegacyFileTransferDialog           = FALSE;
static DWORD bMicaEffectOnTitlebar               = FALSE;
static DWORD bNoMenuAccelerator                  = FALSE;
static DWORD bNoPropertiesInContextMenu          = FALSE;
static DWORD bOldTaskbar                         = TRUE;
static DWORD bPinnedItemsActAsQuickLaunch        = FALSE;
static DWORD bPropertiesInWinX                   = FALSE;
static DWORD bRemoveExtraGapAroundPinnedItems    = FALSE;
static DWORD bReplaceNetwork                     = FALSE;
static DWORD bShowUpdateToast                    = FALSE;
static DWORD bShrinkExplorerAddressBar           = FALSE;
static DWORD bSkinIcons                          = TRUE;
static DWORD bSkinMenus                          = TRUE;
static DWORD bTaskbarAutohideOnDoubleClick       = FALSE;
static DWORD bToolbarSeparators                  = FALSE;
static DWORD bUseClassicDriveGrouping            = FALSE;
static DWORD bWasClassicThemeMitigationsSet      = FALSE;
static DWORD bWasOldTaskbarSet                   = FALSE;
static DWORD bWasPinnedItemsActAsQuickLaunch     = FALSE;
static DWORD bWasRemoveExtraGapAroundPinnedItems = FALSE;
static DWORD bWeatherFixedSize                   = FALSE;

static DWORD dwAltTabSettings                = 0;
static DWORD dwExplorerReadyDelay            = 0;
static DWORD dwFileExplorerCommandUI         = 9999;
static DWORD dwIMEStyle                      = 0;
static DWORD dwMMTaskbarGlomLevel            = MMTASKBARGLOMLEVEL_DEFAULT;
static DWORD dwMonitorCount                  = 0;
static DWORD dwOrbStyle                      = 0;
static DWORD dwShowTaskViewButton            = FALSE;
static DWORD dwSnapAssistSettings            = 0;
static DWORD dwSpotlightDesktopMenuMask      = 0;
static DWORD dwSpotlightUpdateSchedule       = 0;
static DWORD dwStartShowClassicMode          = 0;
static DWORD dwTaskbarAl                     = 1;
static DWORD dwTaskbarDa                     = FALSE;
static DWORD dwTaskbarGlomLevel              = TASKBARGLOMLEVEL_DEFAULT;
static DWORD dwTaskbarSmallIcons             = FALSE;
static DWORD dwUndeadStartCorner             = FALSE;
static DWORD dwUpdatePolicy                  = UPDATE_POLICY_DEFAULT;
static DWORD dwWeatherContentsMode           = 0;
static DWORD dwWeatherDevMode                = FALSE;
static DWORD dwWeatherGeolocationMode        = 0;
static DWORD dwWeatherIconPack               = EP_WEATHER_ICONPACK_MICROSOFT;
static DWORD dwWeatherTemperatureUnit        = EP_WEATHER_TUNIT_CELSIUS;
static DWORD dwWeatherTheme                  = 0;
static DWORD dwWeatherUpdateSchedule         = EP_WEATHER_UPDATE_NORMAL;
static DWORD dwWeatherViewMode               = EP_WEATHER_VIEW_ICONTEXT;
static DWORD dwWeatherWindowCornerPreference = DWMWCP_ROUND;
static DWORD dwWeatherZoomFactor             = 0;

static DWORD StartDocked_DisableRecommendedSection      = FALSE;
static DWORD StartDocked_DisableRecommendedSectionApply = TRUE;
static DWORD StartMenu_ShowAllApps                      = 0;
static DWORD StartMenu_maximumFreqApps                  = 6;
static DWORD StartUI_EnableRoundedCorners               = FALSE;
static DWORD StartUI_EnableRoundedCornersApply          = TRUE;
static DWORD StartUI_ShowMoreTiles                      = FALSE;
static DWORD Start_ForceStartSize                       = 0;
static DWORD Start_NoStartMenuMorePrograms              = 0;
static DWORD sws_IsEnabled                              = FALSE;

static DWORD  epw_cbTemperature  = 0;
static DWORD  epw_cbUnit         = 0;
static DWORD  epw_cbCondition    = 0;
static DWORD  epw_cbImage        = 0;
static WCHAR *epw_wszTemperature = NULL;
static WCHAR *epw_wszUnit        = NULL;
static WCHAR *epw_wszCondition   = NULL;
static char  *epw_pImage         = NULL;

static HHOOK  hShell_TrayWndMouseHook = NULL;
static HANDLE hCheckForegroundThread  = NULL;
static HANDLE hShell32                = NULL;
static HANDLE hDelayedInjectionThread = NULL;
static HANDLE hIsWinXShown            = NULL;
static HANDLE hWinXThread             = NULL;
static HANDLE hSwsSettingsChanged     = NULL;
static HANDLE hSwsOpacityMaybeChanged = NULL;
static HANDLE hWin11AltTabInitialized = NULL;
static HANDLE hEPWeatherKillswitch    = NULL;
static HANDLE hCanStartSws            = NULL;
static HWND   hWndServiceWindow       = NULL;
static HWND   hArchivehWnd            = NULL;
static HWND   hWndWeatherFlyout       = NULL;
static HWND   hWinXWnd                = NULL;
static HKEY   hKey_StartUI_TileGrid   = NULL;
static HKEY   hKeySpotlight1          = NULL;
static HKEY   hKeySpotlight2          = NULL;
static HDPA   hOrbCollection          = NULL;

static HMONITOR hMonitorList[MAX_NUM_MONITORS];
static WCHAR    wszEPWeatherKillswitch[MAX_PATH];
static WCHAR    wszWeatherTerm[MAX_PATH];
static WCHAR    wszWeatherLanguage[MAX_PATH];

static BYTE     *lpShouldDisplayCCButton = NULL;
static ULONGLONG elapsedCheckForeground  = 0;
static int       dllCode                 = 0;

#ifdef _WIN64
static int prev_total_h = 0;
static IEPWeather *epw = NULL;
static CRITICAL_SECTION lock_epw;
#endif

/*--------------------------------------------------------------------------------------*/

static __int64 (*PeopleBand_DrawTextWithGlowFunc)(
    HDC, const UINT16 *, INT, RECT *,
    UINT, UINT, UINT, UINT, UINT, INT,
    PeopleBandDrawTextWithGlowHook_CB_t, INT64);

static HWND(*CreateWindowExWFunc)(
    DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
    int X, int Y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);

static HRESULT(*CTaskGroup_DoesWindowMatchFunc)(
    LONG_PTR *task_group, HWND hCompareWnd, ITEMIDLIST *pCompareItemIdList,
    WCHAR *pCompareAppId, int *pnMatch, LONG_PTR **p_task_item);

static BOOL     (*SHELL32_CanDisplayWin8CopyDialogFunc)(void);
static BOOL     (*SetChildWindowNoActivateFunc)(HWND);
static BOOL     (*ShouldAppsUseDarkMode)(void);
static BOOL     (*ShouldSystemUseDarkMode)(void);
static HRESULT  (*CInputSwitchControl_InitFunc)(IInputSwitchControl *, UINT);
static HRESULT  (*CInputSwitchControl_ShowInputSwitchFunc)(IInputSwitchControl *, RECT *);
static HRESULT  (*ICoreWindow5_get_DispatcherQueueFunc)(INT64, INT64);
static HRESULT  (*Widgets_GetTooltipTextFunc)(INT64, INT64, INT64, LPWSTR, UINT);
static HRESULT  (*explorer_SetWindowThemeFunc)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
static HRESULT  (*shell32_UICommand_InvokeFunc)(UICommand *, void *, void *);
static HWND     (*NtUserFindWindowEx)(HWND hWndParent, HWND hWndChildAfter, LPCWSTR lpszClass, LPCWSTR lpszWindow, DWORD dwType);
static INT      (*PeopleButton_ShowTooltipFunc)(INT64, UINT8 bShow);
static INT      (*RtlQueryFeatureConfigurationFunc)(UINT32, INT, INT64 *, struct RTL_FEATURE_CONFIGURATION *);
static INT64    (*CImmersiveContextMenuOwnerDrawHelper_s_ContextMenuWndProcFunc)(HWND hWnd, INT, HWND, INT, BOOL *);
static INT64    (*CLauncherTipContextMenu_GetMenuItemsAsyncFunc)(void *_this, void *rect, void **iunk);
static INT64    (*CLauncherTipContextMenu_ShowLauncherTipContextMenuFunc)(void *_this, POINT *);
static INT64    (*ImmersiveContextMenuHelper_ApplyOwnerDrawToMenuFunc)(HMENU, HMENU, HWND, UINT, void *data);
static INT64    (*PeopleButton_OnClickFunc)(INT64, INT64);
static INT64    (*StartDocked_LauncherFrame_OnVisibilityChangedFunc)(void *, INT64, void *);
static INT64    (*StartDocked_LauncherFrame_ShowAllAppsFunc)(void *_this);
static INT64    (*StartDocked_StartSizingFrame_StartSizingFrameFunc)(void *_this);
static INT64    (*StartDocked_SystemListPolicyProvider_GetMaximumFrequentAppsFunc)(void *);
static INT64    (*StartUI_SystemListPolicyProvider_GetMaximumFrequentAppsFunc)(void *);
static INT64    (*Widgets_OnClickFunc)(INT64, INT64);
static INT64    (*twinui_pcshell_CMultitaskingViewManager__CreateDCompMTVHostFunc)(INT64, UINT, INT64, INT64, INT64 *);
static INT64    (*twinui_pcshell_CMultitaskingViewManager__CreateXamlMTVHostFunc)(INT64, UINT, INT64, INT64, INT64 *);
static INT64    (*twinui_pcshell_IsUndockedAssetAvailableFunc)(INT, INT64, INT64, LPCSTR);
static INT64    (*winrt_Windows_Internal_Shell_implementation_MeetAndChatManager_OnMessageFunc)(void *_this, INT64, INT);
static LONG_PTR (*CTaskBtnGroup_GetIdealSpanFunc)(ITaskBtnGroup *_this, LONG_PTR, LONG_PTR, LONG_PTR, LONG_PTR, LONG_PTR);
static LONG_PTR (*SetWindowLongPtrWFunc)(HWND hWnd, INT nIndex, LONG_PTR dwNewLong);
static SIZE     (*PeopleButton_CalculateMinimumSizeFunc)(void *, SIZE *);
static UINT     (*GetTaskbarColor)(INT64, INT64);
static UINT     (*GetTaskbarTheme)(void);
static void     (*AllowDarkModeForWindow)(HWND hWnd, INT64 bAllowDark);
static void     (*CLauncherTipContextMenu_ExecuteCommandFunc)(void *_this, INT);
static void     (*CLauncherTipContextMenu_ExecuteShutdownCommandFunc)(void *_this, void *);
static void     (*GetThemeName)(void *, void *, void *);
static void     (*ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc)(HMENU _this, HMENU hWnd, HWND);
static void     (*SetPreferredAppMode)(INT64 bAllowDark);

static HRESULT(STDMETHODCALLTYPE *shell32_DriveTypeCategorizer_CreateInstanceFunc)(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
static HRESULT(WINAPI *explorer_SHCreateStreamOnModuleResourceWFunc)(HMODULE hModule, LPCWSTR pwszName, LPCWSTR pwszType, IStream **ppStream);
static HRESULT(WINAPI *SLGetWindowsInformationDWORDFunc)(PCWSTR pwszValueName, DWORD *pdwValue);

/*--------------------------------------------------------------------------------------*/

static void *P_Icon_Light_Search = NULL;
static DWORD S_Icon_Light_Search = 0;
static void *P_Icon_Light_TaskView = NULL;
static DWORD S_Icon_Light_TaskView = 0;
static void *P_Icon_Light_Widgets = NULL;
static DWORD S_Icon_Light_Widgets = 0;
static void *P_Icon_Dark_Search = NULL;
static DWORD S_Icon_Dark_Search = 0;
static void *P_Icon_Dark_TaskView = NULL;
static DWORD S_Icon_Dark_TaskView = 0;
static void *P_Icon_Dark_Widgets = NULL;
static DWORD S_Icon_Dark_Widgets = 0;

extern HRESULT     InjectStartFromExplorer(void);
extern BOOL        InvokeClockFlyout(void);
extern void WINAPI Explorer_RefreshUI(int unused);

extern HRESULT WINAPI _DllRegisterServer();
extern HRESULT WINAPI _DllUnregisterServer();
extern HRESULT WINAPI _DllCanUnloadNow();
extern HRESULT WINAPI _DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);

/****************************************************************************************/

#pragma region "Updates"
#ifdef _WIN64

static DWORD CheckForUpdatesThread(LPVOID timeout)
{
    HRESULT        hr = S_OK;
    HSTRING_HEADER header_AppIdHString;
    HSTRING_HEADER header_ToastNotificationManagerHString;
    HSTRING_HEADER header_ToastNotificationHString;
    HSTRING        AppIdHString                    = NULL;
    HSTRING        ToastNotificationManagerHString = NULL;
    HSTRING        ToastNotificationHString        = NULL;

    __x_ABI_CWindows_CUI_CNotifications_CIToastNotificationManagerStatics *toastStatics = NULL;
    __x_ABI_CWindows_CUI_CNotifications_CIToastNotifier                   *notifier     = NULL;
    __x_ABI_CWindows_CUI_CNotifications_CIToastNotificationFactory        *notifFactory = NULL;
    __x_ABI_CWindows_CUI_CNotifications_CIToastNotification               *toast        = NULL;

    while (TRUE) {
        HWND hShell_TrayWnd = FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL);
        if (hShell_TrayWnd) {
            Sleep(timeout);
            break;
        }
        Sleep(100);
    }
    wprintf(L"[Updates] Starting daemon.\n");

    if (SUCCEEDED(hr))
        hr = RoInitialize(RO_INIT_MULTITHREADED);
    if (SUCCEEDED(hr))
        hr = WindowsCreateStringReference(APPID, (UINT32)(sizeof(APPID) / sizeof(WCHAR) - 1), &header_AppIdHString, &AppIdHString);
    if (SUCCEEDED(hr)) {
        hr = WindowsCreateStringReference(
            RuntimeClass_Windows_UI_Notifications_ToastNotificationManager,
            (UINT32)(sizeof(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager) / sizeof(wchar_t) - 1),
            &header_ToastNotificationManagerHString, &ToastNotificationManagerHString
        );
    }
    if (SUCCEEDED(hr))
        hr = RoGetActivationFactory(ToastNotificationManagerHString, &UIID_IToastNotificationManagerStatics, (LPVOID *)&toastStatics);
    if (SUCCEEDED(hr))
        hr = toastStatics->lpVtbl->CreateToastNotifierWithId(toastStatics, AppIdHString, &notifier);
    if (SUCCEEDED(hr)) {
        hr = WindowsCreateStringReference(
            RuntimeClass_Windows_UI_Notifications_ToastNotification,
            (UINT32)(sizeof(RuntimeClass_Windows_UI_Notifications_ToastNotification) / sizeof(wchar_t) - 1),
            &header_ToastNotificationHString, &ToastNotificationHString
        );
    }
    if (SUCCEEDED(hr))
        hr = RoGetActivationFactory(ToastNotificationHString, &UIID_IToastNotificationFactory, (LPVOID *)&notifFactory);

    HANDLE hEvents[2] = {
        CreateEventW(NULL, FALSE, FALSE, L"EP_Ev_CheckForUpdates_" EP_CLSID),
        CreateEventW(NULL, FALSE, FALSE, L"EP_Ev_InstallUpdates_" EP_CLSID),
    };

    if (hEvents[0] && hEvents[1]) {
        if (bShowUpdateToast) {
            ShowUpdateSuccessNotification(hModule, notifier, notifFactory, &toast);

            HKEY hKey = NULL;
            RegCreateKeyExW(HKEY_CURRENT_USER, L"" REGPATH, 0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WOW64_64KEY | KEY_WRITE, NULL, &hKey, NULL);

            if (hKey != NULL && hKey != INVALID_HANDLE_VALUE) {
                bShowUpdateToast = FALSE;
                RegSetValueExW(hKey, L"IsUpdatePending", 0, REG_DWORD, &bShowUpdateToast, sizeof(DWORD));
                RegCloseKey(hKey);
            }
        }
        if (dwUpdatePolicy != UPDATE_POLICY_MANUAL)
            InstallUpdatesIfAvailable(hModule, notifier, notifFactory, &toast,
                                      UPDATES_OP_DEFAULT, bAllocConsole, dwUpdatePolicy);

        DWORD dwRet = 0;
        while (TRUE) {
            switch (WaitForMultipleObjects(2, hEvents, FALSE, INFINITE)) {
            case WAIT_OBJECT_0:
                InstallUpdatesIfAvailable(hModule, notifier, notifFactory, &toast,
                                          UPDATES_OP_CHECK, bAllocConsole, dwUpdatePolicy);
                break;
            case WAIT_OBJECT_0 + 1:
                InstallUpdatesIfAvailable(hModule, notifier, notifFactory, &toast,
                                          UPDATES_OP_INSTALL, bAllocConsole, dwUpdatePolicy);
                break;
            default:
                break;
            }
        }
        CloseHandle(hEvents[0]);
        CloseHandle(hEvents[1]);
    }

    if (toast)
        toast->lpVtbl->Release(toast);
    if (notifFactory)
        notifFactory->lpVtbl->Release(notifFactory);
    if (ToastNotificationHString)
        WindowsDeleteString(ToastNotificationHString);
    if (notifier)
        notifier->lpVtbl->Release(notifier);
    if (toastStatics)
        toastStatics->lpVtbl->Release(toastStatics);
    if (ToastNotificationManagerHString)
        WindowsDeleteString(ToastNotificationManagerHString);
    if (AppIdHString)
        WindowsDeleteString(AppIdHString);

    return 0;
}

#endif
#pragma endregion


#pragma region "Generics"
#ifdef _WIN64

static HWND GetMonitorInfoFromPointForTaskbarFlyoutActivation(POINT ptCursor, DWORD dwFlags, LPMONITORINFO lpMi)
{
    HMONITOR hMonitor = MonitorFromPoint(ptCursor, dwFlags);
    HWND     hWnd     = NULL;
    do {
        hWnd = FindWindowExW(NULL, hWnd, L"Shell_SecondaryTrayWnd", NULL);
        if (MonitorFromWindow(hWnd, dwFlags) == hMonitor) {
            if (lpMi)
                GetMonitorInfo(MonitorFromPoint(ptCursor, dwFlags), lpMi);
            break;
        }
    } while (hWnd);
    if (!hWnd) {
        hWnd = FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL);
        // ptCursor.x = 0;
        // ptCursor.y = 0;
        if (lpMi)
            GetMonitorInfoW(MonitorFromWindow(hWnd, dwFlags), lpMi);
    }
    return hWnd;
}

static POINT GetDefaultWinXPosition(
    BOOL  bUseRcWork,
    BOOL *lpBottom,
    BOOL *lpRight,
    BOOL  bAdjust,
    BOOL  bToRight
)
{
    if (lpBottom)
        *lpBottom = FALSE;
    if (lpRight)
        *lpRight = FALSE;
    POINT point;
    point.x = 0;
    point.y = 0;
    POINT ptCursor;
    GetCursorPos(&ptCursor);
    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    HWND hWnd = GetMonitorInfoFromPointForTaskbarFlyoutActivation(ptCursor, MONITOR_DEFAULTTOPRIMARY, &mi);

    if (hWnd) {
        RECT rc;
        GetWindowRect(hWnd, &rc);
        if (rc.left - mi.rcMonitor.left <= 0) {
            if (bUseRcWork)
                point.x = mi.rcWork.left;
            else
                point.x = mi.rcMonitor.left;
            if (bToRight)
                point.x = mi.rcMonitor.right;
            if (bAdjust)
                point.x++;
            if (rc.top - mi.rcMonitor.top <= 0) {
                if (bUseRcWork)
                    point.y = mi.rcWork.top;
                else
                    point.y = mi.rcMonitor.top;
                if (bAdjust)
                    point.y++;
            } else {
                if (lpBottom)
                    *lpBottom = TRUE;
                if (bUseRcWork)
                    point.y = mi.rcWork.bottom;
                else
                    point.y = mi.rcMonitor.bottom;
                if (bAdjust)
                    point.y--;
            }
        } else {
            if (lpRight)
                *lpRight = TRUE;
            if (bUseRcWork)
                point.x = mi.rcWork.right;
            else
                point.x = mi.rcMonitor.right;
            if (bAdjust)
                point.x--;
            if (rc.top - mi.rcMonitor.top <= 0) {
                if (bUseRcWork)
                    point.y = mi.rcWork.top;
                else
                    point.y = mi.rcMonitor.top;
                if (bAdjust)
                    point.y++;
            } else {
                if (lpBottom)
                    *lpBottom = TRUE;
                if (bUseRcWork)
                    point.y = mi.rcWork.bottom;
                else
                    point.y = mi.rcMonitor.bottom;
                if (bAdjust)
                    point.y--;
            }
        }
    }

    return point;
}

static BOOL TerminateShellExperienceHost(void)
{
    BOOL  bRet = FALSE;
    WCHAR wszKnownPath[MAX_PATH];
    GetWindowsDirectoryW(wszKnownPath, MAX_PATH);
    wcscat_s(wszKnownPath, MAX_PATH, L"\\SystemApps\\ShellExperienceHost_cw5n1h2txyewy\\ShellExperienceHost.exe");

    PROCESSENTRY32W pe32 = {.dwSize = sizeof pe32};
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32FirstW(hSnapshot, &pe32) == TRUE) {
        do {
            if (WStrEq(pe32.szExeFile, L"ShellExperienceHost.exe")) {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    WCHAR wszProcessPath[MAX_PATH];
                    DWORD dwLength = MAX_PATH;
                    QueryFullProcessImageNameW(hProcess, 0, wszProcessPath, &dwLength);
                    if (WStrIEq(wszProcessPath, wszKnownPath)) {
                        TerminateProcess(hProcess, 0);
                        bRet = TRUE;
                    }
                    CloseHandle(hProcess);
                    hProcess = NULL;
                }
            }
        } while (Process32NextW(hSnapshot, &pe32) == TRUE);
    }

    if (hSnapshot)
        CloseHandle(hSnapshot);
    return bRet;
}

static DWORD CheckForegroundThread(DWORD dwMode)
{
    wprintf(L"Started \"Check foreground window\" thread.\n");
    UINT i = 0;
    while (TRUE) {
        wchar_t text[256];
        GetClassNameW(GetForegroundWindow(), text, _countof(text));
        if (WStrEq(text, L"Windows.UI.Core.CoreWindow"))
            break;
        if (++i >= 15)
            break;
        Sleep(100);
    }
    while (TRUE) {
        wchar_t text[256];
        GetClassNameW(GetForegroundWindow(), text, _countof(text));
        if (!WStrEq(text, L"Windows.UI.Core.CoreWindow"))
            break;
        Sleep(100);
    }
    elapsedCheckForeground = milliseconds_now();
    if (!dwMode) {
        RegDeleteTreeW(HKEY_CURRENT_USER, L"" SEH_REGPATH);
        TerminateShellExperienceHost();
        Sleep(100);
    }
    wprintf(L"Ended \"Check foreground window\" thread.\n");
    return 0;
}

static void LaunchNetworkTargets(DWORD dwTarget)
{
    // very helpful: https://www.tenforums.com/tutorials/3123-clsid-key-guid-shortcuts-list-windows-10-a.html

    switch (dwTarget) {
    case 0:
        InvokeFlyout(INVOKE_FLYOUT_SHOW, INVOKE_FLYOUT_NETWORK);
        break;
    case 1:
        ShellExecuteW(NULL, L"open", L"ms-settings:network", NULL, NULL, SW_SHOWNORMAL);
        break;
    case 2: {
        HMODULE hVan = LoadLibraryW(L"van.dll");
        if (hVan) {
            long (*ShowVAN)(BOOL, BOOL, void *) = GetProcAddress(hVan, "ShowVAN");
            if (ShowVAN)
                ShowVAN(0, 0, NULL);
            FreeLibrary(hVan);
        }
        break;
    }
    case 3:
        ShellExecuteW(NULL, L"open", L"shell:::{8E908FC9-BECC-40f6-915B-F4CA0E70D03D}", NULL, NULL, SW_SHOWNORMAL);
        break;
    case 4:
        ShellExecuteW(NULL, L"open", L"shell:::{7007ACC7-3202-11D1-AAD2-00805FC1270E}", NULL, NULL, SW_SHOWNORMAL);
        break;
    case 5:
        ShellExecuteW(NULL, L"open", L"ms-availablenetworks:", NULL, NULL, SW_SHOWNORMAL);
        break;
    case 6:
        InvokeActionCenter();
        //ShellExecuteW(NULL, L"open", L"ms-actioncenter:controlcenter/&showFooter=true", NULL, NULL, SW_SHOWNORMAL);
        break;
    }
}

#endif
#pragma endregion


#pragma region "Service Window"
#ifdef _WIN64


static void FixUpCenteredTaskbar(void)
{
    PostMessageW(FindWindowW(L"Shell_TrayWnd", NULL), 798, 0, 0); // uMsg = 0x31E in explorer!TrayUI::WndProc
}

# define EP_SERVICE_WINDOW_CLASS_NAME L"EP_Service_Window_" EP_CLSID
static LRESULT CALLBACK
EP_Service_Window_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT s_uTaskbarRestart = 0;

    if (uMsg == WM_CREATE) {
        s_uTaskbarRestart = RegisterWindowMessageW(L"TaskbarCreated");
    }
    else if (uMsg == WM_HOTKEY && (wParam == 1 || wParam == 2)) {
        InvokeClockFlyout();
        return 0;
    }
    else if (uMsg == s_uTaskbarRestart && bOldTaskbar && (dwOldTaskbarAl || dwMMOldTaskbarAl)) {
        SetTimer(hWnd, 1, 1000, NULL);
    }
    else if (uMsg == WM_TIMER && wParam < 3) {
        FixUpCenteredTaskbar();
        if (wParam != 3 - 1)
            SetTimer(hWnd, wParam + 1, 1000, NULL);
        KillTimer(hWnd, wParam);
        return 0;
    }
    else if (uMsg == WM_TIMER && wParam == 10) {
        if (GetClassWord(GetForegroundWindow(), GCW_ATOM) != RegisterWindowMessageW(L"Windows.UI.Core.CoreWindow"))
        {
            DWORD dwVal = 1;
            RegSetKeyValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                            L"TaskbarAl", REG_DWORD, &dwVal, sizeof(DWORD));
            KillTimer(hWnd, 10);
        }
        return 0;
    }
    else if (uMsg == WM_TIMER && wParam == 100) {
        if (IsSpotlightEnabled())
            SpotlightHelper(SPOP_CLICKMENU_NEXTPIC, hWnd, NULL, NULL);
        wprintf(L"Refreshed Spotlight\n");
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static DWORD EP_ServiceWindowThread(DWORD unused)
{
    WNDCLASSW wc = {
        .style         = CS_DBLCLKS,
        .lpfnWndProc   = EP_Service_Window_WndProc,
        .hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH),
        .hInstance     = GetModuleHandleW(NULL),
        .lpszClassName = EP_SERVICE_WINDOW_CLASS_NAME,
        .hCursor       = LoadCursorW(NULL, IDC_ARROW),
    };
    // BUG I don't know what this is intended to be doing, but I can say that it will just result in an error.
    RegisterClassW(&wc);

    hWndServiceWindow = CreateWindowExW(0, EP_SERVICE_WINDOW_CLASS_NAME, NULL, WS_POPUP,
                                        0, 0, 0, 0, NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (hWndServiceWindow) {
        if (IsSpotlightEnabled() && dwSpotlightUpdateSchedule)
            SetTimer(hWndServiceWindow, 100, dwSpotlightUpdateSchedule * 1000, NULL);
        if (bClockFlyoutOnWinC)
            RegisterHotKey(hWndServiceWindow, 1, MOD_WIN | MOD_NOREPEAT, 'C');
        RegisterHotKey(hWndServiceWindow, 2, MOD_WIN | MOD_ALT, 'D');

        MSG  msg;
        BOOL bRet;
        while ((bRet = GetMessageW(&msg, NULL, 0, 0)) != 0) {
            if (bRet == -1) {
                break;
            } else if (!msg.hwnd) {
                if (msg.message == WM_USER + 1) {
                    EnterCriticalSection(&lock_epw);
                    if (epw) {
                        epw->lpVtbl->Release(epw);
                        epw          = NULL;
                        prev_total_h = 0;
                        if (PeopleButton_LastHWND)
                            InvalidateRect(PeopleButton_LastHWND, NULL, TRUE);
                        // PlaySoundA((LPCTSTR)SND_ALIAS_SYSTEMASTERISK, NULL, SND_ALIAS_ID);
                    }
                    LeaveCriticalSection(&lock_epw);
                }
            } else {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
        DestroyWindow(hWndServiceWindow);
    }
    SetEvent(hCanStartSws);
    return 0;
}

#endif
#pragma endregion


#pragma region "Toggle shell features"
// More details in explorer.exe!CTray::_HandleGlobalHotkey

static BOOL CALLBACK ToggleImmersiveCallback(HWND hWnd, LPARAM lParam)
{
#if 0
    WORD ClassWord = GetClassWord(hWnd, GCW_ATOM);
    if (ClassWord == RegisterWindowMessageW(L"WorkerW"))
        PostMessageW(hWnd, WM_HOTKEY, lParam, 0);
    return TRUE;
#else
    WCHAR szClassName[256];
    GetClassNameW(hWnd, szClassName, sizeof(szClassName) / sizeof(WCHAR));
    if (WStrEq(szClassName, L"WorkerW"))
        PostMessageW(hWnd, WM_HOTKEY, lParam, 0);
    return TRUE;
#endif
}

static void ToggleHelp(void)
{
    PostMessageW(FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL), WM_HOTKEY, 505, 0);
}

static void ToggleRunDialog(void)
{
    PostMessageW(FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL), WM_HOTKEY, 502, MAKELPARAM(MOD_WIN, 0x52));
}

static void ToggleSystemProperties(void)
{
    PostMessageW(FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL), WM_HOTKEY, 512, 0);
}

static void FocusSystray(void)
{
    PostMessageW(FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL), WM_HOTKEY, 514, 0);
}

static void TriggerAeroShake(void)
{
    PostMessageW(FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL), WM_HOTKEY, 515, 0);
}

static void PeekDesktop(void)
{
    PostMessageW(FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL), WM_HOTKEY, 516, 0);
}

static void ToggleEmojiPanel(void)
{
    PostMessageW(FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL), WM_HOTKEY, 579, 0);
}

static void ShowDictationPanel(void)
{
    PostMessageW(FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL), WM_HOTKEY, 577, 0);
}

static void ToggleClipboardViewer(void)
{
    PostMessageW(FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL), WM_HOTKEY, 578, 0);
}

static void ToggleSearch(void)
{
    PostMessageW(FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL), WM_HOTKEY, 507, MAKELPARAM(MOD_WIN, 0x53));
}

static void ToggleTaskView(void)
{
    HWND  win = FindWindowExW(NULL, NULL, L"ApplicationManager_ImmersiveShellWindow", NULL);
    DWORD id  = GetWindowThreadProcessId(win, NULL);
    BOOL  res = EnumThreadWindows(id, ToggleImmersiveCallback, 11);
    //wprintf(L"Toggling Task View callback state (%p, %lu, %d).\n", (void *)win, id, res);
}

static void ToggleWidgetsPanel(void)
{
    EnumThreadWindows(GetWindowThreadProcessId(FindWindowExW(NULL, NULL, L"ApplicationManager_ImmersiveShellWindow", NULL), NULL), ToggleImmersiveCallback, 0x66);
}

static void ToggleMainClockFlyout(void)
{
    EnumThreadWindows(GetWindowThreadProcessId(FindWindowExW(NULL, NULL, L"ApplicationManager_ImmersiveShellWindow", NULL), NULL), ToggleImmersiveCallback, 0x6B);
}

static void ToggleNotificationsFlyout(void)
{
    PostMessageW(FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL), WM_HOTKEY, 591, 0);
}

static void ToggleActionCenter(void)
{
    PostMessageW(FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL), WM_HOTKEY, 500, MAKELPARAM(MOD_WIN, 0x41));
}

#ifdef _WIN64
static void ToggleLauncherTipContextMenu(void)
{
    if (hIsWinXShown) {
        SendMessage(hWinXWnd, WM_CLOSE, 0, 0);
    } else {
        HWND hWnd = FindWindowEx(NULL, NULL, L"Shell_TrayWnd", NULL);
        if (hWnd) {
            hWnd = FindWindowEx(hWnd, NULL, L"Start", NULL);
            if (hWnd) {
                POINT pt = GetDefaultWinXPosition(FALSE, NULL, NULL, TRUE, FALSE);
                // Finally implemented a variation of
                // https://github.com/valinet/ExplorerPatcher/issues/3
                // inspired by how the real Start button activates this menu
                // (CPearl::_GetLauncherTipContextMenu)
                // This also works when auto hide taskbar is on (#63)
                IUnknown *pImmersiveShell = NULL;
                HRESULT   hr = CoCreateInstance(&CLSID_ImmersiveShell, NULL, CLSCTX_INPROC_SERVER, &IID_IServiceProvider, &pImmersiveShell );

                if (SUCCEEDED(hr)) {
                    IImmersiveMonitorService *pMonitorService = NULL;
                    IUnknown_QueryService(pImmersiveShell, &SID_IImmersiveMonitorService, &IID_IImmersiveMonitorService, &pMonitorService);
                    if (pMonitorService) {
                        ILauncherTipContextMenu *pMenu = NULL;
                        pMonitorService->lpVtbl->QueryServiceFromWindow(pMonitorService, hWnd, &IID_ILauncherTipContextMenu, &IID_ILauncherTipContextMenu, &pMenu);
                        if (pMenu) {
                            pMenu->lpVtbl->ShowLauncherTipContextMenu(pMenu, &pt);
                            pMenu->lpVtbl->Release(pMenu);
                        }
                        pMonitorService->lpVtbl->Release(pMonitorService);
                    }
                    pImmersiveShell->lpVtbl->Release(pImmersiveShell);
                }
            }
        }
    }
}
#endif
#pragma endregion


#pragma region "windowsudk.shellcommon Hooks"

static HRESULT WINAPI
windowsudkshellcommon_SLGetWindowsInformationDWORDHook(PCWSTR pwszValueName, DWORD *pdwValue)
{
    HRESULT hr = SLGetWindowsInformationDWORDFunc(pwszValueName, pdwValue);

    if (bDisableAeroSnapQuadrants && !wcsncmp(pwszValueName, L"Shell-Windowing-LimitSnappedWindows", 35))
        *pdwValue = 1;

    return hr;
}

#pragma endregion


#pragma region "twinui.pcshell.dll hooks"
#ifdef _WIN64
# define LAUNCHERTIP_CLASS_NAME L"LauncherTipWnd"

LRESULT CALLBACK CLauncherTipContextMenu_WndProc(
    _In_ HWND   hWnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    LRESULT result;

    if (hWnd == hArchivehWnd &&
        !ArchiveMenuWndProc(hWnd, uMsg, wParam, lParam,
                            ImmersiveContextMenuHelper_ApplyOwnerDrawToMenuFunc,
                            ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc))
    {
        return 0;
    }

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT *pCs = (CREATESTRUCT *)lParam;
        if (pCs->lpCreateParams) {
            *((HWND *)((char *)pCs->lpCreateParams + 0x78)) = hWnd;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, pCs->lpCreateParams);
            result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
        } else {
            result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
            // result = 0;
        }
    } else {
        void *_this = GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        BOOL  v12   = FALSE;
        if ((uMsg == WM_DRAWITEM || uMsg == WM_MEASUREITEM) &&
            CImmersiveContextMenuOwnerDrawHelper_s_ContextMenuWndProcFunc &&
            CImmersiveContextMenuOwnerDrawHelper_s_ContextMenuWndProcFunc(hWnd, uMsg, wParam, lParam, &v12))
        {
            result = 0;
        } else {
            result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
        if (_this) {
            if (uMsg == WM_NCDESTROY) {
                SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
                *((HWND *)((char *)_this + 0x78)) = 0;
            }
        }
    }
    return result;
}

typedef struct {
    void     *_this;
    POINT     point;
    IUnknown *iunk;
    BOOL      bShouldCenterWinXHorizontally;
} ShowLauncherTipContextMenuParameters;

static DWORD ShowLauncherTipContextMenu(ShowLauncherTipContextMenuParameters *params)
{
    static ATOM windowRegistrationAtom = 0;
    if (windowRegistrationAtom == 0) {
        WNDCLASSW wc = {
            .style         = CS_DBLCLKS,
            .lpfnWndProc   = CLauncherTipContextMenu_WndProc,
            .hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH),
            .hInstance     = GetModuleHandleW(NULL),
            .lpszClassName = LAUNCHERTIP_CLASS_NAME,
            .hCursor       = LoadCursorW(NULL, IDC_ARROW),
        };

        ATOM tmp = RegisterClassW(&wc);
        if (tmp != 0)
            windowRegistrationAtom = tmp;
    }

    // Adjust this based on info from: CLauncherTipContextMenu::SetSite
    // and CLauncherTipContextMenu::CLauncherTipContextMenu
    // 22000.739: 0xE8
    // 22000.778: 0xF0
    // What has happened, between .739 and .778 is that the launcher tip
    // context menu object now implements a new interface, ILauncherTipContextMenuMigration;
    // thus, members have shifted 8 bytes (one 64-bit value which will hold the
    // address of the vtable for this intf at runtime) to the right;
    // all this intf seems to do, as of now, is to remove some "obsolete" links
    // from the menu (check out "CLauncherTipContextMenu::RunMigrationTasks"); it
    // seems you can disable this by setting a DWORD "WinXMigrationLevel" = 1 in
    // HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Advanced
    size_t offset_in_class = 0;
    if (global_rovi.dwBuildNumber >= 22621 || (global_rovi.dwBuildNumber == 22000 && global_ubr >= 778))
        offset_in_class = 8;

    hWinXWnd = CreateWindowInBand(
        0, windowRegistrationAtom, NULL, WS_POPUP,
        0, 0, 0, 0,
        NULL, NULL, GetModuleHandleW(NULL),
        (LPVOID)((char *)params->_this - 0x58),
        7
    );
    // DO NOT USE ShowWindow here; it breaks the window order
    // and renders the desktop toggle unusable; but leave
    // SetForegroundWindow as is so that the menu gets dismissed
    // when the user clicks outside it
    //
    // ShowWindow(hWinXWnd, SW_SHOW);
    SetForegroundWindow(hWinXWnd);
    HMENU hMenu;

    while (!(hMenu = *(HMENU *)((char *)params->_this + 0xE8 + offset_in_class)))
        Sleep(1);
    if (!hMenu)
        goto finalize;

    wchar_t buffer[260];
    LoadStringW(GetModuleHandleW(L"ExplorerFrame.dll"), 50222, buffer + (bNoMenuAccelerator ? 0 : 1), 260);
    if (!bNoMenuAccelerator)
        buffer[0] = L'&';
    wchar_t *p = wcschr(buffer, L'(');
    if (p) {
        p--;
        if (*p == L' ') {
            *p = 0;
        } else {
            p++;
            *p = 0;
        }
    }

    MENUITEMINFOW menuInfo = {
        .cbSize     = sizeof menuInfo,
        .fMask      = MIIM_ID | MIIM_STRING | MIIM_DATA,
        .wID        = 3999,
        .dwItemData = 0,
        .fType      = MFT_STRING,
        .dwTypeData = buffer,
        .cch        = wcslen(buffer),
    };
    BOOL bCreatedMenu = FALSE;

    if (bPropertiesInWinX) {
        InsertMenuItemW(hMenu, GetMenuItemCount(hMenu) - 1, TRUE, &menuInfo);
        bCreatedMenu = TRUE;
    }

    INT64 *unknown_array = NULL;
    if (bSkinMenus) {
        unknown_array = calloc(4, sizeof(INT64));
        if (ImmersiveContextMenuHelper_ApplyOwnerDrawToMenuFunc)
            ImmersiveContextMenuHelper_ApplyOwnerDrawToMenuFunc(hMenu, hWinXWnd, &(params->point), 0xC, unknown_array);
    }

    BOOL res = TrackPopupMenu(
        hMenu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON | (params->bShouldCenterWinXHorizontally ? TPM_CENTERALIGN : 0),
        params->point.x, params->point.y, 0, hWinXWnd, NULL
    );

    if (bSkinMenus) {
        if (ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc)
            ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc(hMenu, hWinXWnd, &(params->point));
        free(unknown_array);
    }

    if (bCreatedMenu)
        RemoveMenu(hMenu, 3999, MF_BYCOMMAND);

    if (res > 0) {
        if (bCreatedMenu && res == 3999) {
            LaunchPropertiesGUI(hModule);
        } else if (res < 4000) {
            INT64 info = *(INT64 *)((char *)(*(INT64 *)((char *)params->_this + 0xA8 + offset_in_class - 0x58)) + (INT64)res * 8 - 8);
            if (CLauncherTipContextMenu_ExecuteCommandFunc)
                CLauncherTipContextMenu_ExecuteCommandFunc((char *)params->_this - 0x58, &info);
        } else {
            INT64 info = *(INT64 *)((char *)(*(INT64 *)((char *)params->_this + 0xC8 + offset_in_class - 0x58)) + ((INT64)res - 4000) * 8);
            if (CLauncherTipContextMenu_ExecuteShutdownCommandFunc)
                CLauncherTipContextMenu_ExecuteShutdownCommandFunc((char *)params->_this - 0x58, &info);
        }
    }

finalize:
    params->iunk->lpVtbl->Release(params->iunk);
    SendMessageW(hWinXWnd, WM_CLOSE, 0, 0);
    free(params);
    hIsWinXShown = NULL;
    return 0;
}

static INT64 CLauncherTipContextMenu_ShowLauncherTipContextMenuHook(void *_this, POINT *pt)
{
    if (hWinXThread) {
        WaitForSingleObject(hWinXThread, INFINITE);
        CloseHandle(hWinXThread);
        hWinXThread = NULL;
    }
    if (hIsWinXShown)
        goto finalize;

    BOOL  bShouldCenterWinXHorizontally = FALSE;
    POINT point;
    if (pt) {
        point = *pt;
        BOOL  bBottom, bRight;
        POINT dPt = GetDefaultWinXPosition(FALSE, &bBottom, &bRight, FALSE, FALSE);
        POINT posCursor;
        GetCursorPos(&posCursor);
        RECT rcHitZone = {
            .left   = pt->x - 5,
            .right  = pt->x + 5,
            .top    = pt->y - 5,
            .bottom = pt->y + 5,
        };

        // printf("%d %d = %d %d %d %d\n", posCursor.x, posCursor.y, rcHitZone.left, rcHitZone.right,
        // rcHitZone.top, rcHitZone.bottom);

        if (bBottom && IsThemeActive() && PtInRect(&rcHitZone, posCursor) &&
            GetClassWord(WindowFromPoint(point), GCW_ATOM) == RegisterWindowMessageW(L"Start"))
        {
            HMONITOR    hMonitor = MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO mi;
            mi.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(hMonitor, &mi);
            HWND  hWndUnder = WindowFromPoint(*pt);
            WCHAR wszClassName[100];
            ZeroMemory(wszClassName, 100);
            GetClassNameW(hWndUnder, wszClassName, 100);
            if (WStrEq(wszClassName, L"Shell_TrayWnd") || WStrEq(wszClassName, L"Shell_SecondaryTrayWnd"))
                hWndUnder = FindWindowEx(hWndUnder, NULL, L"Start", NULL);
            RECT rcUnder;
            GetWindowRect(hWndUnder, &rcUnder);

            if (mi.rcMonitor.left != rcUnder.left) {
                bShouldCenterWinXHorizontally = TRUE;
                point.x = rcUnder.left + (rcUnder.right - rcUnder.left) / 2;
                point.y = rcUnder.top;
            } else {
                UINT    dpiX, dpiY;
                HRESULT hr = GetDpiForMonitor(hMonitor, MDT_DEFAULT, &dpiX, &dpiY);
                double  dx = dpiX / 96.0, dy = dpiY / 96.0;
                BOOL    xo = FALSE, yo = FALSE;
                if (point.x - WINX_ADJUST_X * dx < mi.rcMonitor.left)
                    xo = TRUE;
                if (point.y + WINX_ADJUST_Y * dy > mi.rcMonitor.bottom)
                    yo = TRUE;
                POINT ptCursor;
                GetCursorPos(&ptCursor);
                if (xo)
                    ptCursor.x += (WINX_ADJUST_X * 2) * dx;
                else
                    point.x -= WINX_ADJUST_X * dx;
                if (yo)
                    ptCursor.y -= (WINX_ADJUST_Y * 2) * dy;
                else
                    point.y += WINX_ADJUST_Y * dy;
                SetCursorPos(ptCursor.x, ptCursor.y);
            }
        }
    } else {
        point = GetDefaultWinXPosition(FALSE, NULL, NULL, TRUE, FALSE);
    }

    IUnknown *iunk = NULL;
    if (CLauncherTipContextMenu_GetMenuItemsAsyncFunc)
        CLauncherTipContextMenu_GetMenuItemsAsyncFunc(_this, &point, &iunk);
    if (iunk) {
        iunk->lpVtbl->AddRef(iunk);

        ShowLauncherTipContextMenuParameters *params = malloc(sizeof(ShowLauncherTipContextMenuParameters));
        params->bShouldCenterWinXHorizontally = bShouldCenterWinXHorizontally;
        params->_this = _this;
        params->point = point;
        params->iunk  = iunk;

        hIsWinXShown = CreateThread(0, 0, ShowLauncherTipContextMenu, params, 0, 0);
        hWinXThread  = hIsWinXShown;
    }
finalize:
    if (CLauncherTipContextMenu_ShowLauncherTipContextMenuFunc)
        return CLauncherTipContextMenu_ShowLauncherTipContextMenuFunc(_this, pt);

    return 0;
}
#endif
#pragma endregion


#pragma region "Windows 10 Taskbar Hooks"
#ifdef _WIN64
// credits: https://github.com/m417z/7-Taskbar-Tweaker

DEFINE_GUID(IID_ITaskGroup,
            0x3AF85589, 0x678F, 0x4FB5, 0x89, 0x25, 0x5A, 0x13, 0x4E, 0xBF, 0x57, 0x2C);

typedef interface ITaskGroup ITaskGroup;
typedef struct ITaskGroupVtbl {
    BEGIN_INTERFACE

    HRESULT(STDMETHODCALLTYPE *QueryInterface)
    (ITaskGroup       *This,
     /* [in] */ REFIID riid,
     /* [annotation][iid_is][out] */
     _COM_Outptr_ void **ppvObject);

    ULONG(STDMETHODCALLTYPE *AddRef)(ITaskGroup *This);
    ULONG(STDMETHODCALLTYPE *Release)(ITaskGroup *This);
    HRESULT(STDMETHODCALLTYPE *Initialize)(ITaskGroup *This);
    HRESULT(STDMETHODCALLTYPE *AddTaskItem)(ITaskGroup *This);
    HRESULT(STDMETHODCALLTYPE *RemoveTaskItem)(ITaskGroup *This);
    HRESULT(STDMETHODCALLTYPE *EnumTaskItems)(ITaskGroup *This);
    HRESULT(STDMETHODCALLTYPE *DoesWindowMatch)
    (ITaskGroup *This,
     HWND        hCompareWnd,
     ITEMIDLIST *pCompareItemIdList,
     WCHAR      *pCompareAppId,
     int        *pnMatch,
     LONG      **p_task_item);
    // ...

    END_INTERFACE
} ITaskGroupVtbl;

interface ITaskGroup {
    CONST_VTBL struct ITaskGroupVtbl *lpVtbl;
};

static HRESULT __stdcall CTaskGroup_DoesWindowMatchHook(
    LONG_PTR   *task_group,
    HWND        hCompareWnd,
    ITEMIDLIST *pCompareItemIdList,
    WCHAR      *pCompareAppId,
    int        *pnMatch,
    LONG_PTR  **p_task_item
)
{
    HRESULT hr = CTaskGroup_DoesWindowMatchFunc(task_group, hCompareWnd, pCompareItemIdList, pCompareAppId, pnMatch, p_task_item);
    BOOL bDontGroup = FALSE;
    BOOL bPinned    = FALSE;
    if (bPinnedItemsActAsQuickLaunch && SUCCEEDED(hr) && *pnMatch >= 1 && *pnMatch <= 3) // itemlist or appid match
    {
        bDontGroup = FALSE;
        bPinned    = (!task_group[4] || (int)((LONG_PTR *)task_group[4])[0] == 0);
        if (bPinned)
            bDontGroup = TRUE;
        if (bDontGroup)
            hr = E_FAIL;
    }
    return hr;
}

DEFINE_GUID(IID_ITaskBtnGroup,
            0x2E52265D, 0x1A3B, 0x4E46, 0x94, 0x17, 0x51, 0xA5, 0x9C, 0x47, 0xD6, 0x0B);

typedef struct ITaskBtnGroupVtbl {
    BEGIN_INTERFACE

    HRESULT(STDMETHODCALLTYPE *QueryInterface)
    (ITaskBtnGroup    *This,
     /* [in] */ REFIID riid,
     /* [annotation][iid_is][out] */
     _COM_Outptr_ void **ppvObject);

    ULONG(STDMETHODCALLTYPE *AddRef)(ITaskBtnGroup *This);
    ULONG(STDMETHODCALLTYPE *Release)(ITaskBtnGroup *This);
    HRESULT(STDMETHODCALLTYPE *Shutdown)(ITaskBtnGroup *This);
    HRESULT(STDMETHODCALLTYPE *GetGroupType)(ITaskBtnGroup *This);
    HRESULT(STDMETHODCALLTYPE *UpdateGroupType)(ITaskBtnGroup *This);
    HRESULT(STDMETHODCALLTYPE *GetGroup)(ITaskBtnGroup *This);
    HRESULT(STDMETHODCALLTYPE *AddTaskItem)(ITaskBtnGroup *This);
    HRESULT(STDMETHODCALLTYPE *IndexOfTaskItem)(ITaskBtnGroup *This);
    HRESULT(STDMETHODCALLTYPE *RemoveTaskItem)(ITaskBtnGroup *This);
    HRESULT(STDMETHODCALLTYPE *RealityCheck)(ITaskBtnGroup *This);
    HRESULT(STDMETHODCALLTYPE *IsItemBeingRemoved)(ITaskBtnGroup *This);
    HRESULT(STDMETHODCALLTYPE *CancelRemoveItem)(ITaskBtnGroup *This);
    LONG_PTR(STDMETHODCALLTYPE *GetIdealSpan)
    (ITaskBtnGroup *This, LONG_PTR var2, LONG_PTR var3, LONG_PTR var4, LONG_PTR var5, LONG_PTR var6);
    // ...

    END_INTERFACE
} ITaskBtnGroupVtbl;

interface ITaskBtnGroup {
    CONST_VTBL struct ITaskBtnGroupVtbl *lpVtbl;
};

static LONG_PTR __stdcall CTaskBtnGroup_GetIdealSpanHook(
    ITaskBtnGroup *_this,
    LONG_PTR       var2,
    LONG_PTR       var3,
    LONG_PTR       var4,
    LONG_PTR       var5,
    LONG_PTR       var6
)
{
    LONG_PTR ret               = NULL;
    BOOL     bTypeModified     = FALSE;
    int      button_group_type = *(unsigned *)((INT64)_this + 64);
    if (bRemoveExtraGapAroundPinnedItems && button_group_type == 2) {
        *(unsigned *)((INT64)_this + 64) = 4;
        bTypeModified = TRUE;
    }
    ret = CTaskBtnGroup_GetIdealSpanFunc(_this, var2, var3, var4, var5, var6);
    if (bRemoveExtraGapAroundPinnedItems && bTypeModified)
        *(unsigned *)((INT64)_this + 64) = button_group_type;
    return ret;
}

static HRESULT explorer_QISearch(void *that, LPCQITAB pqit, REFIID riid, void **ppv)
{
    HRESULT hr = QISearch(that, pqit, riid, ppv);
    if (SUCCEEDED(hr) && IsEqualGUID(pqit[0].piid, &IID_ITaskGroup) && bPinnedItemsActAsQuickLaunch) {
        ITaskGroup *pTaskGroup   = (char *)that + pqit[0].dwOffset;
        DWORD       flOldProtect = 0;
        if (VirtualProtect(pTaskGroup->lpVtbl, sizeof(ITaskGroupVtbl), PAGE_EXECUTE_READWRITE, &flOldProtect)) {
            if (!CTaskGroup_DoesWindowMatchFunc)
                CTaskGroup_DoesWindowMatchFunc = pTaskGroup->lpVtbl->DoesWindowMatch;
            pTaskGroup->lpVtbl->DoesWindowMatch = CTaskGroup_DoesWindowMatchHook;
            VirtualProtect(pTaskGroup->lpVtbl, sizeof(ITaskGroupVtbl), flOldProtect, &flOldProtect);
        }
    } else if (SUCCEEDED(hr) && IsEqualGUID(pqit[0].piid, &IID_ITaskBtnGroup) && bRemoveExtraGapAroundPinnedItems) {
        ITaskBtnGroup *pTaskBtnGroup = (char *)that + pqit[0].dwOffset;
        DWORD          flOldProtect  = 0;
        if (VirtualProtect(pTaskBtnGroup->lpVtbl, sizeof(ITaskBtnGroupVtbl), PAGE_EXECUTE_READWRITE, &flOldProtect)) {
            if (!CTaskBtnGroup_GetIdealSpanFunc)
                CTaskBtnGroup_GetIdealSpanFunc = pTaskBtnGroup->lpVtbl->GetIdealSpan;
            pTaskBtnGroup->lpVtbl->GetIdealSpan = CTaskBtnGroup_GetIdealSpanHook;
            VirtualProtect(pTaskBtnGroup->lpVtbl, sizeof(ITaskBtnGroupVtbl), flOldProtect, &flOldProtect);
        }
    }
    return hr;
}
#endif
#pragma endregion


#pragma region "Show Start in correct location according to TaskbarAl"
#ifdef _WIN64
static void UpdateStartMenuPositioning(LPARAM loIsShouldInitializeArray_hiIsShouldRoInitialize)
{
    BOOL bShouldInitialize   = LOWORD(loIsShouldInitializeArray_hiIsShouldRoInitialize);
    BOOL bShouldRoInitialize = HIWORD(loIsShouldInitializeArray_hiIsShouldRoInitialize);

    if (!bOldTaskbar)
        return;

    DWORD dwPosCurrent = GetStartMenuPosition(SHRegGetValueFromHKCUHKLMFunc);
    if (bShouldInitialize || InterlockedAdd(&dwTaskbarAl, 0) != dwPosCurrent) {
        HRESULT hr = S_OK;
        if (bShouldRoInitialize)
            hr = RoInitialize(RO_INIT_MULTITHREADED);
        if (SUCCEEDED(hr)) {
            InterlockedExchange(&dwTaskbarAl, dwPosCurrent);
            StartMenuPositioningData spd = {
                .pMonitorCount = &dwMonitorCount,
                .pMonitorList  = hMonitorList,
                .location      = dwPosCurrent,
            };

            if (bShouldInitialize) {
                spd.operation = STARTMENU_POSITIONING_OPERATION_REMOVE;
                unsigned k    = InterlockedAdd(&dwMonitorCount, 0);
                for (unsigned i = 0; i < k; ++i)
                    NeedsRo_PositionStartMenuForMonitor(hMonitorList[i], NULL, NULL, &spd);
                InterlockedExchange(&dwMonitorCount, 0);
                spd.operation = STARTMENU_POSITIONING_OPERATION_ADD;
            } else {
                spd.operation = STARTMENU_POSITIONING_OPERATION_CHANGE;
            }
            EnumDisplayMonitors(NULL, NULL, NeedsRo_PositionStartMenuForMonitor, &spd);
            if (bShouldRoInitialize)
                RoUninitialize();
        }
    }
}
#else
void UpdateStartMenuPositioning(LPARAM loIsShouldInitializeArray_hiIsShouldRoInitialize)
{
}
#endif
#pragma endregion


#pragma region "Fix Windows 11 taskbar not showing tray when auto hide is on"
#ifdef _WIN64
# define FIXTASKBARAUTOHIDE_CLASS_NAME L"FixTaskbarAutohide_" EP_CLSID

static LRESULT CALLBACK FixTaskbarAutohide_WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    static UINT s_uTaskbarRestart;

    switch (uMessage) {
    case WM_CREATE:
        s_uTaskbarRestart = RegisterWindowMessageW(L"TaskbarCreated");
        break;

    default:
        if (uMessage == s_uTaskbarRestart)
            PostQuitMessage(0);
        break;
    }

    return DefWindowProcW(hWnd, uMessage, wParam, lParam);
}

static DWORD FixTaskbarAutohide(DWORD unused)
{
    WNDCLASSW wc = {
        .style         = CS_DBLCLKS,
        .lpfnWndProc   = FixTaskbarAutohide_WndProc,
        .hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH),
        .hInstance     = GetModuleHandleW(NULL),
        .lpszClassName = FIXTASKBARAUTOHIDE_CLASS_NAME,
        .hCursor       = LoadCursorW(NULL, IDC_ARROW),
    };
    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(0, FIXTASKBARAUTOHIDE_CLASS_NAME, NULL, WS_POPUP,
                                0, 0, 0, 0, NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (hWnd) {
        MSG  msg;
        BOOL bRet;
        while ((bRet = GetMessageW(&msg, NULL, 0, 0)) != 0) {
            if (bRet == -1) {
                break;
            } else {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
        DestroyWindow(hWnd);

        APPBARDATA abd;
        abd.cbSize = sizeof(APPBARDATA);
        if (SHAppBarMessage(ABM_GETSTATE, &abd) == ABS_AUTOHIDE) {
            abd.lParam = 0;
            SHAppBarMessage(ABM_SETSTATE, &abd);
            Sleep(1000);
            abd.lParam = ABS_AUTOHIDE;
            SHAppBarMessage(ABM_SETSTATE, &abd);
        }
    }
    SetEvent(hCanStartSws);
    return 0;
}

#endif
#pragma endregion


#pragma region "EnsureXAML on OS builds 22621+"
#ifdef _WIN64
DEFINE_GUID(uuidof_Windows_Internal_Shell_XamlExplorerHost_IXamlApplicationStatics,
            0x5148D7B1, 0x800E, 0x5C86, 0x8F, 0x69, 0x55, 0x81, 0x97, 0x48, 0x31, 0x23);
DEFINE_GUID(uuidof_Windows_UI_Core_ICoreWindow5,
            0x28258A12, 0x7D82, 0x505B, 0xB2, 0x10, 0x71, 0x2B, 0x04, 0xA5, 0x88, 0x82);


static HRESULT WINAPI ICoreWindow5_get_DispatcherQueueHook(void *_this, void **ppValue)
{
    static unsigned char flg = 0;
    if (!_InterlockedExchange8(&flg, 1))
        SendMessageTimeoutW(FindWindowW(L"Shell_TrayWnd", NULL),
                            WM_SETTINGCHANGE, 0, L"EnsureXAML", SMTO_NOTIMEOUTIFNOTHUNG, INFINITE, NULL);
    return ICoreWindow5_get_DispatcherQueueFunc(_this, ppValue);
}

typedef void (__fastcall *GetActivationFactory_func_t)(HSTRING, IActivationFactory *);

static HMODULE __fastcall Windows11v22H2_combase_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    //return LoadLibraryExW(lpLibFileName, hFile, dwFlags);
    static unsigned char flg = 0;

    HMODULE hModule = LoadLibraryExW(lpLibFileName, hFile, dwFlags);

    if (hModule && hModule == GetModuleHandleW(L"Windows.Ui.Xaml.dll")) {
        if (_InterlockedExchange8(&flg, 1))
            return hModule;

        HSTRING_HEADER hstringHeaderWindowsXamlManager;
        HSTRING        hstringWindowsXamlManager = NULL;
        GetActivationFactory_func_t DllGetActivationFactory =
            (GetActivationFactory_func_t)GetProcAddress(hModule, "DllGetActivationFactory");

        if (!DllGetActivationFactory) {
            wprintf(L"Error in Windows11v22H2_combase_LoadLibraryExW on DllGetActivationFactory\n");
            return hModule;
        }
        if (FAILED(WindowsCreateStringReference(L"Windows.UI.Xaml.Hosting.WindowsXamlManager", 0x2Au,
                                                &hstringHeaderWindowsXamlManager, &hstringWindowsXamlManager)))
        {
            wprintf(L"Error in Windows11v22H2_combase_LoadLibraryExW on WindowsCreateStringReference\n");
            return hModule;
        }

        IActivationFactory *pWindowsXamlManagerFactory = NULL;
        DllGetActivationFactory(hstringWindowsXamlManager, &pWindowsXamlManagerFactory);

        if (pWindowsXamlManagerFactory) {
            IInspectable *pCoreWindow5 = NULL;
            IActivationFactory_QueryInterface(pWindowsXamlManagerFactory, &uuidof_Windows_UI_Core_ICoreWindow5, &pCoreWindow5);

            if (pCoreWindow5) {
                INT64 *pCoreWindow5Vtbl = pCoreWindow5->lpVtbl;
                DWORD  flOldProtect     = 0;

                if (VirtualProtect(pCoreWindow5->lpVtbl, sizeof(IInspectableVtbl) + sizeof(INT64), PAGE_EXECUTE_READWRITE, &flOldProtect))
                {
                    ICoreWindow5_get_DispatcherQueueFunc = pCoreWindow5Vtbl[6];
                    pCoreWindow5Vtbl[6]                  = ICoreWindow5_get_DispatcherQueueHook;
                    VirtualProtect(pCoreWindow5->lpVtbl, sizeof(IInspectableVtbl) + sizeof(INT64), flOldProtect, &flOldProtect);
                }
                IInspectable_Release(pCoreWindow5);
            }

            IActivationFactory_Release(pWindowsXamlManagerFactory);
        }

        WindowsDeleteString(hstringWindowsXamlManager);
    }

    return hModule;
}

#endif
#pragma endregion


#pragma region "Shell_TrayWnd subclass"
#ifdef _WIN64

static int HandleTaskbarCornerInteraction(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    POINT pt;

    if (uMsg == WM_RBUTTONUP || uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONDOWN || uMsg == WM_LBUTTONDOWN) {
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ClientToScreen(hWnd, &pt);
    } else if (uMsg == WM_NCLBUTTONUP || uMsg == WM_NCRBUTTONUP || uMsg == WM_NCLBUTTONDOWN || uMsg == WM_NCRBUTTONDOWN) {
        DWORD dwPos = GetMessagePos();
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
    } else {
        pt = (POINT){0, 0};
    }

    HMONITOR    hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi       = {.cbSize = sizeof(MONITORINFO)};
    GetMonitorInfoW(hMonitor, &mi);

    int  t   = 2;
    BOOL bOk = FALSE;
    if (pt.x < mi.rcMonitor.left + t && pt.y > mi.rcMonitor.bottom - t) {
        // printf("bottom left\n");
        bOk = TRUE;
    } else if (pt.x < mi.rcMonitor.left + t && pt.y < mi.rcMonitor.top + t) {
        // printf("top left\n");
        bOk = TRUE;
    } else if (pt.x > mi.rcMonitor.right - t && pt.y < mi.rcMonitor.top + t) {
        // printf("top right\n");
        bOk = TRUE;
    }

    if (bOk) {
        if (uMsg == WM_RBUTTONUP || uMsg == WM_NCRBUTTONUP || uMsg == WM_RBUTTONDOWN || uMsg == WM_NCRBUTTONDOWN) {
            ToggleLauncherTipContextMenu();
            return 1;
        } else if (uMsg == WM_LBUTTONUP || uMsg == WM_NCLBUTTONUP || uMsg == WM_LBUTTONDOWN || uMsg == WM_NCLBUTTONDOWN) {
            if (!dwUndeadStartCorner)
                return 1;
            if (dwUndeadStartCorner != 2) {
                OpenStartOnMonitor(hMonitor);
                return 1;
            }
            DWORD dwVal = 1, dwSize = sizeof(DWORD);
            RegGetValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                         L"TaskbarAl", RRF_RT_DWORD, NULL, &dwVal, &dwSize);
            if (dwVal) {
                dwVal = 0;
                RegSetKeyValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                                L"TaskbarAl", REG_DWORD, &dwVal, sizeof(DWORD));
                if (hWndServiceWindow)
                    SetTimer(hWndServiceWindow, 10, 1000, NULL);
            }
            OpenStartOnMonitor(hMonitor);
            return 1;
        }
    }
    return 0;
}

static INT64 ReBarWindow32SubclassProc(
    _In_ HWND   hWnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    UINT_PTR    uIdSubclass,
    DWORD_PTR   dwUnused
)
{
    if (uMsg == WM_NCDESTROY)
        RemoveWindowSubclass(hWnd, ReBarWindow32SubclassProc, ReBarWindow32SubclassProc);

    if (TaskbarCenter_ShouldCenter(dwOldTaskbarAl) &&
        TaskbarCenter_ShouldStartBeCentered(dwOldTaskbarAl) &&
        uMsg == WM_WINDOWPOSCHANGING)
    {
        LPWINDOWPOS lpWP = (LPWINDOWPOS)lParam;
        lpWP->cx += lpWP->x;
        lpWP->x = 0;
        lpWP->cy += lpWP->y;
        lpWP->y = 0;
    } else if (uMsg == RB_INSERTBANDW) {
        REBARBANDINFOW *lpRbi = lParam;
    } else if (uMsg == RB_SETBANDINFOW) {
        REBARBANDINFOW *lpRbi = lParam;
        if (GetClassWord(lpRbi->hwndChild, GCW_ATOM) == RegisterWindowMessageW(L"PeopleBand")) {
            lpRbi->fMask |= RBBIM_STYLE;
            lpRbi->fStyle &= ~RBBS_FIXEDSIZE;
            // lpRbi->fStyle &= ~RBBS_NOGRIPPER;
        }
    } else if (TaskbarCenter_ShouldCenter(dwOldTaskbarAl) &&
               TaskbarCenter_ShouldStartBeCentered(dwOldTaskbarAl) &&
               (uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONUP) &&
               HandleTaskbarCornerInteraction(hWnd, uMsg, wParam, lParam))
    {
        return 0;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static HMENU explorer_LoadMenuW(HINSTANCE hInstance, LPCWSTR lpMenuName)
{
    HMENU hMenu = LoadMenuW(hInstance, lpMenuName);
    if (hInstance == GetModuleHandleW(NULL) && lpMenuName == MAKEINTRESOURCEW(205)) {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu) {
            WCHAR buffer[260];
            LoadStringW(GetModuleHandleW(L"ExplorerFrame.dll"), 50222, buffer + (bNoMenuAccelerator ? 0 : 1), 260);
            if (!bNoMenuAccelerator)
                buffer[0] = L'&';
            wchar_t *p = wcschr(buffer, L'(');
            if (p) {
                p--;
                if (*p == L' ') {
                    *p = 0;
                } else {
                    p++;
                    *p = 0;
                }
            }

            MENUITEMINFOW menuInfo = {
                .cbSize     = sizeof menuInfo,
                .fMask      = MIIM_ID | MIIM_STRING | MIIM_DATA,
                .wID        = 12100,
                .dwItemData = CheckForUpdatesThread,
                .fType      = MFT_STRING,
                .dwTypeData = buffer,
                .cch        = wcslen(buffer),
            };
            if (!bNoPropertiesInContextMenu)
                InsertMenuItemW(hSubMenu, GetMenuItemCount(hSubMenu) - 4, TRUE, &menuInfo);
        }
    }
    return hMenu;
}

static BOOL IsPointOnEmptyAreaOfNewTaskbar(POINT pt)
{
    IUIAutomation2       *pIUIAutomation2                 = NULL;
    IUIAutomationElement *pIUIAutomationElement           = NULL;
    BOOL                  bIsWindows11Version22H2OrHigher = IsWindows11Version22H2OrHigher();
    HWND hWnd     = NULL;
    BOOL bRet     = FALSE;
    BSTR elemName = NULL;
    BSTR elemType = NULL;

    HRESULT hr = CoCreateInstance(&CLSID_CUIAutomation8, NULL, CLSCTX_INPROC_SERVER, &IID_IUIAutomation2, &pIUIAutomation2);
    if (SUCCEEDED(hr))
        hr = pIUIAutomation2->lpVtbl->ElementFromPoint(pIUIAutomation2, pt, &pIUIAutomationElement);
    if (SUCCEEDED(hr) && bIsWindows11Version22H2OrHigher)
        hr = pIUIAutomationElement->lpVtbl->get_CurrentName(pIUIAutomationElement, &elemName);
    if (SUCCEEDED(hr) && bIsWindows11Version22H2OrHigher)
        hr = pIUIAutomationElement->lpVtbl->get_CurrentClassName(pIUIAutomationElement, &elemType);
    if (SUCCEEDED(hr) && bIsWindows11Version22H2OrHigher)
        bRet = elemName && elemType && elemName[0] == L'\0' && WStrEq(elemType, L"Taskbar.TaskbarFrameAutomationPeer");
    if (SUCCEEDED(hr) && !bIsWindows11Version22H2OrHigher)
        hr = pIUIAutomationElement->lpVtbl->get_CurrentNativeWindowHandle(pIUIAutomationElement, &hWnd);

    if (SUCCEEDED(hr) && !bIsWindows11Version22H2OrHigher) {
        WCHAR wszClassName[200];
        GetClassNameW(hWnd, wszClassName, 200);
        if (IsWindow(hWnd)) {
            if (WStrEq(wszClassName, L"Windows.UI.Input.InputSite.WindowClass")) {
                HWND hAncestor = GetAncestor(hWnd, GA_ROOT);
                HWND hWindow   = FindWindowExW(hAncestor, NULL, L"Windows.UI.Composition.DesktopWindowContentBridge", NULL);
                if (IsWindow(hWindow)) {
                    hWindow = FindWindowExW(hWindow, NULL, L"Windows.UI.Input.InputSite.WindowClass", NULL);
                    if (IsWindow(hWindow) && hWindow == hWnd)
                        bRet = TRUE;
                }
            } else if (WStrEq(wszClassName, L"MSTaskListWClass")) {
                IUIAutomationTreeWalker *pControlWalker     = NULL;
                IUIAutomationElement    *pTaskbarButton     = NULL;
                IUIAutomationElement    *pNextTaskbarButton = NULL;
                RECT rc;
                if (SUCCEEDED(hr))
                    hr = pIUIAutomation2->lpVtbl->get_RawViewWalker(pIUIAutomation2, &pControlWalker);
                if (SUCCEEDED(hr) && pControlWalker)
                    hr = pControlWalker->lpVtbl->GetFirstChildElement(pControlWalker, pIUIAutomationElement, &pTaskbarButton);

                BOOL bValid = TRUE, bFirst = TRUE;
                while (SUCCEEDED(hr) && pTaskbarButton) {
                    pControlWalker->lpVtbl->GetNextSiblingElement(pControlWalker, pTaskbarButton, &pNextTaskbarButton);
                    SetRect(&rc, 0, 0, 0, 0);
                    pTaskbarButton->lpVtbl->get_CurrentBoundingRectangle(pTaskbarButton, &rc);
                    if (bFirst) {
                        // Account for Start button as well
                        rc.left -= (rc.right - rc.left);
                        bFirst = FALSE;
                    }
                    // printf("PT %d %d RECT %d %d %d %d\n", pt.x, pt.y, rc.left, rc.top, rc.right, rc.bottom);
                    if (pNextTaskbarButton && PtInRect(&rc, pt))
                        bValid = FALSE;
                    pTaskbarButton->lpVtbl->Release(pTaskbarButton);
                    pTaskbarButton = pNextTaskbarButton;
                }
                // printf("IS VALID %d\n\n", bValid);
                if (pControlWalker)
                    pControlWalker->lpVtbl->Release(pControlWalker);
                if (bValid) {
                    HWND hAncestor = GetAncestor(hWnd, GA_ROOT);
                    HWND hWindow   = FindWindowExW(hAncestor, NULL, L"WorkerW", NULL);
                    if (IsWindow(hWindow)) {
                        hWindow = FindWindowExW(hWindow, NULL, L"MSTaskListWClass", NULL);
                        if (IsWindow(hWindow) && hWindow == hWnd)
                            bRet = TRUE;
                    }
                }
            }
        }
    }

    if (elemName)
        SysFreeString(elemName);
    if (elemType)
        SysFreeString(elemType);
    if (pIUIAutomationElement)
        pIUIAutomationElement->lpVtbl->Release(pIUIAutomationElement);
    if (pIUIAutomation2)
        pIUIAutomation2->lpVtbl->Release(pIUIAutomation2);
    return bRet;
}

static LRESULT CALLBACK Shell_TrayWndMouseProc(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    static BOOL      bTaskbarLeftClickEven = FALSE;
    static long long TaskbarLeftClickTime  = 0;

    if (!bOldTaskbar && !bNoPropertiesInContextMenu &&
        nCode == HC_ACTION && wParam == WM_RBUTTONUP &&
        IsPointOnEmptyAreaOfNewTaskbar(((MOUSEHOOKSTRUCT *)lParam)->pt))
    {
        PostMessageW(FindWindowW(L"Shell_TrayWnd", NULL),
                     RegisterWindowMessageW(L"Windows11ContextMenu_" EP_CLSID),
                     0, MAKELPARAM(((MOUSEHOOKSTRUCT *)lParam)->pt.x,
                                   ((MOUSEHOOKSTRUCT *)lParam)->pt.y));
        return 1;
    }

    if (!bOldTaskbar && bTaskbarAutohideOnDoubleClick &&
        nCode == HC_ACTION && wParam == WM_LBUTTONUP &&
        IsPointOnEmptyAreaOfNewTaskbar(((MOUSEHOOKSTRUCT *)lParam)->pt))
    {
# if 0
        BOOL bShouldCheck = FALSE;
        if (bOldTaskbar)
        {
            WCHAR cn[200];
            GetClassNameW(((MOUSEHOOKSTRUCT*)lParam)->hwnd, cn, 200);
            wprintf(L"%s\n", cn);
            bShouldCheck = WStrEq(cn, L"Shell_SecondaryTrayWnd"); // WStrEq(cn, L"Shell_TrayWnd")
        }
        else
        {
            bShouldCheck = IsPointOnEmptyAreaOfNewTaskbar(((MOUSEHOOKSTRUCT*)lParam)->pt);
        }
        if (bShouldCheck)
        {
# endif
        if (bTaskbarLeftClickEven) {
            if (TaskbarLeftClickTime != 0)
                TaskbarLeftClickTime = milliseconds_now() - TaskbarLeftClickTime;
            if (TaskbarLeftClickTime != 0 && TaskbarLeftClickTime < GetDoubleClickTime()) {
                TaskbarLeftClickTime = 0;
                ToggleTaskbarAutohide();
            } else {
                TaskbarLeftClickTime = milliseconds_now();
            }
        }
        bTaskbarLeftClickEven = !bTaskbarLeftClickEven;
# if 0
        }
# endif
    }

    return CallNextHookEx(hShell_TrayWndMouseHook, nCode, wParam, lParam);
}

static void Shell_handleWin11Menu(HWND hWnd, LPARAM lParam)
{
    HMENU hMenu = LoadMenuW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(205));
    if (!hMenu)
        return;
    HMENU hSubMenu = GetSubMenu(hMenu, 0);
    if (!hSubMenu)
        return;

    if (GetAsyncKeyState(VK_SHIFT) >= 0 || GetAsyncKeyState(VK_CONTROL) >= 0)
        DeleteMenu(hSubMenu, 518, MF_BYCOMMAND); // Exit Explorer
    DeleteMenu(hSubMenu, 424, MF_BYCOMMAND);     // Lock the taskbar
    DeleteMenu(hSubMenu, 425, MF_BYCOMMAND);     // Lock all taskbars

    static const unsigned TRAYUI_OFFSET_IN_CTRAY = 110;

    HWND      hShellTray_Wnd = FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL);
    INT64    *CTrayInstance  = (BYTE *)GetWindowLongPtrW(hShellTray_Wnd, 0); // -> CTray
    uintptr_t TrayUIInstance = *((INT64 *)CTrayInstance + TRAYUI_OFFSET_IN_CTRAY) + 8;

    /*
     * NOTE:
     * This section reeks of having been decompiled by IDA or Ghidra. If that was
     * the case, using it in a product licensed under the GPL is certainly illegal.
     * It's also broken.
     */
#if 0
    if (TrayUIInstance) {
        UINT_PTR const offset = IsWindows11Version22H2OrHigher() ? 640 : 656;
        if ((*(unsigned __int8(__fastcall **)(INT64))(**(INT64 **)(TrayUIInstance + offset) + 104i64))(*(INT64 *)(TrayUIInstance + offset)))
        {
            DeleteMenu(hSubMenu, 0x1A0u, 0);
        }
        else {
            WCHAR Buffer[MAX_PATH];
            WCHAR v40[MAX_PATH];
            WCHAR NewItem[MAX_PATH];
            LoadStringW(GetModuleHandleW(NULL), 0x216u, Buffer, 64);
            UINT v22 = (*(__int64(__fastcall **)(INT64))(**(INT64 **)(TrayUIInstance + offset) + 112i64))(*(INT64 *)(TrayUIInstance + offset));
            LoadStringW(GetModuleHandleW(NULL), v22, v40, 96);
            swprintf_s(NewItem, 0xA0ui64, Buffer, v40);
            ModifyMenuW(hSubMenu, 0x1A0u, 0, 0x1A0ui64, NewItem);
        }
    } else {
        DeleteMenu(hSubMenu, 416, MF_BYCOMMAND); // Undo
    }
#else
    DeleteMenu(hSubMenu, 416, MF_BYCOMMAND); // Undo
#endif

    DeleteMenu(hSubMenu, 437, MF_BYCOMMAND);  // Show Pen button
    DeleteMenu(hSubMenu, 438, MF_BYCOMMAND);  // Show touchpad button
    DeleteMenu(hSubMenu, 435, MF_BYCOMMAND);  // Show People on the taskbar
    DeleteMenu(hSubMenu, 430, MF_BYCOMMAND);  // Show Task View button
    DeleteMenu(hSubMenu, 449, MF_BYCOMMAND);  // Show Cortana button
    DeleteMenu(hSubMenu, 621, MF_BYCOMMAND);  // News and interests
    DeleteMenu(hSubMenu, 445, MF_BYCOMMAND);  // Cortana
    DeleteMenu(hSubMenu, 431, MF_BYCOMMAND);  // Search
    DeleteMenu(hSubMenu, 421, MF_BYCOMMAND);  // Customize notification icons
    DeleteMenu(hSubMenu, 408, MF_BYCOMMAND);  // Adjust date/time
    DeleteMenu(hSubMenu, 436, MF_BYCOMMAND);  // Show touch keyboard button
    DeleteMenu(hSubMenu,   0, MF_BYPOSITION); // Separator
    DeleteMenu(hSubMenu,   0, MF_BYPOSITION); // Separator

    WCHAR buffer[260];
    LoadStringW(GetModuleHandleW(L"ExplorerFrame.dll"), 50222, buffer + (bNoMenuAccelerator ? 0 : 1), 260);
    if (!bNoMenuAccelerator)
        buffer[0] = L'&';
    wchar_t *p = wcschr(buffer, L'(');
    if (p) {
        if (*--p == L' ')
            *p = 0;
        else
            *++p = 0;
    }

    MENUITEMINFOW menuInfo = {
        .cbSize     = sizeof menuInfo,
        .fMask      = MIIM_ID | MIIM_STRING | MIIM_DATA | MIIM_STATE,
        .wID        = 3999,
        .dwItemData = 0,
        .fType      = MFT_STRING,
        .dwTypeData = buffer,
        .cch        = wcslen(buffer),
    };
    if (!bNoPropertiesInContextMenu)
        InsertMenuItemW(hSubMenu, GetMenuItemCount(hSubMenu) - 1, TRUE, &menuInfo);

    POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

    INT64 *unknown_array = NULL;
    if (bSkinMenus) {
        unknown_array = calloc(4, sizeof(INT64));
        if (ImmersiveContextMenuHelper_ApplyOwnerDrawToMenuFunc)
            ImmersiveContextMenuHelper_ApplyOwnerDrawToMenuFunc(hSubMenu, hWnd, &pt, 0xC, unknown_array);
    }

    BOOL res = TrackPopupMenu(hSubMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, 0);
    if (res == 3999)
        LaunchPropertiesGUI(hModule);
    else
        PostMessageW(hWnd, WM_COMMAND, res, 0);

    if (bSkinMenus) {
        if (ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc)
            ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc(hSubMenu, hWnd, &pt);
        free(unknown_array);
    }

    DestroyMenu(hSubMenu);
    DestroyMenu(hMenu);
}

static INT64 Shell_TrayWndSubclassProc(
    _In_ HWND   hWnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    UINT_PTR    uIdSubclass,
    DWORD_PTR   bIsPrimaryTaskbar
)
{
    if (uMsg == WM_NCDESTROY)
    {
        if (bIsPrimaryTaskbar)
            UnhookWindowsHookEx(hShell_TrayWndMouseHook);
        RemoveWindowSubclass(hWnd, Shell_TrayWndSubclassProc, Shell_TrayWndSubclassProc);
    }
    else if ((uMsg == WM_NCLBUTTONDOWN || uMsg == WM_NCRBUTTONUP) &&
             bOldTaskbar && !bIsPrimaryTaskbar &&
             TaskbarCenter_ShouldCenter(dwMMOldTaskbarAl) &&
             TaskbarCenter_ShouldStartBeCentered(dwMMOldTaskbarAl) &&
             HandleTaskbarCornerInteraction(hWnd, uMsg, wParam, lParam))
    {
        return 0;
    }
    else if (uMsg == WM_CONTEXTMENU && !bIsPrimaryTaskbar)
    {
        // Received some times when right clicking a secondary taskbar button, and it would
        // show the classic taskbar context menu but containing only "Show desktop" instead
        // of ours or a button's jump list, so we cancel it and that seems to properly invoke
        // the right menu
        return 0;
    }
    else if (uMsg == WM_SETCURSOR && !bOldTaskbar && !bIsPrimaryTaskbar)
    {
        // Received when mouse is over taskbar edge and autohide is on
        PostMessageW(hWnd, WM_ACTIVATE, WA_ACTIVE, NULL);
    }
    else if (uMsg == WM_LBUTTONDBLCLK && bOldTaskbar && bTaskbarAutohideOnDoubleClick)
    {
        ToggleTaskbarAutohide();
        return 0;
    }
    else if (uMsg == WM_HOTKEY && wParam == 500 &&
             lParam == MAKELPARAM(MOD_WIN, 0x41) && IsWindows11())
    {
        InvokeActionCenter();
        return 0;
# if 0
        if (lpShouldDisplayCCButton)
        {
            *lpShouldDisplayCCButton = 1;
        }
        LRESULT lRes = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        if (lpShouldDisplayCCButton)
        {
            *lpShouldDisplayCCButton = bHideControlCenterButton;
        }
        return lRes;
# endif
    }
    else if (uMsg == WM_DISPLAYCHANGE && bIsPrimaryTaskbar)
    {
        UpdateStartMenuPositioning(MAKELPARAM(TRUE, FALSE));
    }
# if 0
    else if (uMsg == WM_PARENTNOTIFY && wParam == WM_RBUTTONDOWN && !bOldTaskbar && !Shell_TrayWndMouseHook) // && !IsUndockingDisabled
    {
        DWORD dwThreadId = GetCurrentThreadId();
        Shell_TrayWndMouseHook = SetWindowsHookExW(WH_MOUSE, Shell_TrayWndMouseProc, NULL, dwThreadId);
    }
# endif
    else if (uMsg == RegisterWindowMessageW(L"Windows11ContextMenu_" EP_CLSID))
    {
        Shell_handleWin11Menu(hWnd, lParam);
    }
    else if (uMsg == 1368)
    {
        g_bIsDesktopRaised = (lParam & 1) == 0;
    }
    else if (uMsg == WM_SETTINGCHANGE &&
             *((WORD *)&lParam + 1) != NULL &&
             WStrEq((LPCWSTR)lParam, L"EnsureXAML") &&
             IsWindows11Version22H2OrHigher())
    {
        //EnsureXAML();
        ExplorerPatcher_EnsureXAML();
        return 0;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

#endif
#pragma endregion


#pragma region "Allow legacy volume applet"
#ifdef _WIN64
static LSTATUS sndvolsso_RegGetValueW(
    HKEY    hkey,
    LPCWSTR lpSubKey,
    LPCWSTR lpValue,
    DWORD   dwFlags,
    LPDWORD pdwType,
    PVOID   pvData,
    LPDWORD pcbData
)
{
    if (SHRegGetValueFromHKCUHKLMFunc && hkey == HKEY_LOCAL_MACHINE &&
        WStrIEq(lpSubKey, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\MTCUVC") &&
        WStrIEq(lpValue, L"EnableMTCUVC"))
    {
        return SHRegGetValueFromHKCUHKLMFunc(lpSubKey, lpValue, SRRF_RT_REG_DWORD, pdwType, pvData, pcbData);
    }
    return RegGetValueW(hkey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);
}
#endif
#pragma endregion


#pragma region "Allow legacy date and time"
#ifdef _WIN64

DEFINE_GUID(GUID_Win32Clock,
            0x0A323554A, 0x0FE1, 0x4E49, 0xAE, 0xE1, 0x67, 0x22, 0x46, 0x5D, 0x79, 0x9F);
DEFINE_GUID(IID_Win32Clock,
            0x7A5FCA8A, 0x76B1, 0x44C8, 0xA9, 0x7C, 0xE7, 0x17, 0x3C, 0xCA, 0x5F, 0x4F);

typedef interface Win32Clock Win32Clock;
typedef struct Win32ClockVtbl {
    BEGIN_INTERFACE

    HRESULT(STDMETHODCALLTYPE *QueryInterface)
    (Win32Clock       *This,
     /* [in] */ REFIID riid,
     /* [annotation][iid_is][out] */
     _COM_Outptr_ void **ppvObject);

    ULONG(STDMETHODCALLTYPE *AddRef)(Win32Clock *This);

    ULONG(STDMETHODCALLTYPE *Release)(Win32Clock *This);

    HRESULT(STDMETHODCALLTYPE *ShowWin32Clock)
    (Win32Clock       *This,
     /* [in] */ HWND   hWnd,
     /* [in] */ LPRECT lpRect);

    END_INTERFACE
} Win32ClockVtbl;

interface Win32Clock {
    CONST_VTBL struct Win32ClockVtbl *lpVtbl;
};

static DWORD ShouldShowLegacyClockExperience(void)
{
    DWORD dwVal = 0, dwSize = sizeof(DWORD);
    if (SHRegGetValueFromHKCUHKLMFunc &&
        SHRegGetValueFromHKCUHKLMFunc(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell", L"UseWin32TrayClockExperience",
                                      SRRF_RT_REG_DWORD, NULL, &dwVal, (LPDWORD)(&dwSize)) == ERROR_SUCCESS)
        return dwVal;
    return 0;
}

static BOOL ShowLegacyClockExperience(HWND hWnd)
{
    if (!hWnd)
        return FALSE;
    Win32Clock *pWin32Clock = NULL;
    HRESULT     hr = CoCreateInstance(&GUID_Win32Clock, NULL,
                                      CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                                      &IID_Win32Clock, &pWin32Clock);
    if (SUCCEEDED(hr)) {
        RECT rc;
        GetWindowRect(hWnd, &rc);
        pWin32Clock->lpVtbl->ShowWin32Clock(pWin32Clock, hWnd, &rc);
        pWin32Clock->lpVtbl->Release(pWin32Clock);
    }
    return TRUE;
}

static INT64 ClockButtonSubclassProc(
    _In_ HWND   hWnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    UINT_PTR    uIdSubclass,
    DWORD_PTR   dwRefData
)
{
    if (uMsg == WM_NCDESTROY) {
        RemoveWindowSubclass(hWnd, ClockButtonSubclassProc, ClockButtonSubclassProc);
    } else if (uMsg == WM_LBUTTONDOWN || (uMsg == WM_KEYDOWN && wParam == VK_RETURN)) {
        if (ShouldShowLegacyClockExperience() == 1) {
            if (!FindWindowW(L"ClockFlyoutWindow", NULL))
                return ShowLegacyClockExperience(hWnd);
            else
                return 1;
        } else if (ShouldShowLegacyClockExperience() == 2) {
            if (FindWindowW(L"Windows.UI.Core.CoreWindow", NULL)) {
                if (IsWindows11())
                    ToggleNotificationsFlyout();
                else
                    ToggleActionCenter();
            }
            return 1;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

#endif
#pragma endregion


#pragma region "Popup menu hooks"
static BOOL CheckIfImmersiveContextMenu(HWND unnamedParam1, LPCSTR unnamedParam2, HANDLE unnamedParam3)
{
    if ((*((WORD *)&(unnamedParam2) + 1))) {
        if (!strncmp(unnamedParam2, "ImmersiveContextMenuArray", 25)) {
            bIsImmersiveMenu = TRUE;
            return FALSE;
        }
    }
    return TRUE;
}

static void RemoveOwnerDrawFromMenu(int level, HMENU hMenu)
{
    if (hMenu) {
        int k = GetMenuItemCount(hMenu);
        for (int i = 0; i < k; ++i) {
            MENUITEMINFOW mii = {
                .cbSize = sizeof mii,
                .fMask  = MIIM_FTYPE | MIIM_SUBMENU,
            };
            if (GetMenuItemInfoW(hMenu, i, TRUE, &mii) && (mii.fType & MFT_OWNERDRAW)) {
                mii.fType &= ~MFT_OWNERDRAW;
                printf("[ROD]: Level %d Position %d/%d Status %d\n",
                       level, i, k, SetMenuItemInfoW(hMenu, i, TRUE, &mii));
                RemoveOwnerDrawFromMenu(level + 1, mii.hSubMenu);
            }
        }
    }
}

static BOOL CheckIfMenuContainsOwnPropertiesItem(HMENU hMenu)
{
#ifdef _WIN64
    if (hMenu) {
        int k = GetMenuItemCount(hMenu);
        for (int i = k - 1; i >= 0; i--) {
            MENUITEMINFOW mii = {
                .cbSize = sizeof mii,
                .fMask  = MIIM_DATA | MIIM_ID,
            };
            BOOL b = GetMenuItemInfoW(hMenu, i, TRUE, &mii);
            if (b && (mii.wID >= 12000 && mii.wID <= 12200) && mii.dwItemData == CheckForUpdatesThread)
                return TRUE;
        }
    }
#endif
    return FALSE;
}

static BOOL TrackPopupMenuHookEx(HMENU hMenu, UINT uFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm)
{
    bIsImmersiveMenu = FALSE;

    wchar_t wszClassName[200];
    GetClassNameW(hWnd, wszClassName, 200);

    BOOL bIsTaskbar = (WStrEq(wszClassName, L"Shell_TrayWnd") || WStrEq(wszClassName, L"Shell_SecondaryTrayWnd"))
                          ? !bSkinMenus
                          : bDisableImmersiveContextMenu;
    // wprintf(L">> %s %d %d\n", wszClassName, bIsTaskbar, bIsExplorerProcess);

    BOOL bContainsOwn = FALSE;
    if (bIsExplorerProcess && (WStrEq(wszClassName, L"Shell_TrayWnd") ||
                               WStrEq(wszClassName, L"Shell_SecondaryTrayWnd")))
    {
        bContainsOwn = CheckIfMenuContainsOwnPropertiesItem(hMenu);
    }

    bool isExplorerProcess = bIsExplorerProcess
                             ? 1
                             : (WStrEq(wszClassName, L"SHELLDLL_DefView") ||
                                WStrEq(wszClassName, L"SysTreeView32"));

    if (bIsTaskbar && isExplorerProcess) {
        EnumPropsA(hWnd, CheckIfImmersiveContextMenu);
        if (bIsImmersiveMenu) {
            bIsImmersiveMenu = FALSE;
#ifndef _WIN64
            if (bIsExplorerProcess) {
#else
            if (bIsExplorerProcess && ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc) {
                POINT pt = {x, y};
                ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc(hMenu, hWnd, &(pt));
#endif
            } else {
                RemoveOwnerDrawFromMenu(0, hMenu);
            }

            BOOL bRet = TrackPopupMenuEx(hMenu, uFlags, x, y, hWnd, lptpm);
#ifdef _WIN64
            if (bContainsOwn && (bRet >= 12000 && bRet <= 12200)) {
                LaunchPropertiesGUI(hModule);
                return FALSE;
            }
#endif
            return bRet;
        }
        bIsImmersiveMenu = FALSE;
    }
    BOOL b = TrackPopupMenuEx(hMenu, uFlags, x, y, hWnd, lptpm);
#ifdef _WIN64
    if (bContainsOwn && (b >= 12000 && b <= 12200)) {
        LaunchPropertiesGUI(hModule);
        return FALSE;
    }
#endif
    return b;
}

static BOOL TrackPopupMenuHook(
    HMENU       hMenu,
    UINT        uFlags,
    int         x,
    int         y,
    int         nReserved,
    HWND        hWnd,
    const RECT *prcRect
)
{
    bIsImmersiveMenu = FALSE;

    wchar_t wszClassName[200];
    ZeroMemory(wszClassName, 200);
    GetClassNameW(hWnd, wszClassName, 200);

    BOOL bIsTaskbar = (WStrEq(wszClassName, L"Shell_TrayWnd") || WStrEq(wszClassName, L"Shell_SecondaryTrayWnd"))
                          ? !bSkinMenus
                          : bDisableImmersiveContextMenu;
    // wprintf(L">> %s %d %d\n", wszClassName, bIsTaskbar, bIsExplorerProcess);

    BOOL bContainsOwn = FALSE;
    if (bIsExplorerProcess &&
        (WStrEq(wszClassName, L"Shell_TrayWnd") ||
         WStrEq(wszClassName, L"Shell_SecondaryTrayWnd")))
    {
        bContainsOwn = CheckIfMenuContainsOwnPropertiesItem(hMenu);
    }

    if (bIsTaskbar &&
        (bIsExplorerProcess ||
         WStrEq(wszClassName, L"SHELLDLL_DefView") ||
         WStrEq(wszClassName, L"SysTreeView32")))
    {
        EnumPropsA(hWnd, CheckIfImmersiveContextMenu);
        if (bIsImmersiveMenu) {
            bIsImmersiveMenu = FALSE;

#ifndef _WIN64
            if (bIsExplorerProcess) {
#else
            if (bIsExplorerProcess && ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc) {
                POINT pt;
                pt.x = x;
                pt.y = y;
                ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc(hMenu, hWnd, &(pt));
#endif
            } else {
                RemoveOwnerDrawFromMenu(0, hMenu);
            }

            BOOL bRet = TrackPopupMenu(hMenu, uFlags, x, y, 0, hWnd, prcRect);
#ifdef _WIN64
            if (bContainsOwn && (bRet >= 12000 && bRet <= 12200)) {
                LaunchPropertiesGUI(hModule);
                return FALSE;
            }
#endif
            return bRet;
        }
        bIsImmersiveMenu = FALSE;
    }
    BOOL b = TrackPopupMenu(hMenu, uFlags, x, y, 0, hWnd, prcRect);
#ifdef _WIN64
    if (bContainsOwn && (b >= 12000 && b <= 12200)) {
        LaunchPropertiesGUI(hModule);
        return FALSE;
    }
#endif
    return b;
}

#ifdef _WIN64
# define TB_POS_NOWHERE 0
# define TB_POS_BOTTOM  1
# define TB_POS_TOP     2
# define TB_POS_LEFT    3
# define TB_POS_RIGHT   4

static UINT GetTaskbarLocationAndSize(POINT ptCursor, RECT *rc);

static void PopupMenuAdjustCoordinatesAndFlags(int *x, int *y, UINT *uFlags)
{
    POINT pt;
    GetCursorPos(&pt);
    RECT rc;
    UINT tbPos = GetTaskbarLocationAndSize(pt, &rc);
    if (tbPos == TB_POS_BOTTOM) {
        *y = MIN(*y, rc.top);
        *uFlags |= TPM_CENTERALIGN | TPM_BOTTOMALIGN;
    } else if (tbPos == TB_POS_TOP) {
        *y = MAX(*y, rc.bottom);
        *uFlags |= TPM_CENTERALIGN | TPM_TOPALIGN;
    } else if (tbPos == TB_POS_LEFT) {
        *x = MAX(*x, rc.right);
        *uFlags |= TPM_VCENTERALIGN | TPM_LEFTALIGN;
    }
    if (tbPos == TB_POS_RIGHT) {
        *x = MIN(*x, rc.left);
        *uFlags |= TPM_VCENTERALIGN | TPM_RIGHTALIGN;
    }
}

static UINT GetTaskbarLocationAndSize(POINT ptCursor, RECT *rc)
{
    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    HWND hWnd = GetMonitorInfoFromPointForTaskbarFlyoutActivation(ptCursor, MONITOR_DEFAULTTOPRIMARY, &mi);
    if (hWnd) {
        GetWindowRect(hWnd, rc);
        RECT rcC = *rc;
        rcC.left -= mi.rcMonitor.left;
        rcC.right -= mi.rcMonitor.left;
        rcC.top -= mi.rcMonitor.top;
        rcC.bottom -= mi.rcMonitor.top;

        if (rcC.left < 5 && rcC.top > 5)
            return TB_POS_BOTTOM;
        else if (rcC.left < 5 && rcC.top < 5 && rcC.right > rcC.bottom)
            return TB_POS_TOP;
        else if (rcC.left < 5 && rcC.top < 5 && rcC.right < rcC.bottom)
            return TB_POS_LEFT;
        else if (rcC.left > 5 && rcC.top < 5)
            return TB_POS_RIGHT;
    }
    return TB_POS_NOWHERE;
}

static INT64 OwnerDrawSubclassProc(
    _In_ HWND   hWnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    UINT_PTR    uIdSubclass,
    DWORD_PTR   dwRefData
)
{
    BOOL v12 = FALSE;
    if ((uMsg == WM_DRAWITEM || uMsg == WM_MEASUREITEM) &&
        CImmersiveContextMenuOwnerDrawHelper_s_ContextMenuWndProcFunc &&
        CImmersiveContextMenuOwnerDrawHelper_s_ContextMenuWndProcFunc(hWnd, uMsg, wParam, lParam, &v12))
    {
        return 0;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static BOOL explorer_TrackPopupMenuExHook(
    HMENU       hMenu,
    UINT        uFlags,
    int         x,
    int         y,
    HWND        hWnd,
    LPTPMPARAMS lptpm
)
{
    static ULONGLONG explorer_TrackPopupMenuExElapsed = 0;

    ULONGLONG elapsed = milliseconds_now() - explorer_TrackPopupMenuExElapsed;
    BOOL b = FALSE;

    wchar_t wszClassName[200];
    ZeroMemory(wszClassName, 200);
    GetClassNameW(hWnd, wszClassName, 200);
    BOOL bContainsOwn = FALSE;
    if (bIsExplorerProcess &&
        (WStrEq(wszClassName, L"Shell_TrayWnd") || WStrEq(wszClassName, L"Shell_SecondaryTrayWnd"))) {
        bContainsOwn = CheckIfMenuContainsOwnPropertiesItem(hMenu);
    }

    wchar_t wszClassNameOfWindowUnderCursor[200];
    ZeroMemory(wszClassNameOfWindowUnderCursor, 200);
    POINT p;
    p.x = x;
    p.y = y;
    GetClassNameW(WindowFromPoint(p), wszClassNameOfWindowUnderCursor, 200);
    BOOL bIsSecondaryTaskbar =
        (WStrEq(wszClassName, L"Shell_SecondaryTrayWnd") &&
         WStrEq(wszClassNameOfWindowUnderCursor, L"Shell_SecondaryTrayWnd"));

    if (elapsed > POPUPMENU_EX_ELAPSED || !bFlyoutMenus || bIsSecondaryTaskbar) {
        if (bCenterMenus && !bIsSecondaryTaskbar)
            PopupMenuAdjustCoordinatesAndFlags(&x, &y, &uFlags);
        bIsImmersiveMenu = FALSE;
        if (!bSkinMenus) {
            EnumPropsA(hWnd, CheckIfImmersiveContextMenu);
            if (bIsImmersiveMenu) {
                if (ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc) {
                    POINT pt = {x, y};
                    ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc(hMenu, hWnd, &(pt));
                } else {
                    RemoveOwnerDrawFromMenu(0, hMenu);
                }
            }
            bIsImmersiveMenu = FALSE;
        }
        b = TrackPopupMenuEx(hMenu, uFlags, x, y, hWnd, lptpm);
        if (bContainsOwn && (b >= 12000 && b <= 12200)) {
            LaunchPropertiesGUI(hModule);
            return FALSE;
        }
        if (!bIsSecondaryTaskbar)
            explorer_TrackPopupMenuExElapsed = milliseconds_now();
    }
    return b;
}

static BOOL pnidui_TrackPopupMenuHook(
    HMENU       hMenu,
    UINT        uFlags,
    int         x,
    int         y,
    int         nReserved,
    HWND        hWnd,
    const RECT *prcRect
)
{
    static ULONGLONG pnidui_TrackPopupMenuElapsed = 0;

    ULONGLONG elapsed = milliseconds_now() - pnidui_TrackPopupMenuElapsed;
    BOOL b = FALSE;

    if (elapsed > POPUPMENU_PNIDUI_TIMEOUT || !bFlyoutMenus) {
        if (bCenterMenus)
            PopupMenuAdjustCoordinatesAndFlags(&x, &y, &uFlags);
        bIsImmersiveMenu = FALSE;
        if (!bSkinMenus) {
            EnumPropsA(hWnd, CheckIfImmersiveContextMenu);
            if (bIsImmersiveMenu) {
                if (ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc) {
                    POINT pt = {x, y};
                    ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc(hMenu, hWnd, &(pt));
                } else {
                    RemoveOwnerDrawFromMenu(0, hMenu);
                }
            }
            bIsImmersiveMenu = FALSE;
        }
        b = TrackPopupMenu(hMenu, uFlags | TPM_RIGHTBUTTON, x, y, 0, hWnd, prcRect);
        if (bReplaceNetwork && b == 3109) {
            LaunchNetworkTargets(bReplaceNetwork + 2);
            b = 0;
        }
        pnidui_TrackPopupMenuElapsed = milliseconds_now();
    }
    return b;
}

static BOOL sndvolsso_TrackPopupMenuExHook(
    HMENU       hMenu,
    UINT        uFlags,
    int         x,
    int         y,
    HWND        hWnd,
    LPTPMPARAMS lptpm
)
{
    static ULONGLONG sndvolsso_TrackPopupMenuExElapsed = 0;

    ULONGLONG elapsed = milliseconds_now() - sndvolsso_TrackPopupMenuExElapsed;
    BOOL b = FALSE;

    if (elapsed > POPUPMENU_SNDVOLSSO_TIMEOUT || !bFlyoutMenus) {
        if (bCenterMenus)
            PopupMenuAdjustCoordinatesAndFlags(&x, &y, &uFlags);
        bIsImmersiveMenu = FALSE;
        if (!bSkinMenus) {
            EnumPropsA(hWnd, CheckIfImmersiveContextMenu);
            if (bIsImmersiveMenu) {
                if (ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc) {
                    POINT pt;
                    pt.x = x;
                    pt.y = y;
                    ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc(hMenu, hWnd, &(pt));
                } else {
                    RemoveOwnerDrawFromMenu(0, hMenu);
                }
            }
            bIsImmersiveMenu = FALSE;
        }

#if 0
        MENUITEMINFOW menuInfo;
        ZeroMemory(&menuInfo, sizeof(MENUITEMINFOW));
        menuInfo.cbSize = sizeof(MENUITEMINFOW);
        menuInfo.fMask = MIIM_ID | MIIM_STRING;
        printf("GetMenuItemInfoW %d\n", GetMenuItemInfoW(hMenu, GetMenuItemCount(hMenu) - 1, TRUE, &menuInfo));
        menuInfo.dwTypeData = malloc(menuInfo.cch + sizeof(wchar_t));
        menuInfo.cch++;
        printf("GetMenuItemInfoW %d\n", GetMenuItemInfoW(hMenu, GetMenuItemCount(hMenu) - 1, TRUE, &menuInfo));
        wcscpy_s(menuInfo.dwTypeData, menuInfo.cch, L"test");
        menuInfo.fMask = MIIM_STRING;
        wprintf(L"SetMenuItemInfoW %s %d\n", menuInfo.dwTypeData, SetMenuItemInfoW(hMenu, GetMenuItemCount(hMenu) - 1, TRUE, &menuInfo));
        wcscpy_s(menuInfo.dwTypeData, menuInfo.cch, L"");
        printf("GetMenuItemInfoW %d\n", GetMenuItemInfoW(hMenu, GetMenuItemCount(hMenu) - 1, TRUE, &menuInfo));
        wprintf(L"%s\n", menuInfo.dwTypeData);
        free(menuInfo.dwTypeData);
#endif

        b = TrackPopupMenuEx(hMenu, uFlags | TPM_RIGHTBUTTON, x, y, hWnd, lptpm);
        sndvolsso_TrackPopupMenuExElapsed = milliseconds_now();
    }

    return b;
}

static void PatchSndvolsso(void)
{
    HANDLE hSndvolsso = LoadLibraryW(L"sndvolsso.dll");
    VnPatchIAT(hSndvolsso, "user32.dll", "TrackPopupMenuEx", sndvolsso_TrackPopupMenuExHook);
    VnPatchIAT(hSndvolsso, "api-ms-win-core-registry-l1-1-0.dll", "RegGetValueW", sndvolsso_RegGetValueW);
# ifdef USE_PRIVATE_INTERFACES
    if (bSkinIcons)
        VnPatchIAT(hSndvolsso, "user32.dll", "LoadImageW", SystemTray_LoadImageWHook);
# endif
    printf("Setup sndvolsso functions done\n");
}

static BOOL stobject_TrackPopupMenuExHook(
    HMENU       hMenu,
    UINT        uFlags,
    int         x,
    int         y,
    HWND        hWnd,
    LPTPMPARAMS lptpm
)
{
    static ULONGLONG stobject_TrackPopupMenuExElapsed = 0;

    ULONGLONG elapsed = milliseconds_now() - stobject_TrackPopupMenuExElapsed;
    BOOL b = FALSE;

    if (elapsed > POPUPMENU_SAFETOREMOVE_TIMEOUT || !bFlyoutMenus) {
        if (bCenterMenus)
            PopupMenuAdjustCoordinatesAndFlags(&x, &y, &uFlags);
        INT64 *unknown_array = NULL;
        POINT  pt = {x, y};
        if (bSkinMenus) {
            unknown_array = calloc(4, sizeof(INT64));
            if (ImmersiveContextMenuHelper_ApplyOwnerDrawToMenuFunc)
                ImmersiveContextMenuHelper_ApplyOwnerDrawToMenuFunc(hMenu, hWnd, &(pt), 0xC, unknown_array);
            SetWindowSubclass(hWnd, OwnerDrawSubclassProc, OwnerDrawSubclassProc, 0);
        }
        b = TrackPopupMenuEx(hMenu, uFlags | TPM_RIGHTBUTTON, x, y, hWnd, lptpm);
        stobject_TrackPopupMenuExElapsed = milliseconds_now();
        if (bSkinMenus) {
            RemoveWindowSubclass(hWnd, OwnerDrawSubclassProc, OwnerDrawSubclassProc);
            if (ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc)
                ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc(hMenu, hWnd, &(pt));
            free(unknown_array);
        }
    }
    return b;
}

static BOOL stobject_TrackPopupMenuHook(
    HMENU       hMenu,
    UINT        uFlags,
    int         x,
    int         y,
    int         nReserved,
    HWND        hWnd,
    const RECT *prcRect
)
{
    static ULONGLONG stobject_TrackPopupMenuElapsed = 0;

    ULONGLONG elapsed = milliseconds_now() - stobject_TrackPopupMenuElapsed;
    BOOL b = FALSE;

    if (elapsed > POPUPMENU_SAFETOREMOVE_TIMEOUT || !bFlyoutMenus) {
        if (bCenterMenus)
            PopupMenuAdjustCoordinatesAndFlags(&x, &y, &uFlags);
        INT64 *unknown_array = NULL;
        POINT  pt = {x, y};
        if (bSkinMenus) {
            unknown_array = calloc(4, sizeof(INT64));
            if (ImmersiveContextMenuHelper_ApplyOwnerDrawToMenuFunc)
                ImmersiveContextMenuHelper_ApplyOwnerDrawToMenuFunc(hMenu, hWnd, &(pt), 0xC, unknown_array);
            SetWindowSubclass(hWnd, OwnerDrawSubclassProc, OwnerDrawSubclassProc, 0);
        }
        b = TrackPopupMenu(hMenu, uFlags | TPM_RIGHTBUTTON, x, y, 0, hWnd, prcRect);
        stobject_TrackPopupMenuElapsed = milliseconds_now();
        if (bSkinMenus) {
            RemoveWindowSubclass(hWnd, OwnerDrawSubclassProc, OwnerDrawSubclassProc);
            if (ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc)
                ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc(hMenu, hWnd, &(pt));
            free(unknown_array);
        }
    }
    return b;
}

static BOOL bthprops_TrackPopupMenuExHook(
    HMENU       hMenu,
    UINT        uFlags,
    int         x,
    int         y,
    HWND        hWnd,
    LPTPMPARAMS lptpm
)
{
    static ULONGLONG bthprops_TrackPopupMenuExElapsed = 0;

    ULONGLONG elapsed = milliseconds_now() - bthprops_TrackPopupMenuExElapsed;
    BOOL b = FALSE;

    if (elapsed > POPUPMENU_BLUETOOTH_TIMEOUT || !bFlyoutMenus) {
        if (bCenterMenus)
            PopupMenuAdjustCoordinatesAndFlags(&x, &y, &uFlags);
        INT64 *unknown_array = NULL;
        POINT  pt;
        if (bSkinMenus) {
            unknown_array = calloc(4, sizeof(INT64));
            pt.x = x;
            pt.y = y;
            if (ImmersiveContextMenuHelper_ApplyOwnerDrawToMenuFunc)
                ImmersiveContextMenuHelper_ApplyOwnerDrawToMenuFunc(hMenu, hWnd, &(pt), 0xC, unknown_array);
            SetWindowSubclass(hWnd, OwnerDrawSubclassProc, OwnerDrawSubclassProc, 0);
        }
        b = TrackPopupMenuEx(hMenu, uFlags | TPM_RIGHTBUTTON, x, y, hWnd, lptpm);
        bthprops_TrackPopupMenuExElapsed = milliseconds_now();
        if (bSkinMenus) {
            RemoveWindowSubclass(hWnd, OwnerDrawSubclassProc, OwnerDrawSubclassProc);
            if (ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc)
                ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc(hMenu, hWnd, &(pt));
            free(unknown_array);
        }
    }
    return b;
}

static BOOL inputswitch_TrackPopupMenuExHook(
    HMENU       hMenu,
    UINT        uFlags,
    int         x,
    int         y,
    HWND        hWnd,
    LPTPMPARAMS lptpm
)
{
    static ULONGLONG inputswitch_TrackPopupMenuExElapsed = 0;

    ULONGLONG elapsed = milliseconds_now() - inputswitch_TrackPopupMenuExElapsed;
    BOOL b = FALSE;

    if (elapsed > POPUPMENU_INPUTSWITCH_TIMEOUT || !bFlyoutMenus) {
        if (bCenterMenus)
            PopupMenuAdjustCoordinatesAndFlags(&x, &y, &uFlags);
        bIsImmersiveMenu = FALSE;
        if (!bSkinMenus) {
            EnumPropsA(hWnd, CheckIfImmersiveContextMenu);
            if (bIsImmersiveMenu) {
                if (ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc) {
                    POINT pt = {x, y};
                    ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc(hMenu, hWnd, &(pt));
                } else {
                    RemoveOwnerDrawFromMenu(0, hMenu);
                }
            }
            bIsImmersiveMenu = FALSE;
        }
        b = TrackPopupMenuEx(hMenu, uFlags | TPM_RIGHTBUTTON, x, y, hWnd, lptpm);
        inputswitch_TrackPopupMenuExElapsed = milliseconds_now();
    }
    return b;
}

static BOOL twinui_TrackPopupMenuHook(
    HMENU       hMenu,
    UINT        uFlags,
    int         x,
    int         y,
    int         nReserved,
    HWND        hWnd,
    const RECT *prcRect
)
{
    static ULONGLONG twinui_TrackPopupMenuElapsed = 0;

    // ULONGLONG elapsed = milliseconds_now() - twinui_TrackPopupMenuElapsed;
    BOOL b = FALSE;

    if (1 /*elapsed > POPUPMENU_WINX_TIMEOUT || !bFlyoutMenus*/) {
        if (bCenterMenus) {
            // PopupMenuAdjustCoordinatesAndFlags(&x, &y, &uFlags);
        }
        bIsImmersiveMenu = FALSE;
        if (!bSkinMenus) {
            EnumPropsA(hWnd, CheckIfImmersiveContextMenu);
            if (bIsImmersiveMenu) {
                if (ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc) {
                    POINT pt;
                    pt.x = x;
                    pt.y = y;
                    ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc(hMenu, hWnd, &(pt));
                } else {
                    RemoveOwnerDrawFromMenu(0, hMenu);
                }
            }
            bIsImmersiveMenu = FALSE;
        }
        b = TrackPopupMenu(hMenu, uFlags | TPM_RIGHTBUTTON, x, y, 0, hWnd, prcRect);
        // twinui_TrackPopupMenuElapsed = milliseconds_now();
    }
    return b;
}
#endif
#pragma endregion


#pragma region "Disable immersive menus"
static BOOL WINAPI DisableImmersiveMenus_SystemParametersInfoW(
    UINT  uiAction,
    UINT  uiParam,
    PVOID pvParam,
    UINT  fWinIni
)
{
    if (bDisableImmersiveContextMenu && uiAction == SPI_GETSCREENREADER) {
        printf("SystemParametersInfoW\n");
        *(BOOL *)pvParam = TRUE;
        return TRUE;
    }
    return SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);
}
#pragma endregion


#pragma region "Explorer: Hide search bar, hide icon and/or title, Mica effect, hide navigation bar"
static inline BOOL IsRibbonEnabled(HWND hWnd)
{
    return GetPropW(hWnd, (LPCWSTR)0xA91C);
}

static inline BOOL ShouldApplyMica(HWND hWnd)
{
    if (!IsRibbonEnabled(hWnd))
        return TRUE;
    return FindWindowExW(hWnd, NULL, L"Windows.UI.Composition.DesktopWindowContentBridge", NULL);
}

static HRESULT ApplyMicaToExplorerTitlebar(HWND hWnd, DWORD_PTR bMicaEffectOnTitleBarOrig)
{
    wchar_t wszParentText[128];
    RECT Rect;
    GetWindowRect(hWnd, &Rect);
    HWND hWndRoot = GetAncestor(hWnd, GA_ROOT);
    MapWindowPoints(NULL, hWndRoot, (LPPOINT)&Rect, 2);
    MARGINS pMarInset = {
        .cyTopHeight = Rect.bottom,
    };

    GetWindowTextW(GetParent(hWnd), wszParentText, _countof(wszParentText));
    if (WStrIEq(wszParentText, L"FloatingWindow"))
        pMarInset.cyTopHeight = 0;

    BOOL bShouldApplyMica = (bMicaEffectOnTitleBarOrig == 2)
                            ? FALSE : ShouldApplyMica(GetAncestor(hWnd, GA_ROOT));

    if (bShouldApplyMica) {
        DwmExtendFrameIntoClientArea(hWndRoot, &pMarInset);
        SetPropW(hWndRoot, L"EP_METB", TRUE);
    } else {
        RemovePropW(hWndRoot, L"EP_METB");
    }
    return SetMicaMaterialForThisWindow(hWndRoot, bShouldApplyMica);
}

static LRESULT RebarWindow32MicaTitlebarSubclassproc(
    HWND      hWnd,
    UINT      uMsg,
    WPARAM    wParam,
    LPARAM    lParam,
    UINT_PTR  uIdSubclass,
    DWORD_PTR dwRefData
)
{
    if (uMsg == RB_SETWINDOWTHEME && !wcsncmp(lParam, L"DarkMode", 8) && dwRefData != 2 &&
        ShouldApplyMica(GetAncestor(hWnd, GA_ROOT))) {
        lParam = wcsstr(lParam, L"NavbarComposited");
    } else if (uMsg == WM_DESTROY) {
        RemoveWindowSubclass(hWnd, RebarWindow32MicaTitlebarSubclassproc, RebarWindow32MicaTitlebarSubclassproc);
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static LRESULT ExplorerMicaTitlebarSubclassProc(
    HWND      hWnd,
    UINT      uMsg,
    WPARAM    wParam,
    LPARAM    lParam,
    UINT_PTR  uIdSubclass,
    DWORD_PTR dwRefData
)
{
    if (uMsg == WM_DESTROY)
        RemoveWindowSubclass(hWnd, ExplorerMicaTitlebarSubclassProc,
                             ExplorerMicaTitlebarSubclassProc);
    if (uMsg == WM_ERASEBKGND) {
        wchar_t wszParentText[100];
        GetWindowTextW(GetParent(hWnd), wszParentText, 100);
        if (_wcsicmp(wszParentText, L"FloatingWindow") != 0 &&
            dwRefData != 2 &&
            ShouldApplyMica(GetAncestor(hWnd, GA_ROOT)))
        {
            return TRUE;
        }
    } else if (uMsg == WM_WINDOWPOSCHANGED) {
        WINDOWPOS *lpWp = (WINDOWPOS *)lParam;
        if (lpWp->flags & SWP_NOMOVE)
            ApplyMicaToExplorerTitlebar(hWnd, dwRefData);
        else
            PostMessageW(hWnd, WM_APP, 0, 0);
    } else if (uMsg == WM_APP) {
        ApplyMicaToExplorerTitlebar(hWnd, dwRefData);
    } else if (uMsg == WM_PARENTNOTIFY) {
        if (LOWORD(wParam) == WM_CREATE) {
            ATOM atom = GetClassWord(lParam, GCW_ATOM);
            if (atom == RegisterWindowMessageW(L"ReBarWindow32"))
                SetWindowSubclass(lParam, RebarWindow32MicaTitlebarSubclassproc,
                                  RebarWindow32MicaTitlebarSubclassproc, dwRefData);
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK HideIconAndTitleInExplorerSubClass(
    HWND      hWnd,
    UINT      uMsg,
    WPARAM    wParam,
    LPARAM    lParam,
    UINT_PTR  uIdSubclass,
    DWORD_PTR dwRefData
)
{
    if (uMsg == WM_DESTROY) {
        RemoveWindowSubclass(hWnd, HideIconAndTitleInExplorerSubClass, HideIconAndTitleInExplorerSubClass);
    } else if (uMsg == WM_PARENTNOTIFY) {
        if (LOWORD(wParam) == WM_CREATE) {
            WTA_OPTIONS ops;
            ops.dwFlags = bHideIconAndTitleInExplorer;
            ops.dwMask  = WTNCA_NODRAWCAPTION | WTNCA_NODRAWICON;
            SetWindowThemeAttribute(hWnd, WTA_NONCLIENT, &ops, sizeof(WTA_OPTIONS));
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static HRESULT uxtheme_DwmExtendFrameIntoClientAreaHook(HWND hWnd, MARGINS *m)
{
    if (GetPropW(hWnd, L"EP_METB"))
        return S_OK;
    return DwmExtendFrameIntoClientArea(hWnd, m);
}

static HWND(__stdcall *explorerframe_SHCreateWorkerWindowFunc)(
    WNDPROC  wndProc,
    HWND     hWndParent,
    DWORD    dwExStyle,
    DWORD    dwStyle,
    HMENU    hMenu,
    LONG_PTR wnd_extra
);
static HWND WINAPI explorerframe_SHCreateWorkerWindowHook(
    WNDPROC  wndProc,
    HWND     hWndParent,
    DWORD    dwExStyle,
    DWORD    dwStyle,
    HMENU    hMenu,
    LONG_PTR wnd_extra
)
{
    HWND    result;
    LSTATUS lRes   = ERROR_FILE_NOT_FOUND;
    DWORD   dwSize = 0;

    if (SHRegGetValueFromHKCUHKLMWithOpt(
            L"SOFTWARE\\Classes\\CLSID\\{056440FD-8568-48e7-A632-72157243B55B}\\InProcServer32",
            L"", KEY_READ | KEY_WOW64_64KEY, NULL, (LPDWORD)(&dwSize)
        ) == ERROR_SUCCESS &&
        (dwSize < 4) && dwExStyle == 0x10000 && dwStyle == 1174405120)
    {
        result = NULL;
    } else {
        result = explorerframe_SHCreateWorkerWindowFunc(wndProc, hWndParent, dwExStyle, dwStyle, hMenu, wnd_extra);
    }
    if (dwExStyle == 0x10000 && dwStyle == 0x46000000 && result) {
        if (bHideIconAndTitleInExplorer)
            SetWindowSubclass(hWndParent, HideIconAndTitleInExplorerSubClass, HideIconAndTitleInExplorerSubClass, 0);
        if (bMicaEffectOnTitlebar)
            SetWindowSubclass(result, ExplorerMicaTitlebarSubclassProc, ExplorerMicaTitlebarSubclassProc, bMicaEffectOnTitlebar);
        if (bHideExplorerSearchBar)
            SetWindowSubclass(hWndParent, HideExplorerSearchBarSubClass, HideExplorerSearchBarSubClass, 0);
    }
    return result;
}
#pragma endregion


#pragma region "Fix battery flyout"
#ifdef _WIN64
static LSTATUS stobject_RegGetValueW(
    HKEY    hkey,
    LPCWSTR lpSubKey,
    LPCWSTR lpValue,
    DWORD   dwFlags,
    LPDWORD pdwType,
    PVOID   pvData,
    LPDWORD pcbData
)
{
    if (WStrEq(lpValue, L"UseWin32BatteryFlyout") && SHRegGetValueFromHKCUHKLMFunc)
        return SHRegGetValueFromHKCUHKLMFunc(lpSubKey, lpValue, SRRF_RT_REG_DWORD, pdwType, pvData, pcbData);
    return RegGetValueW(hkey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);
}

static HRESULT stobject_CoCreateInstanceHook(
    REFCLSID  rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD     dwClsContext,
    REFIID    riid,
    LPVOID   *ppv
)
{
    DWORD dwVal = 0, dwSize = sizeof(DWORD);
    if (IsEqualGUID(rclsid, &CLSID_ImmersiveShell) &&
        IsEqualGUID(riid, &IID_IServiceProvider) &&
        SHRegGetValueFromHKCUHKLMFunc)
    {
        SHRegGetValueFromHKCUHKLMFunc(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell",
                                      L"UseWin32BatteryFlyout", SRRF_RT_REG_DWORD, NULL, &dwVal, &dwSize);

        if (!dwVal) {
            if (hCheckForegroundThread) {
                if (WaitForSingleObject(hCheckForegroundThread, 0) == WAIT_TIMEOUT)
                    return E_NOINTERFACE;
                WaitForSingleObject(hCheckForegroundThread, INFINITE);
                CloseHandle(hCheckForegroundThread);
                hCheckForegroundThread = NULL;
            }
            HKEY hKey = NULL;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"" SEH_REGPATH, 0, NULL, REG_OPTION_VOLATILE, KEY_READ, NULL, &hKey, NULL) == ERROR_SUCCESS)
                RegCloseKey(hKey);
            TerminateShellExperienceHost();
            InvokeFlyout(0, INVOKE_FLYOUT_BATTERY);
            Sleep(100);
            hCheckForegroundThread = CreateThread(0, 0, CheckForegroundThread, dwVal, 0, 0);
        }
    }
    return CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}
#endif
#pragma endregion


#pragma region "Show WiFi networks on network icon click"
#ifdef _WIN64
static HRESULT pnidui_CoCreateInstanceHook(
    REFCLSID  rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD     dwClsContext,
    REFIID    riid,
    LPVOID   *ppv
)
{
    DWORD dwVal = 0, dwSize = sizeof(DWORD);
    if (IsEqualGUID(rclsid, &CLSID_ImmersiveShell) &&
        IsEqualGUID(riid, &IID_IServiceProvider) &&
        SHRegGetValueFromHKCUHKLMFunc)
    {
        SHRegGetValueFromHKCUHKLMFunc(
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Settings\\Network",
            L"ReplaceVan", SRRF_RT_REG_DWORD, NULL, &dwVal, (LPDWORD)(&dwSize)
        );
        if (dwVal) {
            if (dwVal == 5 || dwVal == 6) {
                if (hCheckForegroundThread) {
                    WaitForSingleObject(hCheckForegroundThread, INFINITE);
                    CloseHandle(hCheckForegroundThread);
                    hCheckForegroundThread = NULL;
                }
                if (milliseconds_now() - elapsedCheckForeground > CHECKFOREGROUNDELAPSED_TIMEOUT) {
                    LaunchNetworkTargets(dwVal);
                    hCheckForegroundThread = CreateThread(0, 0, CheckForegroundThread, dwVal, 0, 0);
                }
            } else {
                LaunchNetworkTargets(dwVal);
            }
            return E_NOINTERFACE;
        } else {
            if (hCheckForegroundThread) {
                if (WaitForSingleObject(hCheckForegroundThread, 0) == WAIT_TIMEOUT)
                    return E_NOINTERFACE;
                WaitForSingleObject(hCheckForegroundThread, INFINITE);
                CloseHandle(hCheckForegroundThread);
                hCheckForegroundThread = NULL;
            }
            HKEY hKey = NULL;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"" SEH_REGPATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, NULL) == ERROR_SUCCESS)
                RegCloseKey(hKey);
            TerminateShellExperienceHost();
            InvokeFlyout(0, INVOKE_FLYOUT_NETWORK);
            Sleep(100);
            hCheckForegroundThread = CreateThread(0, 0, CheckForegroundThread, dwVal, 0, 0);
        }
    }
    return CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}
#endif
#pragma endregion


#pragma region "Clock flyout helper"
#ifdef _WIN64
typedef struct _ClockButton_ToggleFlyoutCallback_Params {
    void    *TrayUIInstance;
    unsigned CLOCKBUTTON_OFFSET_IN_TRAYUI;
    void    *oldClockButtonInstance;
} ClockButton_ToggleFlyoutCallback_Params;

static void ClockButton_ToggleFlyoutCallback(
    HWND    hWnd,
    UINT    uMsg,
    ClockButton_ToggleFlyoutCallback_Params *params,
    LRESULT lRes
)
{
    *((INT64 *)params->TrayUIInstance + params->CLOCKBUTTON_OFFSET_IN_TRAYUI) = params->oldClockButtonInstance;
    free(params);
}

static BOOL InvokeClockFlyout(void)
{
    POINT ptCursor;
    GetCursorPos(&ptCursor);
    HWND hWnd           = GetMonitorInfoFromPointForTaskbarFlyoutActivation(ptCursor, MONITOR_DEFAULTTOPRIMARY, NULL);
    HWND prev_hWnd      = hWnd;
    HWND hShellTray_Wnd = FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL);

    const unsigned WM_TOGGLE_CLOCK_FLYOUT = 1486;

    if (hWnd == hShellTray_Wnd) {
        if (ShouldShowLegacyClockExperience() == 1) {
            if (!FindWindowW(L"ClockFlyoutWindow", NULL)) {
                if (bOldTaskbar) {
                    return ShowLegacyClockExperience(FindWindowExW(
                        FindWindowExW(hShellTray_Wnd, NULL, L"TrayNotifyWnd", NULL), NULL, L"TrayClockWClass", NULL
                    ));
                } else {
                    POINT pt;
                    pt.x = 0;
                    pt.y = 0;
                    GetCursorPos(&pt);
                    BOOL  bBottom, bRight;
                    POINT dPt = GetDefaultWinXPosition(FALSE, NULL, NULL, FALSE, TRUE);
                    SetCursorPos(dPt.x - 1, dPt.y);
                    BOOL bRet = ShowLegacyClockExperience(hShellTray_Wnd);
                    SetCursorPos(pt.x, pt.y);
                    return bRet;
                }
            } else {
                return PostMessageW(FindWindowW(L"ClockFlyoutWindow", NULL), WM_CLOSE, 0, 0);
            }
        } else if (ShouldShowLegacyClockExperience() == 2) {
            ToggleNotificationsFlyout();
            return 0;
        }
        // On the main monitor, the TrayUI component of CTray handles this
        // message and basically does a `ClockButton::ToggleFlyout`; that's
        // the only place in code where that is used, otherwise, clicking and
        // dismissing the clock flyout probably involves 2 separate methods
        PostMessageW(hShellTray_Wnd, WM_TOGGLE_CLOCK_FLYOUT, 0, 0);
    } else {
        // Of course, on secondary monitors, the situation is much more
        // complicated; there is no simple way to do this, afaik; the way I do it
        // is to obtain a pointer to TrayUI from CTray (pointers to the classes
        // that created the windows are always available at location 0 in the hWnd)
        // and from there issue a "show clock flyout" message manually, taking care to temporarly
        // change the internal clock button pointer of the class to point
        // to the clock button on the secondary monitor.
        if (bOldTaskbar)
            hWnd = FindWindowExW(hWnd, NULL, L"ClockButton", NULL);
        if (hWnd) {
            if (ShouldShowLegacyClockExperience() == 1) {
                if (!FindWindowW(L"ClockFlyoutWindow", NULL)) {
                    if (bOldTaskbar) {
                        return ShowLegacyClockExperience(hWnd);
                    } else {
                        POINT pt;
                        pt.x = 0;
                        pt.y = 0;
                        GetCursorPos(&pt);
                        BOOL  bBottom, bRight;
                        POINT dPt = GetDefaultWinXPosition(FALSE, NULL, NULL, FALSE, TRUE);
                        SetCursorPos(dPt.x, dPt.y);
                        BOOL bRet = ShowLegacyClockExperience(hWnd);
                        SetCursorPos(pt.x, pt.y);
                        return bRet;
                    }
                } else {
                    return PostMessageW(FindWindowW(L"ClockFlyoutWindow", NULL), WM_CLOSE, 0, 0);
                }
            } else if (ShouldShowLegacyClockExperience() == 2) {
                ToggleNotificationsFlyout();
                return 0;
            }
            if (bOldTaskbar) {
                INT64 *CTrayInstance       = (BYTE *)(GetWindowLongPtrW(hShellTray_Wnd, 0)); // -> CTray
                void  *ClockButtonInstance = (BYTE *)(GetWindowLongPtrW(hWnd, 0));           // -> ClockButton

                // inspect CTray::v_WndProc, look for mentions of
                // CTray::_HandlePowerStatus or patterns like **((_QWORD **)this + 110) + 184i64
                const unsigned TRAYUI_OFFSET_IN_CTRAY = 110;
                // simply inspect vtable of TrayUI
                const unsigned TRAYUI_WNDPROC_POSITION_IN_VTABLE = 4;
                // inspect TrayUI::WndProc, specifically this section
#if 0
                    {
                      if ( (_DWORD)a3 == 1486 ) {
                        v80 = (ClockButton *)*((_QWORD *)this + 100);
                        if ( v80 )
                          ClockButton::ToggleFlyout(v80);
#endif
                const unsigned CLOCKBUTTON_OFFSET_IN_TRAYUI = 100;
                void *TrayUIInstance         = *((INT64 *)CTrayInstance + TRAYUI_OFFSET_IN_CTRAY);
                void *oldClockButtonInstance = *((INT64 *)TrayUIInstance + CLOCKBUTTON_OFFSET_IN_TRAYUI);

                ClockButton_ToggleFlyoutCallback_Params *params = malloc(sizeof(ClockButton_ToggleFlyoutCallback_Params));
                if (params) {
                    *((INT64 *)TrayUIInstance + CLOCKBUTTON_OFFSET_IN_TRAYUI) = ClockButtonInstance;
                    params->TrayUIInstance               = TrayUIInstance;
                    params->CLOCKBUTTON_OFFSET_IN_TRAYUI = CLOCKBUTTON_OFFSET_IN_TRAYUI;
                    params->oldClockButtonInstance       = oldClockButtonInstance;
                    SendMessageCallbackW(hShellTray_Wnd, WM_TOGGLE_CLOCK_FLYOUT, 0, 0,
                                         ClockButton_ToggleFlyoutCallback, params);
                }
            } else {
                PostMessageW(hShellTray_Wnd, WM_TOGGLE_CLOCK_FLYOUT, 0, 0);
            }
        }
    }

    return TRUE;
}

static INT64 winrt_Windows_Internal_Shell_implementation_MeetAndChatManager_OnMessageHook(void *_this, INT64 a2, INT a3)
{
    if (!bClockFlyoutOnWinC) {
        if (winrt_Windows_Internal_Shell_implementation_MeetAndChatManager_OnMessageFunc)
            return winrt_Windows_Internal_Shell_implementation_MeetAndChatManager_OnMessageFunc(_this, a2, a3);
        return 0;
    }
    if (a2 == 786 && a3 == 107)
        InvokeClockFlyout();
    return 0;
}
#endif
#pragma endregion


#pragma region "Enable old taskbar"
#ifdef _WIN64
DEFINE_GUID(GUID_18C02F2E_2754_5A20_8BD5_0B34CE79DA2B,
            0x18C02F2E, 0x2754, 0x5A20, 0x8B, 0xD5, 0x0B, 0x34, 0xCE, 0x79, 0xDA, 0x2B);

static IActivationFactory *ep_XamlExtensions_BackupFactory_backer;
IActivationFactory **ep_XamlExtensions_BackupFactory = &ep_XamlExtensions_BackupFactory_backer;

static HRESULT explorer_RoGetActivationFactoryHook(HSTRING activatableClassId, GUID *iid, void **factory)
{
    PCWSTR StringRawBuffer = WindowsGetStringRawBuffer(activatableClassId, 0);
    if (WStrEq(StringRawBuffer, L"WindowsUdk.ApplicationModel.AppExtensions.XamlExtensions") &&
        IsEqualGUID(iid, &GUID_18C02F2E_2754_5A20_8BD5_0B34CE79DA2B))
    {
        Sleep(5000);
        RoGetActivationFactory(activatableClassId, iid, ep_XamlExtensions_BackupFactory);
        *factory = &XamlExtensionsFactory;
        return S_OK;
    }
    return RoGetActivationFactory(activatableClassId, iid, factory);
}

static FARPROC explorer_GetProcAddressHook(HMODULE hModule, const CHAR *lpProcName)
{
    if ((*((WORD *)&(lpProcName) + 1)) && !strncmp(lpProcName, "RoGetActivationFactory", 22))
        return (FARPROC)explorer_RoGetActivationFactoryHook;
    else
        return GetProcAddress(hModule, lpProcName);
}
#endif
#pragma endregion


#pragma region "Open power user menu on Win+X"
#ifdef _WIN64
static LRESULT explorer_SendMessageW(HWND hWndx, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == TB_GETTEXTROWS) {
        HWND hWnd = FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL);
        if (hWnd) {
            hWnd = FindWindowExW(hWnd, NULL, L"RebarWindow32", NULL);
            if (hWnd) {
                hWnd = FindWindowExW(hWnd, NULL, L"MSTaskSwWClass", NULL);
                if (hWnd && hWnd == hWndx && wParam == (WPARAM)-1) {
                    ToggleLauncherTipContextMenu();
                    return 0;
                }
            }
        }
    }
    return SendMessageW(hWndx, uMsg, wParam, lParam);
}
#endif
#pragma endregion


#pragma region "Set up taskbar button hooks, implement Weather widget"
#ifdef _WIN64

static DWORD ShouldShowWidgetsInsteadOfCortana(void)
{
    DWORD dwVal = 0, dwSize = sizeof(DWORD);
    if (SHRegGetValueFromHKCUHKLMFunc &&
        SHRegGetValueFromHKCUHKLMFunc(
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
            L"TaskbarDa", SRRF_RT_REG_DWORD, NULL, &dwVal, (LPDWORD)(&dwSize)
        ) == ERROR_SUCCESS)
    {
        return dwVal;
    }
    return 0;
}

static __int64 Widgets_OnClickHook(__int64 a1, __int64 a2)
{
    if (ShouldShowWidgetsInsteadOfCortana() == 1) {
        ToggleWidgetsPanel();
        return 0;
    } else {
        if (Widgets_OnClickFunc)
            return Widgets_OnClickFunc(a1, a2);
        return 0;
    }
}

static HRESULT WINAPI Widgets_GetTooltipTextHook(__int64 a1, __int64 a2, __int64 a3, WCHAR *a4, UINT a5)
{
    if (ShouldShowWidgetsInsteadOfCortana() == 1) {
        return SHLoadIndirectString(
            L"@{windows?ms-resource://Windows.UI.SettingsAppThreshold/SystemSettings/"
            L"Resources/SystemSettings_DesktopTaskbar_Da2/DisplayName}",
            a4, a5, 0
        );
    } else {
        if (Widgets_GetTooltipTextFunc)
            return Widgets_GetTooltipTextFunc(a1, a2, a3, a4, a5);
        return 0;
    }
}

#if 0
int WINAPI explorer_LoadStringWHook(HINSTANCE hInstance, UINT uID, WCHAR* lpBuffer, UINT cchBufferMax)
{
    WCHAR wszBuffer[MAX_PATH];
    if (hInstance == GetModuleHandleW(NULL) && uID == 912)// && SUCCEEDED(epw->lpVtbl->GetTitle(epw, MAX_PATH, wszBuffer)))
    {
        //sws_error_PrintStackTrace();
        int rez = LoadStringW(hInstance, uID, lpBuffer, cchBufferMax);
        //wprintf(L"%s\n", lpBuffer);
        return rez;
    }
    else {
        return LoadStringW(hInstance, uID, lpBuffer, cchBufferMax);
    }
}
#endif

static void stub1(void *i)
{
}

# define WEATHER_FIXEDSIZE2_MAXWIDTH 192
static BOOL explorer_DeleteMenu(HMENU hMenu, UINT uPosition, UINT uFlags)
{
    if (uPosition == 621 && uFlags == 0) // when removing News and interests
    {
        DeleteMenu(hMenu, 449, 0); // remove Cortana menu
        DeleteMenu(hMenu, 435, 0); // remove People menu
    }
    if (!IsWindows11() && uPosition == 445 && uFlags == 0) // when removing Cortana in Windows 10
        DeleteMenu(hMenu, 435, 0);
    return DeleteMenu(hMenu, uPosition, uFlags);
}

static void RecomputeWeatherFlyoutLocation(HWND hWnd)
{
    RECT rcButton;
    GetWindowRect(PeopleButton_LastHWND, &rcButton);
    POINT pButton;
    pButton.x = rcButton.left;
    pButton.y = rcButton.top;

    RECT rcWeatherFlyoutWindow;
    GetWindowRect(hWnd, &rcWeatherFlyoutWindow);

    POINT pNewWindow;

    RECT rc;
    UINT tbPos = GetTaskbarLocationAndSize(pButton, &rc);
    if (tbPos == TB_POS_BOTTOM)
        pNewWindow.y = rcButton.top - (rcWeatherFlyoutWindow.bottom - rcWeatherFlyoutWindow.top);
    else if (tbPos == TB_POS_TOP)
        pNewWindow.y = rcButton.bottom;
    else if (tbPos == TB_POS_LEFT)
        pNewWindow.x = rcButton.right;
    if (tbPos == TB_POS_RIGHT)
        pNewWindow.x = rcButton.left - (rcWeatherFlyoutWindow.right - rcWeatherFlyoutWindow.left);

    if (tbPos == TB_POS_BOTTOM || tbPos == TB_POS_TOP) {
        pNewWindow.x = rcButton.left + ((rcButton.right - rcButton.left) / 2) -
                       ((rcWeatherFlyoutWindow.right - rcWeatherFlyoutWindow.left) / 2);

        HMONITOR hMonitor = MonitorFromPoint(pButton, MONITOR_DEFAULTTOPRIMARY);
        if (hMonitor) {
            MONITORINFO mi;
            mi.cbSize = sizeof(MONITORINFO);
            if (GetMonitorInfoW(hMonitor, &mi)) {
                if (mi.rcWork.right < pNewWindow.x + (rcWeatherFlyoutWindow.right - rcWeatherFlyoutWindow.left))
                    pNewWindow.x = mi.rcWork.right - (rcWeatherFlyoutWindow.right - rcWeatherFlyoutWindow.left);
                if (mi.rcWork.left > pNewWindow.x)
                    pNewWindow.x = mi.rcWork.left;
            }
        }
    } else if (tbPos == TB_POS_LEFT || tbPos == TB_POS_RIGHT) {
        pNewWindow.y = rcButton.top + ((rcButton.bottom - rcButton.top) / 2) -
                       ((rcWeatherFlyoutWindow.bottom - rcWeatherFlyoutWindow.top) / 2);

        HMONITOR hMonitor = MonitorFromPoint(pButton, MONITOR_DEFAULTTOPRIMARY);
        if (hMonitor) {
            MONITORINFO mi;
            mi.cbSize = sizeof(MONITORINFO);
            if (GetMonitorInfoW(hMonitor, &mi)) {
                if (mi.rcWork.bottom < pNewWindow.y + (rcWeatherFlyoutWindow.bottom - rcWeatherFlyoutWindow.top))
                    pNewWindow.y = mi.rcWork.bottom - (rcWeatherFlyoutWindow.bottom - rcWeatherFlyoutWindow.top);
                if (mi.rcWork.top > pNewWindow.y)
                    pNewWindow.y = mi.rcWork.top;
            }
        }
    }

    SetWindowPos(hWnd, NULL, pNewWindow.x, pNewWindow.y, 0, 0, SWP_NOSIZE | SWP_NOSENDCHANGING);
}

static SIZE WINAPI PeopleButton_CalculateMinimumSizeHook(void *_this, SIZE *pSz)
{
    SIZE ret        = PeopleButton_CalculateMinimumSizeFunc(_this, pSz);
    BOOL bHasLocked = TryEnterCriticalSection(&lock_epw);

    if (bHasLocked && epw) {
        if (bWeatherFixedSize == 1) {
            int mul = 1;
            switch (dwWeatherViewMode) {
            case EP_WEATHER_VIEW_ICONTEXT:
                mul = 4;
                break;
            case EP_WEATHER_VIEW_TEXTONLY:
                mul = 3;
                break;
            case EP_WEATHER_VIEW_ICONTEMP:
                mul = 2;
                break;
            case EP_WEATHER_VIEW_ICONONLY:
            case EP_WEATHER_VIEW_TEMPONLY:
                mul = 1;
                break;
            }
            pSz->cx = pSz->cx * mul;
        } else {
            if (!prev_total_h)
                pSz->cx = 10000;
            else
                pSz->cx = prev_total_h;
        }
        // printf("[CalculateMinimumSize] %d %d\n", pSz->cx, pSz->cy);
        if (pSz->cy) {
            BOOL    bIsInitialized = TRUE;
            HRESULT hr = epw->lpVtbl->IsInitialized(epw, &bIsInitialized);
            if (SUCCEEDED(hr)) {
                int rt = MulDiv(48, pSz->cy, 60);
                if (!bIsInitialized) {
                    epw->lpVtbl->SetTerm(epw, sizeof wszWeatherTerm, wszWeatherTerm);
                    epw->lpVtbl->SetLanguage(epw, sizeof wszWeatherLanguage, wszWeatherLanguage);
                    epw->lpVtbl->SetDevMode(epw, dwWeatherDevMode, FALSE);
                    epw->lpVtbl->SetIconPack(epw, dwWeatherIconPack, FALSE);
                    UINT        dpiX = 0, dpiY = 0;
                    HMONITOR    hMonitor = MonitorFromWindow(PeopleButton_LastHWND, MONITOR_DEFAULTTOPRIMARY);
                    HRESULT     hr       = GetDpiForMonitor(hMonitor, MDT_DEFAULT, &dpiX, &dpiY);
                    MONITORINFO mi       = {.cbSize = sizeof(MONITORINFO)};

                    if (GetMonitorInfoW(hMonitor, &mi)) {
                        DWORD dwTextScaleFactor = 0, dwSize = sizeof(DWORD);
                        if (SHRegGetValueFromHKCUHKLMFunc &&
                            SHRegGetValueFromHKCUHKLMFunc(
                                L"SOFTWARE\\Microsoft\\Accessibility", L"TextScaleFactor",
                                SRRF_RT_REG_DWORD, NULL, &dwTextScaleFactor, (LPDWORD)(&dwSize)
                            ) != ERROR_SUCCESS)
                        {
                            dwTextScaleFactor = 100;
                        }

                        RECT rcWeatherFlyoutWindow;
                        rcWeatherFlyoutWindow.left = mi.rcWork.left;
                        rcWeatherFlyoutWindow.top  = mi.rcWork.top;
                        rcWeatherFlyoutWindow.right =
                            rcWeatherFlyoutWindow.left +
                            MulDiv(MulDiv(MulDiv(EP_WEATHER_WIDTH, dpiX, 96), dwTextScaleFactor, 100), dwWeatherZoomFactor, 100);
                        rcWeatherFlyoutWindow.bottom =
                            rcWeatherFlyoutWindow.top +
                            MulDiv(MulDiv(MulDiv(EP_WEATHER_HEIGHT, dpiX, 96), dwTextScaleFactor, 100), dwWeatherZoomFactor, 100);
                        int k = 0;
                        while (FAILED(
                            hr = epw->lpVtbl->Initialize(
                                epw, wszEPWeatherKillswitch, bAllocConsole, EP_WEATHER_PROVIDER_GOOGLE, rt, rt,
                                dwWeatherTemperatureUnit, dwWeatherUpdateSchedule * 1000, rcWeatherFlyoutWindow,
                                dwWeatherTheme, dwWeatherGeolocationMode, &hWndWeatherFlyout,
                                dwWeatherZoomFactor ? dwWeatherZoomFactor : 100, dpiX, dpiY
                            )
                        ))
                        {
                            BOOL bFailed = FALSE;
                            if (k == 0 && hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                                if (DownloadAndInstallWebView2Runtime())
                                    k++;
                                else
                                    bFailed = TRUE;
                            else
                                bFailed = TRUE;
                            if (bFailed) {
                                prev_total_h = 0;
                                PostMessageW(FindWindowW(L"Shell_TrayWnd", NULL), WM_COMMAND, 435, 0);
                                // PostMessageW(FindWindowW(L"ExplorerPatcher_GUI_" EP_CLSID, NULL),
                                // WM_USER + 1, 0, 0);
                                if (hServiceWindowThread)
                                    PostThreadMessageW(GetThreadId(hServiceWindowThread), WM_USER + 1, NULL, NULL);
                                break;
                            }
                        }
                        if (SUCCEEDED(hr))
                            epw->lpVtbl->SetWindowCornerPreference(epw, dwWeatherWindowCornerPreference);
                    }
                } else {
                    epw->lpVtbl->SetIconSize(epw, rt, rt);
                }
            } else if (hr == 0x800706BA) // RPC server is unavailable
            {
#if 0
                // ReleaseSRWLockShared(&lock_epw);
                AcquireSRWLockExclusive(&lock_epw);
                epw = NULL;
                prev_total_h = 0;
                InvalidateRect(PeopleButton_LastHWND, NULL, TRUE);
                ReleaseSRWLockExclusive(&lock_epw);
#endif
                if (hServiceWindowThread)
                    PostThreadMessageW(GetThreadId(hServiceWindowThread), WM_USER + 1, NULL, NULL);
#if 0
                // AcquireSRWLockShared(&lock_epw);
#endif
            }
        }
        LeaveCriticalSection(&lock_epw);
    } else {
        if (bHasLocked)
            LeaveCriticalSection(&lock_epw);
    }
    return ret;
}

static int PeopleBand_MulDivHook(int nNumber, int nNumerator, int nDenominator)
{
    if (nNumber != 46) // 46 = vertical taskbar, 48 = horizontal taskbar
    {
        // printf("[MulDivHook] %d %d %d\n", nNumber, nNumerator, nDenominator);
        BOOL bHasLocked = TryEnterCriticalSection(&lock_epw);
        if (bHasLocked && epw) {
            if (bWeatherFixedSize == 1) {
                int mul = 1;
                switch (dwWeatherViewMode) {
                case EP_WEATHER_VIEW_ICONTEXT:
                    mul = 4;
                    break;
                case EP_WEATHER_VIEW_TEXTONLY:
                    mul = 3;
                    break;
                case EP_WEATHER_VIEW_ICONTEMP:
                    mul = 2;
                    break;
                case EP_WEATHER_VIEW_ICONONLY:
                case EP_WEATHER_VIEW_TEMPONLY:
                    mul = 1;
                    break;
                }
                LeaveCriticalSection(&lock_epw);
                return MulDiv(nNumber * mul, nNumerator, nDenominator);
            } else if (prev_total_h) {
                LeaveCriticalSection(&lock_epw);
                return prev_total_h;
            } else {
                prev_total_h = MulDiv(nNumber, nNumerator, nDenominator);
                LeaveCriticalSection(&lock_epw);
                return prev_total_h;
            }
            LeaveCriticalSection(&lock_epw);
        } else {
            if (bHasLocked)
                LeaveCriticalSection(&lock_epw);
        }
    }
    return MulDiv(nNumber, nNumerator, nDenominator);
}


static __int64 __fastcall
PeopleBand_DrawTextWithGlowHook(
    HDC    hdc,
    uint16_t const *a2,
    INT    a3,
    RECT  *a4,
    UINT   a5,
    UINT   a6,
    UINT   a7,
    UINT   dy,
    UINT   a9,
    INT    a10,
    PeopleBandDrawTextWithGlowHook_CB_t a11,
    INT64  a12
)
{
    BOOL bHasLocked = FALSE;
    if (a5 == 0x21 && (bHasLocked = TryEnterCriticalSection(&lock_epw)) && epw) {
        bPeopleHasEllipsed = FALSE;

        BOOL bUseCachedData = InSendMessage();
        BOOL bIsThemeActive = TRUE;
        if (!IsThemeActive() || IsHighContrast())
            bIsThemeActive = FALSE;
        HRESULT hr = S_OK;

        if (bUseCachedData ? TRUE : SUCCEEDED(hr = epw->lpVtbl->LockData(epw)))
        {
            UINT    dpiX = 0, dpiY = 0;
            HRESULT hr = GetDpiForMonitor(MonitorFromWindow(PeopleButton_LastHWND, MONITOR_DEFAULTTOPRIMARY),
                                          MDT_DEFAULT, &dpiX, &dpiY);
            BOOL  bShouldUnlockData = TRUE;
            DWORD cbTemperature     = 0;
            DWORD cbUnit            = 0;
            DWORD cbCondition       = 0;
            DWORD cbImage           = 0;
            BOOL  bEmptyData        = FALSE;

            if (bUseCachedData
                    ? TRUE
                    : SUCCEEDED(hr = epw->lpVtbl->GetDataSizes(epw, &cbTemperature, &cbUnit, &cbCondition, &cbImage)))
            {
                if (cbTemperature && cbUnit && cbCondition && cbImage) {
                    epw_cbTemperature = cbTemperature;
                    epw_cbUnit        = cbUnit;
                    epw_cbCondition   = cbCondition;
                    epw_cbImage       = cbImage;
                } else {
                    if (!bUseCachedData) {
                        bEmptyData = TRUE;
                        if (bShouldUnlockData) {
                            epw->lpVtbl->UnlockData(epw);
                            bShouldUnlockData = FALSE;
                        }
                    } else {
                        bEmptyData = !epw_wszTemperature || !epw_wszUnit || !epw_wszCondition;
                    }
                    bUseCachedData = TRUE;
                }
                if (!bUseCachedData) {
                    free(epw_wszTemperature);
                    epw_wszTemperature = calloc(1, epw_cbTemperature);
                    free(epw_wszUnit);
                    epw_wszUnit = calloc(1, epw_cbUnit);
                    free(epw_wszCondition);
                    epw_wszCondition = calloc(1, epw_cbCondition);
                    free(epw_pImage);
                    epw_pImage = calloc(1, epw_cbImage);
                }
                if (bUseCachedData
                        ? TRUE
                        : SUCCEEDED(hr = epw->lpVtbl->GetData(epw, epw_cbTemperature, epw_wszTemperature, epw_cbUnit, epw_wszUnit,
                                                              epw_cbCondition, epw_wszCondition, epw_cbImage, epw_pImage)))
                {
                    if (!bUseCachedData) {
                        WCHAR wszBuffer[MAX_PATH];
                        ZeroMemory(wszBuffer, sizeof(WCHAR) * MAX_PATH);
                        swprintf_s(wszBuffer, MAX_PATH, L"%s %s, %s, ", epw_wszTemperature, epw_wszUnit, epw_wszCondition);
                        int len = wcslen(wszBuffer);
                        epw->lpVtbl->GetTitle(epw, sizeof(WCHAR) * (MAX_PATH - len), wszBuffer + len, dwWeatherViewMode);
                        SetWindowTextW(PeopleButton_LastHWND, wszBuffer);

                        epw->lpVtbl->UnlockData(epw);
                        bShouldUnlockData = FALSE;
                    }

                    LOGFONTW logFont;
                    ZeroMemory(&logFont, sizeof(logFont));
                    LOGFONTW logFont2;
                    ZeroMemory(&logFont2, sizeof(logFont2));
                    NONCLIENTMETRICS ncm;
                    ZeroMemory(&ncm, sizeof(NONCLIENTMETRICS));
                    ncm.cbSize = sizeof(NONCLIENTMETRICS);
                    SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0, dpiX);
                    logFont           = ncm.lfCaptionFont;
                    logFont.lfWeight  = FW_NORMAL;
                    logFont2          = ncm.lfCaptionFont;
                    logFont2.lfWeight = FW_NORMAL;
                    logFont2.lfHeight += 1;
                    if (bEmptyData) {
                        if (!dwTaskbarSmallIcons)
                            logFont.lfHeight *= 1.6;
                    } else if (dwWeatherViewMode == EP_WEATHER_VIEW_ICONTEXT) {
                        // logFont.lfHeight = -12 * (dpiX / 96.0);
                    }

                    HFONT hFont  = CreateFontIndirectW(&logFont);
                    HFONT hFont2 = CreateFontIndirectW(&logFont2);
                    if (hFont) {
                        HDC hDC = CreateCompatibleDC(0);
                        if (hDC) {
                            COLORREF rgbColor = RGB(0, 0, 0);
                            if (bIsThemeActive) {
                                if ((global_rovi.dwBuildNumber < 18985) || (ShouldSystemUseDarkMode && ShouldSystemUseDarkMode()))
                                    rgbColor = RGB(255, 255, 255);
                            } else {
                                rgbColor = GetSysColor(COLOR_BTNTEXT);
                            }
                            HFONT hOldFont = SelectFont(hDC, hFont);

                            if (bEmptyData) {
                                RECT rcText;
                                SetRect(&rcText, 0, 0, a4->right, a4->bottom);
                                SIZE size;
                                size.cx             = rcText.right - rcText.left;
                                size.cy             = rcText.bottom - rcText.top;
                                DWORD   dwTextFlags = DT_SINGLELINE | DT_VCENTER | DT_HIDEPREFIX | DT_CENTER;
                                HBITMAP hBitmap     = sws_WindowHelpers_CreateAlphaTextBitmap(
                                    L"\U0001f4f0", hFont, dwTextFlags, size, rgbColor
                                );
                                if (hBitmap) {
                                    HBITMAP hOldBMP = SelectBitmap(hDC, hBitmap);
                                    BITMAP  BMInf;
                                    GetObjectW(hBitmap, sizeof(BITMAP), &BMInf);

                                    BLENDFUNCTION bf;
                                    bf.BlendOp             = AC_SRC_OVER;
                                    bf.BlendFlags          = 0;
                                    bf.SourceConstantAlpha = 0xFF;
                                    bf.AlphaFormat         = AC_SRC_ALPHA;

                                    GdiAlphaBlend(hdc, 0, 0, BMInf.bmWidth, BMInf.bmHeight, hDC,
                                                  0, 0, BMInf.bmWidth, BMInf.bmHeight, bf);
                                    SelectBitmap(hDC, hOldBMP);
                                    DeleteBitmap(hBitmap);
                                }
                            } else {
                                DWORD dwWeatherSplit = (dwWeatherContentsMode &&
                                                        (dwWeatherViewMode == EP_WEATHER_VIEW_ICONTEXT ||
                                                         dwWeatherViewMode == EP_WEATHER_VIEW_TEXTONLY) &&
                                                        !dwTaskbarSmallIcons);

                                DWORD dwTextFlags = DT_SINGLELINE | DT_HIDEPREFIX;

                                WCHAR wszText1[MAX_PATH];
                                swprintf_s(wszText1, MAX_PATH, L"%s%s %s",
                                           bIsThemeActive ? L"" : L" ", epw_wszTemperature,
                                           dwWeatherTemperatureUnit == EP_WEATHER_TUNIT_FAHRENHEIT ? L"\u00B0F" : L"\u00B0C"); // epw_wszUnit);
                                RECT rcText1;
                                SetRect(&rcText1, 0, 0, a4->right, dwWeatherSplit ? (a4->bottom / 2) : a4->bottom);
                                DrawTextW(hDC, wszText1, -1, &rcText1,
                                          dwTextFlags | DT_CALCRECT | (dwWeatherSplit ? DT_BOTTOM : DT_VCENTER));
                                rcText1.bottom = dwWeatherSplit ? (a4->bottom / 2) : a4->bottom;
                                WCHAR wszText2[MAX_PATH];
                                swprintf_s(wszText2, MAX_PATH, L"%s%s", bIsThemeActive ? L"" : L" ", epw_wszCondition);
                                RECT rcText2;
                                SetRect(&rcText2, 0, 0, a4->right, dwWeatherSplit ? (a4->bottom / 2) : a4->bottom);
                                DrawTextW(hDC, wszText2, -1, &rcText2,
                                          dwTextFlags | DT_CALCRECT | (dwWeatherSplit ? DT_TOP : DT_VCENTER));
                                rcText2.bottom = dwWeatherSplit ? (a4->bottom / 2) : a4->bottom;

                                if (bWeatherFixedSize)
                                    dwTextFlags |= DT_END_ELLIPSIS;

                                int addend = 0;
                                // int rt = MulDiv(48, a4->bottom, 60);
                                int rt       = sqrt(epw_cbImage / 4);
                                int p        = 0; // MulDiv(rt, 4, 64);
                                int margin_h = MulDiv(12, dpiX, 144);

                                BOOL bIsIconMode = (dwWeatherViewMode == EP_WEATHER_VIEW_ICONTEMP ||
                                                    dwWeatherViewMode == EP_WEATHER_VIEW_ICONTEXT ||
                                                    dwWeatherViewMode == EP_WEATHER_VIEW_ICONONLY);

                                switch (dwWeatherViewMode) {
                                case EP_WEATHER_VIEW_ICONTEXT:
                                case EP_WEATHER_VIEW_TEXTONLY:
                                    if (dwWeatherSplit)
                                        addend = MAX((rcText1.right - rcText1.left), (rcText2.right - rcText2.left)) +
                                                 margin_h;
                                    else
                                        addend = (rcText1.right - rcText1.left) + margin_h +
                                                 (rcText2.right - rcText2.left) + margin_h;
                                    break;
                                case EP_WEATHER_VIEW_ICONTEMP:
                                case EP_WEATHER_VIEW_TEMPONLY:
                                    addend = (rcText1.right - rcText1.left) + margin_h;
                                    break;
                                case EP_WEATHER_VIEW_ICONONLY:
                                    addend = 0;
                                    break;
                                }

                                int margin_v = (a4->bottom - rt) / 2;
                                int total_h  = (bIsIconMode ? ((margin_h - p) + rt + (margin_h - p)) : margin_h) + addend;
                                if (bWeatherFixedSize == 1) {
                                    if (total_h > a4->right) {
                                        int diff = total_h - a4->right;
                                        rcText2.right -= diff - 2;
                                        bPeopleHasEllipsed = TRUE;
                                        switch (dwWeatherViewMode) {
                                        case EP_WEATHER_VIEW_ICONTEXT:
                                        case EP_WEATHER_VIEW_TEXTONLY:
                                            if (dwWeatherSplit)
                                                addend = MAX((rcText1.right - rcText1.left), (rcText2.right - rcText2.left)) + margin_h;
                                            else
                                                addend = (rcText1.right - rcText1.left) + margin_h + (rcText2.right - rcText2.left) + margin_h;
                                            break;
                                        case EP_WEATHER_VIEW_ICONTEMP:
                                        case EP_WEATHER_VIEW_TEMPONLY: // should be impossible
                                            addend = (rcText1.right - rcText1.left) + margin_h;
                                            break;
                                        case EP_WEATHER_VIEW_ICONONLY:
                                            addend = 0;
                                            break;
                                        }
                                        total_h = (margin_h - p) + rt + (margin_h - p) + addend;
                                    }
                                }

                                int start_x = 0; // prev_total_h - total_h;
                                if (bWeatherFixedSize == 1)
                                    start_x = (a4->right - total_h) / 2;
                                if (bWeatherFixedSize == 2 && (total_h > MulDiv(192, dpiX, 96))) {
                                    int diff = total_h - MulDiv(WEATHER_FIXEDSIZE2_MAXWIDTH, dpiX, 96);
                                    rcText2.right -= diff - 2;
                                    total_h             = MulDiv(WEATHER_FIXEDSIZE2_MAXWIDTH, dpiX, 96);
                                    bPeopleHasEllipsed = TRUE;
                                }

                                HBITMAP hBitmap = NULL, hOldBitmap = NULL;
                                void   *pvBits = NULL;
                                SIZE    size;

                                if (bIsIconMode) {
                                    BITMAPINFOHEADER BMIH = {
                                        .biSize        = sizeof BMIH,
                                        .biWidth       = rt,
                                        .biHeight      = -rt,
                                        .biPlanes      = 1,
                                        .biBitCount    = 32,
                                        .biCompression = BI_RGB,
                                    };
                                    hBitmap = CreateDIBSection(hDC, &BMIH, 0, &pvBits, NULL, 0);

                                    if (hBitmap) {
                                        memcpy(pvBits, epw_pImage, epw_cbImage);
                                        hOldBitmap = SelectBitmap(hDC, hBitmap);

                                        BLENDFUNCTION bf = {
                                            .BlendOp             = AC_SRC_OVER,
                                            .BlendFlags          = 0,
                                            .SourceConstantAlpha = 0xFF,
                                            .AlphaFormat         = AC_SRC_ALPHA,
                                        };
                                        GdiAlphaBlend(hdc, start_x + (margin_h - p), margin_v, rt, rt, hDC, 0, 0, rt, rt, bf);

                                        SelectBitmap(hDC, hOldBitmap);
                                        DeleteBitmap(hBitmap);
                                    }
                                }

                                if (dwWeatherViewMode == EP_WEATHER_VIEW_ICONTEMP ||
                                    dwWeatherViewMode == EP_WEATHER_VIEW_ICONTEXT ||
                                    dwWeatherViewMode == EP_WEATHER_VIEW_TEMPONLY ||
                                    dwWeatherViewMode == EP_WEATHER_VIEW_TEXTONLY)
                                {
                                    size.cx = rcText1.right - rcText1.left;
                                    size.cy = rcText1.bottom - rcText1.top;
                                    hBitmap = sws_WindowHelpers_CreateAlphaTextBitmap(
                                        wszText1, hFont, dwTextFlags | (dwWeatherSplit ? DT_BOTTOM : DT_VCENTER),
                                        size, rgbColor
                                    );
                                    if (hBitmap) {
                                        HBITMAP hOldBMP = SelectBitmap(hDC, hBitmap);
                                        BITMAP  BMInf;
                                        GetObjectW(hBitmap, sizeof(BITMAP), &BMInf);

                                        BLENDFUNCTION bf = {
                                            .BlendOp             = AC_SRC_OVER,
                                            .BlendFlags          = 0,
                                            .SourceConstantAlpha = 0xFF,
                                            .AlphaFormat         = AC_SRC_ALPHA,
                                        };
                                        GdiAlphaBlend(
                                            hdc,
                                            start_x + (bIsIconMode ? ((margin_h - p) + rt + (margin_h - p)) : margin_h),
                                            0, BMInf.bmWidth, BMInf.bmHeight, hDC, 0, 0, BMInf.bmWidth, BMInf.bmHeight,
                                            bf
                                        );

                                        SelectBitmap(hDC, hOldBMP);
                                        DeleteBitmap(hBitmap);
                                    }
                                }

                                if (dwWeatherViewMode == EP_WEATHER_VIEW_ICONTEXT ||
                                    dwWeatherViewMode == EP_WEATHER_VIEW_TEXTONLY) {
                                    size.cx = rcText2.right - rcText2.left;
                                    size.cy = rcText2.bottom - rcText2.top;
                                    hBitmap = sws_WindowHelpers_CreateAlphaTextBitmap(
                                        wszText2, (dwWeatherSplit && hFont2 ? hFont2 : hFont),
                                        dwTextFlags | (dwWeatherSplit ? DT_TOP : DT_VCENTER), size, rgbColor
                                    );
                                    if (hBitmap) {
                                        HBITMAP hOldBMP = SelectBitmap(hDC, hBitmap);
                                        BITMAP  BMInf;
                                        GetObjectW(hBitmap, sizeof(BITMAP), &BMInf);

                                        BLENDFUNCTION bf = {
                                            .BlendOp             = AC_SRC_OVER,
                                            .BlendFlags          = 0,
                                            .SourceConstantAlpha = 0xFF,
                                            .AlphaFormat         = AC_SRC_ALPHA,
                                        };
                                        GdiAlphaBlend(
                                            hdc,
                                            start_x +
                                                (bIsIconMode ? ((margin_h - p) + rt + (margin_h - p)) : margin_h) +
                                                (dwWeatherSplit ? -1 : (rcText1.right - rcText1.left) + margin_h),
                                            dwWeatherSplit ? (a4->bottom / 2 - 1) : 0, BMInf.bmWidth, BMInf.bmHeight,
                                            hDC, 0, 0, BMInf.bmWidth, BMInf.bmHeight, bf
                                        );

                                        SelectBitmap(hDC, hOldBMP);
                                        DeleteBitmap(hBitmap);
                                    }
                                }

                                if (bWeatherFixedSize == 1) {

                                } else if (total_h != prev_total_h) {
                                    prev_total_h = total_h;
                                    SendNotifyMessageW(HWND_BROADCAST, WM_WININICHANGE, 0, (LPARAM)L"TraySettings");
                                }

#if 0
                                SetLastError(0);
                                LONG_PTR oldStyle = GetWindowLongPtrW(PeopleButton_LastHWND, GWL_EXSTYLE);
                                if (!GetLastError()) {
                                    LONG_PTR style;
                                    if (bIsThemeActive)
                                        style = oldStyle & ~WS_EX_DLGMODALFRAME;
                                    else
                                        style = oldStyle | WS_EX_DLGMODALFRAME;
                                    if (style != oldStyle)
                                        SetWindowLongPtrW(PeopleButton_LastHWND, GWL_EXSTYLE, style);
                                }
#endif
                            }

                            SelectFont(hDC, hOldFont);
                            DeleteDC(hDC);
                        }
                        DeleteFont(hFont);
                    }

                    if (hFont2)
                        DeleteFont(hFont2);
                    if (IsWindowVisible(hWndWeatherFlyout))
                        RecomputeWeatherFlyoutLocation(hWndWeatherFlyout);
                }
#if 0
                free(epw_pImage);
                free(epw_wszCondition);
                free(epw_wszUnit);
                free(epw_wszTemperature);
#endif
            }
            if (!bUseCachedData && bShouldUnlockData)
                epw->lpVtbl->UnlockData(epw);
        } else {
            // printf("444444444444 0x%x\n", hr);
            if (hr == 0x800706BAu) // RPC server is unavailable
            {
#if 0
                // ReleaseSRWLockShared(&lock_epw);
                AcquireSRWLockExclusive(&lock_epw);
                epw = NULL;
                prev_total_h = 0;
                InvalidateRect(PeopleButton_LastHWND, NULL, TRUE);
                ReleaseSRWLockExclusive(&lock_epw);
#endif
                if (hServiceWindowThread)
                    PostThreadMessageW(GetThreadId(hServiceWindowThread), WM_USER + 1, NULL, NULL);
#if 0
                AcquireSRWLockShared(&lock_epw);
#endif
            }
        }

        // printf("hr %x\n", hr);

        LeaveCriticalSection(&lock_epw);
        return S_OK;
    } else {
        if (bHasLocked)
            LeaveCriticalSection(&lock_epw);
        return PeopleBand_DrawTextWithGlowFunc(hdc, a2, a3, a4, a5, a6, a7, dy, a9, a10, a11, a12);
    }
}

static int WINAPI PeopleButton_ShowTooltipHook(__int64 _this, unsigned __int8 bShow)
{
    BOOL bHasLocked = TryEnterCriticalSection(&lock_epw);

    if (bHasLocked && epw) {
        if (bShow) {
            HRESULT hr = epw->lpVtbl->LockData(epw);
            if (SUCCEEDED(hr)) {
                WCHAR wszBuffer[MAX_PATH] = {0};
                DWORD mode = dwWeatherViewMode;
                if (bWeatherFixedSize && bPeopleHasEllipsed)
                    mode = EP_WEATHER_VIEW_ICONTEMP;
                epw->lpVtbl->GetTitle(epw, sizeof(WCHAR) * MAX_PATH, wszBuffer, mode);
                if (wcsstr(wszBuffer, L"(null)")) {
                    HMODULE hModule = GetModuleHandleW(L"pnidui.dll");
                    if (hModule)
                        LoadStringW(hModule, 35, wszBuffer, MAX_PATH);
                }
                TTTOOLINFOW ti = {
                    .cbSize   = sizeof(TTTOOLINFOW),
                    .hwnd     = *((INT64 *)_this + 1),
                    .uId      = *((INT64 *)_this + 1),
                    .lpszText = wszBuffer,
                };
                SendMessageW((HWND) * ((INT64 *)_this + 10), TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
                epw->lpVtbl->UnlockData(epw);
            }
        }
        LeaveCriticalSection(&lock_epw);
    } else {
        if (bHasLocked)
            LeaveCriticalSection(&lock_epw);
        WCHAR wszBuffer[MAX_PATH] = {0};
        LoadStringW(GetModuleHandleW(NULL), 912, wszBuffer, MAX_PATH);
        if (wszBuffer[0]) {
            TTTOOLINFOW ti = {
                .cbSize   = sizeof(TTTOOLINFOW),
                .hwnd     = *((INT64 *)_this + 1),
                .uId      = *((INT64 *)_this + 1),
                .lpszText = wszBuffer,
            };
            SendMessageW((HWND) * ((INT64 *)_this + 10), TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
        }
    }

    if (PeopleButton_ShowTooltipFunc)
        return PeopleButton_ShowTooltipFunc(_this, bShow);

    return 0;
}

static __int64 PeopleButton_OnClickHook(__int64 a1, __int64 a2)
{
    BOOL bHasLocked = TryEnterCriticalSection(&lock_epw);
    if (bHasLocked && epw) {
        if (!hWndWeatherFlyout)
            epw->lpVtbl->GetWindowHandle(epw, &hWndWeatherFlyout);
        if (hWndWeatherFlyout) {
            if (IsWindowVisible(hWndWeatherFlyout)) {
                if (GetForegroundWindow() != hWndWeatherFlyout) {
                    SwitchToThisWindow(hWndWeatherFlyout, TRUE);
                } else {
                    epw->lpVtbl->Hide(epw);
                    // printf("HR %x\n", PostMessageW(hWnd, EP_WEATHER_WM_FETCH_DATA, 0, 0));
                }
            } else {
                RecomputeWeatherFlyoutLocation(hWndWeatherFlyout);

                epw->lpVtbl->Show(epw);

                SwitchToThisWindow(hWndWeatherFlyout, TRUE);
            }
        }
        LeaveCriticalSection(&lock_epw);
        return 0;
    } else {
        if (bHasLocked)
            LeaveCriticalSection(&lock_epw);
        if (PeopleButton_OnClickFunc)
            return PeopleButton_OnClickFunc(a1, a2);
        return 0;
    }
}

static INT64 PeopleButton_SubclassProc(
    _In_ HWND   hWnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    UINT_PTR    uIdSubclass,
    DWORD_PTR   dwRefData
)
{
    if (uMsg == WM_NCDESTROY) {
        RemoveWindowSubclass(hWnd, PeopleButton_SubclassProc, PeopleButton_SubclassProc);
#if 0
        AcquireSRWLockExclusive(&lock_epw);
        if (epw) {
            epw->lpVtbl->Release(epw);
            epw = NULL;
            PeopleButton_LastHWND = NULL;
            prev_total_h = 0;
        }
        ReleaseSRWLockExclusive(&lock_epw);
#endif
        if (hServiceWindowThread)
            PostThreadMessageW(GetThreadId(hServiceWindowThread), WM_USER + 1, NULL, NULL);
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static BOOL explorer_SetChildWindowNoActivateHook(HWND hWnd)
{
    WCHAR className[100];
    ZeroMemory(className, 100);
    GetClassNameW(hWnd, className, 100);
    if (WStrEq(className, L"ControlCenterButton")) {
        lpShouldDisplayCCButton = (BYTE *)(GetWindowLongPtrW(hWnd, 0) + 120);
        if (*lpShouldDisplayCCButton)
            *lpShouldDisplayCCButton = !bHideControlCenterButton;
    }

    // get a look at vtable by searching for v_IsEnabled
    if (WStrEq(className, L"TrayButton")) {
        uintptr_t Instance = *(uintptr_t *)GetWindowLongPtrW(hWnd, 0);
        if (Instance) {
            uintptr_t TrayButton_GetComponentName = *(INT_PTR(WINAPI **)())(Instance + 304); // 280 in versions of Windows 10 where this method exists
            wchar_t *wszComponentName = NULL;

            if (IsWindows11() && !IsBadCodePtr(TrayButton_GetComponentName)) {
                wszComponentName = (const WCHAR *)(*(uintptr_t(**)(void))(Instance + 304))();
            } else {
                WCHAR title[MAX_PATH];
                GetWindowTextW(hWnd, title, MAX_PATH);
                WCHAR   pbtitle[MAX_PATH];
                HMODULE hPeopleBand = LoadLibraryExW(L"PeopleBand.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
                if (hPeopleBand) {
                    LoadStringW(hPeopleBand, 256, pbtitle, 260);
                    FreeLibrary(hPeopleBand);
                }
                if (WStrEq(pbtitle, title))
                    wszComponentName = L"PeopleButton";
            }

            if (wszComponentName) {
                if (WStrEq(wszComponentName, L"CortanaButton")) {
                    DWORD dwOldProtect;
                    VirtualProtect(Instance + 160, sizeof(uintptr_t), PAGE_READWRITE, &dwOldProtect);
                    if (!Widgets_OnClickFunc)
                        Widgets_OnClickFunc = *(uintptr_t *)(Instance + 160);
                    *(uintptr_t *)(Instance + 160) = Widgets_OnClickHook; // OnClick
                    VirtualProtect(Instance + 160, sizeof(uintptr_t), dwOldProtect, &dwOldProtect);
                    VirtualProtect(Instance + 216, sizeof(uintptr_t), PAGE_READWRITE, &dwOldProtect);
                    if (!Widgets_GetTooltipTextFunc)
                        Widgets_GetTooltipTextFunc = *(uintptr_t *)(Instance + 216);
                    *(uintptr_t *)(Instance + 216) = Widgets_GetTooltipTextHook; // OnTooltipShow
                    VirtualProtect(Instance + 216, sizeof(uintptr_t), dwOldProtect, &dwOldProtect);
                }
                else if (WStrEq(wszComponentName, L"MultitaskingButton")) {
                    DWORD dwOldProtect;
                    void (*old_click_handler)() = Instance + 160;
                    VirtualProtect(Instance + 160, sizeof(uintptr_t), PAGE_READWRITE, &dwOldProtect);
                    *(uintptr_t *)(Instance + 160) = ToggleTaskView; // OnClick
                    VirtualProtect(Instance + 160, sizeof(uintptr_t), dwOldProtect, &dwOldProtect);
                }
                else if (WStrEq(wszComponentName, L"PeopleButton")) {
                    DWORD dwOldProtect;
                    uintptr_t PeopleButton_Instance = *((uintptr_t *)GetWindowLongPtrW(hWnd, 0) + 17);

                    VirtualProtect(PeopleButton_Instance + 32, sizeof(uintptr_t), PAGE_READWRITE, &dwOldProtect);
                    if (!PeopleButton_CalculateMinimumSizeFunc)
                        PeopleButton_CalculateMinimumSizeFunc = *(uintptr_t *)(PeopleButton_Instance + 32);
                    *(uintptr_t *)(PeopleButton_Instance + 32) = PeopleButton_CalculateMinimumSizeHook; // CalculateMinimumSize
                    VirtualProtect(PeopleButton_Instance + 32, sizeof(uintptr_t), dwOldProtect, &dwOldProtect);

                    uintptr_t off_PeopleButton_ShowTooltip = 0;
                    if (IsWindows11())
                        off_PeopleButton_ShowTooltip = 224;
                    else
                        off_PeopleButton_ShowTooltip = 200;
                    VirtualProtect(Instance + off_PeopleButton_ShowTooltip, sizeof(uintptr_t), PAGE_READWRITE, &dwOldProtect);
                    if (!PeopleButton_ShowTooltipFunc)
                        PeopleButton_ShowTooltipFunc = *(uintptr_t *)(Instance + off_PeopleButton_ShowTooltip);
                    *(uintptr_t *)(Instance + off_PeopleButton_ShowTooltip) = PeopleButton_ShowTooltipHook; // OnTooltipShow
                    VirtualProtect(Instance + off_PeopleButton_ShowTooltip, sizeof(uintptr_t), dwOldProtect, &dwOldProtect);

                    uintptr_t off_PeopleButton_OnClick = IsWindows11() ? 160 : 136;;
                    VirtualProtect(Instance + off_PeopleButton_OnClick, sizeof(uintptr_t), PAGE_READWRITE, &dwOldProtect);
                    if (!PeopleButton_OnClickFunc)
                        PeopleButton_OnClickFunc = *(uintptr_t *)(Instance + off_PeopleButton_OnClick);
                    *(uintptr_t *)(Instance + off_PeopleButton_OnClick) = PeopleButton_OnClickHook; // OnClick
                    VirtualProtect(Instance + off_PeopleButton_OnClick, sizeof(uintptr_t), dwOldProtect, &dwOldProtect);

                    PeopleButton_LastHWND = hWnd;
                    SetWindowSubclass(hWnd, PeopleButton_SubclassProc, PeopleButton_SubclassProc, 0);

                    EnterCriticalSection(&lock_epw);
                    if (!epw) {
                        if (SUCCEEDED(CoCreateInstance(&CLSID_EPWeather, NULL, CLSCTX_LOCAL_SERVER, &IID_IEPWeather, &epw)) && epw)
                        {
                            epw->lpVtbl->SetNotifyWindow(epw, hWnd);
                            WCHAR wszBuffer[MAX_PATH] = {0};
                            HMODULE hModule = GetModuleHandleW(L"pnidui.dll");
                            if (hModule)
                                LoadStringW(hModule, 35, wszBuffer, MAX_PATH);
                            SetWindowTextW(hWnd, wszBuffer);
                        }
                    }
                    LeaveCriticalSection(&lock_epw);
                }
            }
        }
    }

    return SetChildWindowNoActivateFunc(hWnd);
}
#endif
#pragma endregion


#pragma region "Hide Show desktop button"
#ifdef _WIN64
static INT64 ShowDesktopSubclassProc(
    _In_ HWND   hWnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    UINT_PTR    uIdSubclass,
    DWORD_PTR   dwRefData
)
{
    if (uMsg == WM_NCDESTROY) {
        RemoveWindowSubclass(hWnd, ShowDesktopSubclassProc, ShowDesktopSubclassProc);
    } else if (uMsg == WM_USER + 100) {
        LRESULT lRes = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        if (lRes > 0) {
            DWORD dwVal = 0, dwSize = sizeof(DWORD);
            if (SHRegGetValueFromHKCUHKLMFunc &&
                SHRegGetValueFromHKCUHKLMFunc(
                    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                    L"TaskbarSd",
                    SRRF_RT_REG_DWORD, NULL, &dwVal, (LPDWORD)(&dwSize)
                ) == ERROR_SUCCESS &&
                !dwVal)
            {
                lRes = 0;
            } else if (dwVal) {
                PostMessageW(hWnd, 794, 0, 0);
            }
        }
        return lRes;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
#endif
#pragma endregion


#pragma region "Notify shell ready (fixes delay at logon)"
#ifdef _WIN64
static DWORD SignalShellReady(DWORD wait)
{
    printf("Started \"Signal shell ready\" thread.\n");
    // UpdateStartMenuPositioning(MAKELPARAM(TRUE, TRUE));

    while (!wait && TRUE) {
        HWND hShell_TrayWnd = FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL);
        if (hShell_TrayWnd) {
            HWND hWnd = FindWindowExW(hShell_TrayWnd, NULL, L"Start", NULL);
            if (hWnd) {
                if (IsWindowVisible(hWnd)) {
                    UpdateStartMenuPositioning(MAKELPARAM(TRUE, TRUE));
                    break;
                }
            }
        }
        Sleep(100);
    }

    if (!wait)
        Sleep(600);
    else
        Sleep(wait);

    HANDLE hEvent = CreateEventW(NULL, false, false, L"ShellDesktopSwitchEvent");
    if (hEvent) {
        wprintf(L">>> Signal shell ready.\n");
        SetEvent(hEvent);
    }
    SetEvent(hCanStartSws);
    if (bOldTaskbar && (global_rovi.dwBuildNumber >= 22567))
        PatchSndvolsso();

    wprintf(L"Ended \"Signal shell ready\" thread.\n");
    return 0;
}
#endif
#pragma endregion


#pragma region "Utilities"

static LSTATUS myRegQueryValueEx(HKEY hKey, LPCWSTR lpValueName, LPBYTE lpData)
{
    DWORD dwSize = sizeof(DWORD);
    return RegQueryValueExW(hKey, lpValueName, NULL, NULL, lpData, &dwSize);
}

static void ensureRegistryKey(wchar_t const *keyName)
{
    HKEY hKey = NULL;
    RegCreateKeyExW(HKEY_CURRENT_USER, keyName, 0, NULL, REG_OPTION_NON_VOLATILE,
                    KEY_READ | KEY_WOW64_64KEY, NULL, &hKey, NULL);
    if (hKey != NULL && hKey != INVALID_HANDLE_VALUE)
        RegCloseKey(hKey);
}

#pragma endregion


#pragma region "Window Switcher"
#ifdef _WIN64

static void sws_ReadSettings(sws_WindowSwitcher *sws)
{
    HKEY  hKey   = NULL;
    DWORD dwSize = 0;

    RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, NULL);
    if (hKey == INVALID_HANDLE_VALUE)
        hKey = NULL;
    if (hKey) {
        DWORD val = 0;
        dwSize    = sizeof(DWORD);
        RegQueryValueExW(hKey, L"AltTabSettings", 0, NULL, &val, &dwSize);
        sws_IsEnabled = val == 2;
        RegCloseKey(hKey);
    }

    RegCreateKeyExW(HKEY_CURRENT_USER, REGPATH L"\\sws",
                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, NULL);
    if (hKey == INVALID_HANDLE_VALUE)
        hKey = NULL;
    if (hKey) {
        if (sws) {
            sws_WindowSwitcher_InitializeDefaultSettings(sws);
            sws->dwWallpaperSupport = SWS_WALLPAPERSUPPORT_EXPLORER;

            myRegQueryValueEx(hKey, L"IncludeWallpaper", &sws->bIncludeWallpaper);
            myRegQueryValueEx(hKey, L"RowHeight", &sws->dwRowHeight);
            myRegQueryValueEx(hKey, L"MaxWidth", &sws->dwMaxWP);
            myRegQueryValueEx(hKey, L"MaxHeight", &sws->dwMaxHP);
            myRegQueryValueEx(hKey, L"ColorScheme", &sws->dwColorScheme);
            myRegQueryValueEx(hKey, L"Theme", &sws->dwTheme);
            myRegQueryValueEx(hKey, L"CornerPreference", &sws->dwCornerPreference);
            myRegQueryValueEx(hKey, L"ShowDelay", &sws->dwShowDelay);
            myRegQueryValueEx(hKey, L"PrimaryOnly", &sws->bPrimaryOnly);
            myRegQueryValueEx(hKey, L"PerMonitor", &sws->bPerMonitor);
            myRegQueryValueEx(hKey, L"MaxWidthAbs", &sws->dwMaxAbsoluteWP);
            myRegQueryValueEx(hKey, L"MaxHeightAbs", &sws->dwMaxAbsoluteHP);
            myRegQueryValueEx(hKey, L"NoPerApplicationList", &sws->bNoPerApplicationList);
            myRegQueryValueEx(hKey, L"MasterPadding", &sws->dwMasterPadding);
            myRegQueryValueEx(hKey, L"SwitcherIsPerApplication", &sws->bSwitcherIsPerApplication);
            myRegQueryValueEx(hKey, L"AlwaysUseWindowTitleAndIcon", &sws->bAlwaysUseWindowTitleAndIcon);
            myRegQueryValueEx(hKey, L"ScrollWheelBehavior", &sws->dwScrollWheelBehavior);
            myRegQueryValueEx(hKey, L"ScrollWheelInvert", &sws->bScrollWheelInvert);

            if (sws->bIsInitialized) {
                sws_WindowSwitcher_UnregisterHotkeys(sws);
                sws_WindowSwitcher_RegisterHotkeys(sws, NULL);
                sws_WindowSwitcher_RefreshTheme(sws);
            }
        }
        RegCloseKey(hKey);
    }
}

static DWORD WindowSwitcher(DWORD unused)
{
    if (IsWindows11())
        WaitForSingleObject(hCanStartSws, INFINITE);
    if (!bOldTaskbar)
        WaitForSingleObject(hWin11AltTabInitialized, INFINITE);
    Sleep(1000);

    while (TRUE) {
        // Sleep(5000);
        while (!FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL)) {
            printf("[sws] Waiting for taskbar...\n");
            Sleep(100);
        }
        Sleep(100);
        sws_ReadSettings(NULL);
        if (sws_IsEnabled) {
            sws_error_t         err;
            sws_WindowSwitcher *sws = calloc(1, sizeof(sws_WindowSwitcher));
            if (!sws)
                return 0;
            sws_ReadSettings(sws);
            err = sws_error_Report(sws_error_GetFromInternalError(sws_WindowSwitcher_Initialize(&sws, FALSE)), NULL);
            if (err == SWS_ERROR_SUCCESS) {
                sws_WindowSwitcher_RefreshTheme(sws);
                HANDLE hEvents[3];
                hEvents[0] = sws->hEvExit;
                hEvents[1] = hSwsSettingsChanged;
                hEvents[2] = hSwsOpacityMaybeChanged;
                while (TRUE) {
                    DWORD dwRes = MsgWaitForMultipleObjectsEx(3, hEvents, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                    if (dwRes == WAIT_OBJECT_0 + 0)
                        break;
                    if (dwRes == WAIT_OBJECT_0 + 1) {
                        sws_ReadSettings(sws);
                        if (!sws_IsEnabled)
                            break;
                    } else if (dwRes == WAIT_OBJECT_0 + 2) {
                        sws_WindowSwitcher_RefreshTheme(sws);
                    } else if (dwRes == WAIT_OBJECT_0 + 3) {
                        MSG msg;
                        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                    } else {
                        break;
                    }
                }
                sws_WindowSwitcher_Clear(sws);
                free(sws);
            } else {
                free(sws);
                return 0;
            }
        } else {
            WaitForSingleObject(hSwsSettingsChanged, INFINITE);
        }
    }
}
#endif
#pragma endregion


#pragma region "Load Settings from registry"

#define REFRESHUI_NONE      0b000000
#define REFRESHUI_GLOM      0b000001
#define REFRESHUI_ORB       0b000010
#define REFRESHUI_PEOPLE    0b000100
#define REFRESHUI_TASKBAR   0b001000
#define REFRESHUI_CENTER    0b010000
#define REFRESHUI_SPOTLIGHT 0b100000

static void WINAPI LoadSettings(LPARAM lParam)
{
    HKEY  hKey              = NULL;
    BOOL  bIsExplorer       = LOWORD(lParam);
    BOOL  bIsRefreshAllowed = HIWORD(lParam);
    DWORD dwRefreshUIMask   = REFRESHUI_NONE;
    DWORD dwSize;
    DWORD dwTemp;

    RegCreateKeyExW(HKEY_CURRENT_USER, L"" REGPATH, 0, NULL, REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &hKey, NULL);
    if (hKey == INVALID_HANDLE_VALUE)
        hKey = NULL;

    if (hKey) {
#ifdef _WIN64
        dwTemp = 0;
        myRegQueryValueEx(hKey, L"MigratedFromOldSettings", &dwTemp);
        if (!dwTemp) {
            HKEY hOldKey = NULL;
            RegOpenKeyExW(HKEY_CURRENT_USER, L"" REGPATH_OLD, REG_OPTION_NON_VOLATILE,
                          KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hOldKey);

            if (hOldKey == INVALID_HANDLE_VALUE)
                hOldKey = NULL;
            if (hOldKey) {
                DWORD dw1 = 0;
                DWORD dw2 = 0;
                myRegQueryValueEx(hKey, L"OpenPropertiesAtNextStart", &dw1);
                myRegQueryValueEx(hKey, L"IsUpdatePending", &dw2);

                if (RegCopyTreeW(hOldKey, NULL, hKey) == ERROR_SUCCESS)
                {
                    RegSetValueExW(hKey, L"OpenPropertiesAtNextStart", 0, REG_DWORD, &dw1, sizeof(DWORD));
                    RegSetValueExW(hKey, L"IsUpdatePending", 0, REG_DWORD, &dw2, sizeof(DWORD));
                    RegDeleteKeyExW(hKey, L"" STARTDOCKED_SB_NAME, KEY_WOW64_64KEY, 0);
                    DWORD dwTaskbarGlomLevel = 0, dwMMTaskbarGlomLevel = 0;
                    dwSize = sizeof(DWORD);
                    RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                                 L"TaskbarGlomLevel", REG_DWORD, NULL, &dwTaskbarGlomLevel, &dwSize);
                    RegSetValueExW(hKey, L"TaskbarGlomLevel", 0, REG_DWORD, &dwTaskbarGlomLevel, sizeof(DWORD));
                    dwSize = sizeof(DWORD);
                    RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                                 L"MMTaskbarGlomLevel", REG_DWORD, NULL, &dwMMTaskbarGlomLevel, &dwSize);
                    RegSetValueExW(hKey, L"MMTaskbarGlomLevel", 0, REG_DWORD, &dwMMTaskbarGlomLevel, sizeof(DWORD));
                }
            }

            dwTemp = TRUE;
            RegSetValueExW(hKey, L"MigratedFromOldSettings", 0, REG_DWORD, &dwTemp, sizeof(DWORD));
        }
#endif
        myRegQueryValueEx(hKey, L"AllocConsole", &bAllocConsole);
        dwTemp = 0;
        myRegQueryValueEx(hKey, L"Memcheck", &dwTemp);
        if (dwTemp) {
#if defined(DEBUG) || defined(_DEBUG)
            wprintf(L"[Memcheck] Dumping memory leaks...\n");
            _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
            _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
            _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
            _CrtDumpMemoryLeaks();
            HANDLE process = GetCurrentProcess();
            wprintf(L"[Memcheck] Memory leak dump complete.\n"
                    L"[Memcheck] Objects in use:\nGDI\tGDIp\tUSER\tUSERp\n%lu\t%lu\t%lu\t%lu\n",
                    GetGuiResources(process, GR_GDIOBJECTS),
                    GetGuiResources(process, GR_GDIOBJECTS_PEAK),
                    GetGuiResources(process, GR_USEROBJECTS),
                    GetGuiResources(process, GR_USEROBJECTS_PEAK));
#endif
            dwTemp = 0;
            RegSetValueExW(hKey, L"Memcheck", 0, REG_DWORD, &dwTemp, sizeof(DWORD));
        }

        dwTemp = 0;
        myRegQueryValueEx(hKey, L"OldTaskbarAl", &dwTemp);
        if (dwTemp != dwOldTaskbarAl) {
            dwOldTaskbarAl = dwTemp;
            dwRefreshUIMask |= REFRESHUI_CENTER;
        }

        dwTemp = 0;
        myRegQueryValueEx(hKey, L"MMOldTaskbarAl", &dwTemp);
        if (dwTemp != dwMMOldTaskbarAl) {
            dwMMOldTaskbarAl = dwTemp;
            dwRefreshUIMask |= REFRESHUI_CENTER;
        }

        myRegQueryValueEx(hKey, L"HideExplorerSearchBar", &bHideExplorerSearchBar);
        myRegQueryValueEx(hKey, L"ShrinkExplorerAddressBar", &bShrinkExplorerAddressBar);
        myRegQueryValueEx(hKey, L"UseClassicDriveGrouping", &bUseClassicDriveGrouping);
        myRegQueryValueEx(hKey, L"FileExplorerCommandUI", &dwFileExplorerCommandUI);
        if (dwFileExplorerCommandUI == 9999) {
            if (IsWindows11()) {
                DWORD bIsWindows11CommandBarDisabled = RegGetValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\CLSID\\{d93ed569-3b3e-4bff-8355-3c44f6a52bb5}\\InProcServer32",
                                                                    L"", RRF_RT_REG_SZ, NULL, NULL, NULL) == ERROR_SUCCESS;
                RegSetValueExW(hKey, L"FileExplorerCommandUI", 0, REG_DWORD, &bIsWindows11CommandBarDisabled, sizeof(DWORD));
                dwFileExplorerCommandUI = bIsWindows11CommandBarDisabled;
            } else {
                dwFileExplorerCommandUI = 0;
            }
        }

        myRegQueryValueEx(hKey, L"LegacyFileTransferDialog", &bLegacyFileTransferDialog);
        myRegQueryValueEx(hKey, L"DisableImmersiveContextMenu", &bDisableImmersiveContextMenu);
        dwTemp = FALSE;
        myRegQueryValueEx(hKey, L"ClassicThemeMitigations", &dwTemp);
        if (!bWasClassicThemeMitigationsSet) {
            bClassicThemeMitigations       = dwTemp;
            bWasClassicThemeMitigationsSet = TRUE;
        }

        myRegQueryValueEx(hKey, L"SkinMenus", &bSkinMenus);
        if (bIsExplorerProcess) {
            if (bAllocConsole)
                ExplorerPatcher_OpenConsoleWindow();
            else
                ExplorerPatcher_CloseConsoleWindow();
        }

        myRegQueryValueEx(hKey, L"DoNotRedirectSystemToSettingsApp", &bDoNotRedirectSystemToSettingsApp);
        myRegQueryValueEx(hKey, L"DoNotRedirectProgramsAndFeaturesToSettingsApp", &bDoNotRedirectProgramsAndFeaturesToSettingsApp);
        dwTemp = NULL;
        myRegQueryValueEx(hKey, L"MicaEffectOnTitlebar", &dwTemp);
        if (dwTemp != bMicaEffectOnTitlebar) {
            bMicaEffectOnTitlebar = dwTemp;
            HMODULE hUxtheme      = GetModuleHandleW(L"uxtheme.dll");
            if (hUxtheme) {
                if (bMicaEffectOnTitlebar) {
                    VnPatchDelayIAT(hUxtheme, "dwmapi.dll", "DwmExtendFrameIntoClientArea", uxtheme_DwmExtendFrameIntoClientAreaHook);
                } else {
                    // VnPatchDelayIAT(hUxtheme, "dwmapi.dll", "DwmExtendFrameIntoClientArea", DwmExtendFrameIntoClientArea);
                }
            }
        }

        myRegQueryValueEx(hKey, L"HideIconAndTitleInExplorer", &bHideIconAndTitleInExplorer);
        if (!bIsExplorer) {
            RegCloseKey(hKey);
            return;
        }
        dwTemp = TRUE;
        myRegQueryValueEx(hKey, L"OldTaskbar", &dwTemp);
        if (!bWasOldTaskbarSet) {
            bOldTaskbar       = dwTemp;
            bWasOldTaskbarSet = TRUE;
        }

        myRegQueryValueEx(hKey, L"HideControlCenterButton", &bHideControlCenterButton);
        myRegQueryValueEx(hKey, L"FlyoutMenus", &bFlyoutMenus);
        myRegQueryValueEx(hKey, L"CenterMenus", &bCenterMenus);
        myRegQueryValueEx(hKey, L"SkinIcons", &bSkinIcons);
        myRegQueryValueEx(hKey, L"ReplaceNetwork", &bReplaceNetwork);
        myRegQueryValueEx(hKey, L"ExplorerReadyDelay", &dwExplorerReadyDelay);
        myRegQueryValueEx(hKey, L"ArchiveMenu", &bEnableArchivePlugin);
        myRegQueryValueEx(hKey, L"ClockFlyoutOnWinC", &bClockFlyoutOnWinC);
        myRegQueryValueEx(hKey, L"DisableImmersiveContextMenu", &bDisableImmersiveContextMenu);
        myRegQueryValueEx(hKey, L"HookStartMenu", &bHookStartMenu);
        myRegQueryValueEx(hKey, L"PropertiesInWinX", &bPropertiesInWinX);
        myRegQueryValueEx(hKey, L"NoPropertiesInContextMenu", &bNoPropertiesInContextMenu);
        myRegQueryValueEx(hKey, L"NoMenuAccelerator", &bNoMenuAccelerator);
        myRegQueryValueEx(hKey, L"IMEStyle", &dwIMEStyle);

        if (IsWindows11Version22H2OrHigher()) {
            if (!dwIMEStyle)
                dwIMEStyle = 7;
            else if (dwIMEStyle == 7)
                dwIMEStyle = 0;
        }

        myRegQueryValueEx(hKey, L"UpdatePolicy", &dwUpdatePolicy);
        myRegQueryValueEx(hKey, L"IsUpdatePending", &bShowUpdateToast);
        myRegQueryValueEx(hKey, L"ToolbarSeparators", &bToolbarSeparators);
        myRegQueryValueEx(hKey, L"TaskbarAutohideOnDoubleClick", &bTaskbarAutohideOnDoubleClick);
        dwTemp = ORB_STYLE_WINDOWS10;
        myRegQueryValueEx(hKey, L"OrbStyle", &dwTemp);
        if (bOldTaskbar && (dwTemp != dwOrbStyle)) {
            dwOrbStyle = dwTemp;
            dwRefreshUIMask |= REFRESHUI_ORB;
        }

        myRegQueryValueEx(hKey, L"EnableSymbolDownload", &bEnableSymbolDownload);
        dwTemp = NULL;
        myRegQueryValueEx(hKey, L"OpenPropertiesAtNextStart", &dwTemp);
        if (!IsAppRunningAsAdminMode() && dwTemp) {
#ifdef _WIN64
            LaunchPropertiesGUI(hModule);
#endif
        }

        myRegQueryValueEx(hKey, L"DisableAeroSnapQuadrants", &bDisableAeroSnapQuadrants);
        myRegQueryValueEx(hKey, L"SnapAssistSettings", &dwSnapAssistSettings);
        myRegQueryValueEx(hKey, L"DoNotRedirectDateAndTimeToSettingsApp", &bDoNotRedirectDateAndTimeToSettingsApp);
        myRegQueryValueEx(hKey, L"DoNotRedirectNotificationIconsToSettingsApp", &bDoNotRedirectNotificationIconsToSettingsApp);
        myRegQueryValueEx(hKey, L"DisableOfficeHotkeys", &bDisableOfficeHotkeys);
        myRegQueryValueEx(hKey, L"DisableWinFHotkey", &bDisableWinFHotkey);

        dwTemp = FALSE;
        myRegQueryValueEx(hKey, L"SpotlightDisableIcon", &dwTemp);
        if (dwTemp != bDisableSpotlightIcon) {
            bDisableSpotlightIcon = dwTemp;
#ifdef _WIN64
            if (IsSpotlightEnabled())
                dwRefreshUIMask |= REFRESHUI_SPOTLIGHT;
#endif
        }
        myRegQueryValueEx(hKey, L"SpotlightDesktopMenuMask", &dwSpotlightDesktopMenuMask);
        dwTemp = NULL;
        myRegQueryValueEx(hKey, L"SpotlightUpdateSchedule", &dwTemp);
        if (dwTemp != dwSpotlightUpdateSchedule) {
            dwSpotlightUpdateSchedule = dwTemp;
#ifdef _WIN64
            if (IsSpotlightEnabled() && hWndServiceWindow) {
                if (dwSpotlightUpdateSchedule)
                    SetTimer(hWndServiceWindow, 100, dwSpotlightUpdateSchedule * 1000, NULL);
                else
                    KillTimer(hWndServiceWindow, 100);
            }
#endif
        }

        dwTemp = FALSE;
        myRegQueryValueEx(hKey, L"PinnedItemsActAsQuickLaunch", &dwTemp);
        if (!bWasPinnedItemsActAsQuickLaunch) {
            //if (dwTemp != bPinnedItemsActAsQuickLaunch)
            {
                bPinnedItemsActAsQuickLaunch    = dwTemp;
                bWasPinnedItemsActAsQuickLaunch = TRUE;
                //dwRefreshUIMask |= REFRESHUI_TASKBAR;
            }
        }
        dwTemp = FALSE;
        myRegQueryValueEx(hKey, L"RemoveExtraGapAroundPinnedItems", &dwTemp);
        //if (!bWasRemoveExtraGapAroundPinnedItems)
        {
            if (dwTemp != bRemoveExtraGapAroundPinnedItems) {
                bRemoveExtraGapAroundPinnedItems    = dwTemp;
                bWasRemoveExtraGapAroundPinnedItems = TRUE;
                dwRefreshUIMask |= REFRESHUI_TASKBAR;
            }
        }
        myRegQueryValueEx(hKey, L"UndeadStartCorner", &dwUndeadStartCorner);

#ifdef _WIN64
        EnterCriticalSection(&lock_epw);

        DWORD dwOldWeatherTemperatureUnit = dwWeatherTemperatureUnit;
        myRegQueryValueEx(hKey, L"WeatherTemperatureUnit", &dwWeatherTemperatureUnit);
        if (dwWeatherTemperatureUnit != dwOldWeatherTemperatureUnit && epw) {
            epw->lpVtbl->SetTemperatureUnit(epw, dwWeatherTemperatureUnit);
            HWND hWnd = NULL;
            if (SUCCEEDED(epw->lpVtbl->GetWindowHandle(epw, &hWnd)) && hWnd)
                SendMessageW(hWnd, EP_WEATHER_WM_FETCH_DATA, 0, 0);
        }

        DWORD dwOldWeatherViewMode = dwWeatherViewMode;
        myRegQueryValueEx(hKey, L"WeatherViewMode", &dwWeatherViewMode);
        if (dwWeatherViewMode != dwOldWeatherViewMode && PeopleButton_LastHWND)
            dwRefreshUIMask |= REFRESHUI_PEOPLE;

        DWORD dwOldUpdateSchedule = dwWeatherUpdateSchedule;
        myRegQueryValueEx(hKey, L"WeatherContentUpdateMode", &dwWeatherUpdateSchedule);
        if (dwWeatherUpdateSchedule != dwOldUpdateSchedule && epw)
            epw->lpVtbl->SetUpdateSchedule(epw, dwWeatherUpdateSchedule * 1000);

        dwSize = MAX_PATH * sizeof(WCHAR);
        if (RegQueryValueExW(hKey, L"WeatherLocation", NULL, NULL, wszWeatherTerm, &dwSize))
            wcscpy_s(wszWeatherTerm, MAX_PATH, L"");
        else if (wszWeatherTerm[0] == 0)
            wcscpy_s(wszWeatherTerm, MAX_PATH, L"");
        if (epw)
            epw->lpVtbl->SetTerm(epw, MAX_PATH * sizeof(WCHAR), wszWeatherTerm);

        dwSize = MAX_PATH * sizeof(WCHAR);
        if (RegQueryValueExW(hKey, L"WeatherLanguage", NULL, NULL, wszWeatherLanguage, &dwSize)) {
            BOOL    bOk                = FALSE;
            ULONG   ulNumLanguages     = 0;
            LPCWSTR wszLanguagesBuffer = NULL;
            ULONG   cchLanguagesBuffer = 0;
            if (GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &ulNumLanguages, NULL, &cchLanguagesBuffer)) {
                if (wszLanguagesBuffer = malloc(cchLanguagesBuffer * sizeof(WCHAR))) {
                    if (GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &ulNumLanguages, wszLanguagesBuffer, &cchLanguagesBuffer)) {
                        wcscpy_s(wszWeatherLanguage, MAX_PATH, wszLanguagesBuffer);
                        bOk = TRUE;
                    }
                    free(wszLanguagesBuffer);
                }
            }
            if (!bOk)
                wcscpy_s(wszWeatherLanguage, MAX_PATH, L"en-US");
        } else if (wszWeatherLanguage[0] == 0) {
            BOOL    bOk                = FALSE;
            ULONG   ulNumLanguages     = 0;
            LPCWSTR wszLanguagesBuffer = NULL;
            ULONG   cchLanguagesBuffer = 0;
            if (GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &ulNumLanguages, NULL, &cchLanguagesBuffer)) {
                if (wszLanguagesBuffer = malloc(cchLanguagesBuffer * sizeof(WCHAR))) {
                    if (GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &ulNumLanguages, wszLanguagesBuffer, &cchLanguagesBuffer)) {
                        wcscpy_s(wszWeatherLanguage, MAX_PATH, wszLanguagesBuffer);
                        bOk = TRUE;
                    }
                    free(wszLanguagesBuffer);
                }
            }
            if (!bOk)
                wcscpy_s(wszWeatherLanguage, MAX_PATH, L"en-US");
        }
        if (epw)
            epw->lpVtbl->SetLanguage(epw, MAX_PATH * sizeof(WCHAR), wszWeatherLanguage);

        DWORD bOldWeatherFixedSize = bWeatherFixedSize;
        myRegQueryValueEx(hKey, L"WeatherFixedSize", &bWeatherFixedSize);
        if (bWeatherFixedSize != bOldWeatherFixedSize && epw)
            dwRefreshUIMask |= REFRESHUI_PEOPLE;

        DWORD dwOldWeatherTheme = dwWeatherTheme;
        myRegQueryValueEx(hKey, L"WeatherTheme", &dwWeatherTheme);
        if (dwWeatherTheme != dwOldWeatherTheme && PeopleButton_LastHWND && epw)
            epw->lpVtbl->SetDarkMode(epw, (LONG64)dwWeatherTheme, TRUE);

        DWORD dwOldWeatherGeolocationMode = dwWeatherGeolocationMode;
        myRegQueryValueEx(hKey, L"WeatherLocationType", &dwWeatherGeolocationMode);
        if (dwWeatherGeolocationMode != dwOldWeatherGeolocationMode && PeopleButton_LastHWND && epw)
            epw->lpVtbl->SetGeolocationMode(epw, (LONG64)dwWeatherGeolocationMode);

        DWORD dwOldWeatherWindowCornerPreference = dwWeatherWindowCornerPreference;
        myRegQueryValueEx(hKey, L"WeatherWindowCornerPreference", &dwWeatherWindowCornerPreference);
        if (dwWeatherWindowCornerPreference != dwOldWeatherWindowCornerPreference && PeopleButton_LastHWND && epw)
            epw->lpVtbl->SetWindowCornerPreference(epw, (LONG64)dwWeatherWindowCornerPreference);

        DWORD dwOldWeatherDevMode = dwWeatherDevMode;
        myRegQueryValueEx(hKey, L"WeatherDevMode", &dwWeatherDevMode);
        if (dwWeatherDevMode != dwOldWeatherDevMode && PeopleButton_LastHWND && epw)
            epw->lpVtbl->SetDevMode(epw, (LONG64)dwWeatherDevMode, TRUE);

        DWORD dwOldWeatherIconPack = dwWeatherIconPack;
        myRegQueryValueEx(hKey, L"WeatherIconPack", &dwWeatherIconPack);
        if (dwWeatherIconPack != dwOldWeatherIconPack && PeopleButton_LastHWND && epw)
            epw->lpVtbl->SetIconPack(epw, (LONG64)dwWeatherIconPack, TRUE);

        DWORD dwOldWeatherToLeft = dwWeatherToLeft;
        myRegQueryValueEx(hKey, L"WeatherToLeft", &dwWeatherToLeft);
        if (dwWeatherToLeft != dwOldWeatherToLeft && PeopleButton_LastHWND)
            dwRefreshUIMask |= REFRESHUI_CENTER;

        DWORD dwOldWeatherContentsMode = dwWeatherContentsMode;
        myRegQueryValueEx(hKey, L"WeatherContentsMode", &dwWeatherContentsMode);
        if (dwWeatherContentsMode != dwOldWeatherContentsMode && PeopleButton_LastHWND)
            dwRefreshUIMask |= REFRESHUI_CENTER;

        DWORD dwOldWeatherZoomFactor = dwWeatherZoomFactor;
        myRegQueryValueEx(hKey, L"WeatherZoomFactor", &dwWeatherZoomFactor);
        if (dwWeatherZoomFactor != dwOldWeatherZoomFactor && PeopleButton_LastHWND && epw)
            epw->lpVtbl->SetZoomFactor(epw, dwWeatherZoomFactor ? (LONG64)dwWeatherZoomFactor : 100);

        LeaveCriticalSection(&lock_epw);
#endif

        dwTemp = TASKBARGLOMLEVEL_DEFAULT;
        myRegQueryValueEx(hKey, L"TaskbarGlomLevel", &dwTemp);
        if (bOldTaskbar && (dwTemp != dwTaskbarGlomLevel)) {
            dwRefreshUIMask = REFRESHUI_GLOM;
            if (dwOldTaskbarAl)
                dwRefreshUIMask |= REFRESHUI_CENTER;
        }

        dwTaskbarGlomLevel = dwTemp;
        dwTemp             = MMTASKBARGLOMLEVEL_DEFAULT;
        myRegQueryValueEx(hKey, L"MMTaskbarGlomLevel", &dwTemp);
        if (bOldTaskbar && (dwTemp != dwMMTaskbarGlomLevel)) {
            dwRefreshUIMask = REFRESHUI_GLOM;
            if (dwMMOldTaskbarAl)
                dwRefreshUIMask |= REFRESHUI_CENTER;
        }
        dwMMTaskbarGlomLevel = dwTemp;

        RegCloseKey(hKey);
    }

    ensureRegistryKey(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer");
    ensureRegistryKey(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartPage");
    ensureRegistryKey(L"\\sws");
    ensureRegistryKey(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MultitaskingView\\AltTabViewHost");
    ensureRegistryKey(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
    ensureRegistryKey(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search");
    ensureRegistryKey(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\People");
    ensureRegistryKey(L"SOFTWARE\\Microsoft\\TabletTip\\1.7");

    if (bIsRefreshAllowed && dwRefreshUIMask) {
        if (dwRefreshUIMask & REFRESHUI_GLOM)
            Explorer_RefreshUI(0);
        if ((dwRefreshUIMask & REFRESHUI_ORB) || (dwRefreshUIMask & REFRESHUI_PEOPLE)) {
            SendNotifyMessageW(HWND_BROADCAST, WM_WININICHANGE, 0, (LPARAM)L"TraySettings");
            if (dwRefreshUIMask & REFRESHUI_ORB)
                InvalidateRect(FindWindowW(L"ExplorerPatcher_GUI_" EP_CLSID, NULL), NULL, FALSE);
            if (dwRefreshUIMask & REFRESHUI_PEOPLE) {
#if 0
                if (epw_dummytext[0] == 0)
                    epw_dummytext = L"\u2009";
                else
                    epw_dummytext = L"";
#endif
#ifdef _WIN64
                InvalidateRect(PeopleButton_LastHWND, NULL, TRUE);
#endif
            }
        }
        if (dwRefreshUIMask & REFRESHUI_TASKBAR) {
            // HACK this is mostly a hack...
#if 0
            DWORD dwGlomLevel = 2, dwSize = sizeof(DWORD), dwNewGlomLevel;
            RegGetValueW(HKEY_CURRENT_USER, IsWindows11() ? TEXT(REGPATH) :
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarGlomLevel",
            RRF_RT_DWORD, NULL, &dwGlomLevel, &dwSize); Sleep(100); dwNewGlomLevel = 0;
            RegSetKeyValueW(HKEY_CURRENT_USER, IsWindows11() ? TEXT(REGPATH) :
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarGlomLevel",
            REG_DWORD, &dwNewGlomLevel, sizeof(DWORD)); Explorer_RefreshUI(0); Sleep(100); dwNewGlomLevel = 2;
            RegSetKeyValueW(HKEY_CURRENT_USER, IsWindows11() ? TEXT(REGPATH) :
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarGlomLevel",
            REG_DWORD, &dwNewGlomLevel, sizeof(DWORD)); Explorer_RefreshUI(0); Sleep(100);
            RegSetKeyValueW(HKEY_CURRENT_USER, IsWindows11() ? TEXT(REGPATH) :
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarGlomLevel",
            REG_DWORD, &dwGlomLevel, sizeof(DWORD)); Explorer_RefreshUI(0);
#endif
        }
        if (dwRefreshUIMask & REFRESHUI_CENTER) {
#ifdef _WIN64
# if 0
            ToggleTaskbarAutohide();
            Sleep(1000);
            ToggleTaskbarAutohide();
# endif
            FixUpCenteredTaskbar();
#endif
        }
        if (dwRefreshUIMask & REFRESHUI_SPOTLIGHT) {
            DWORD dwAttributes = 0;
            dwTemp             = sizeof(DWORD);
            RegGetValueW(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\{2cc5ca98-6485-489a-920e-b3e88a6ccce3}\\ShellFolder",
                         L"Attributes", RRF_RT_DWORD, NULL, &dwAttributes, &dwTemp);
            if (bDisableSpotlightIcon)
                dwAttributes |= SFGAO_NONENUMERATED;
            else
                dwAttributes &= ~SFGAO_NONENUMERATED;
            RegSetKeyValueW(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\{2cc5ca98-6485-489a-920e-b3e88a6ccce3}\\ShellFolder",
                            L"Attributes", REG_DWORD, &dwAttributes, sizeof(DWORD));
            SHFlushSFCache();
            SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
        }
    }
}

static void Explorer_RefreshClockHelper(HWND hClockButton)
{
    INT64 *ClockButtonInstance = (INT64 *)(GetWindowLongPtrW(hClockButton, 0)); // -> ClockButton
    // we call v_Initialize because all it does is to query the
    // registry and update the internal state to display seconds or not
    // to get the offset, simply inspect the vtable of ClockButton
    if (ClockButtonInstance) {
        ((void (*)(void *))(
            *(INT64 *)(
                *ClockButtonInstance + 6 * sizeof(uintptr_t)))
        )(ClockButtonInstance); // v_Initialize

        // we need to refresh the button; for the text to actually change, we need to set this:
        // inspect ClockButton::v_OnTimer
        *((BYTE *)ClockButtonInstance + 547) = 1;
        // then, we simply invalidate the area
        InvalidateRect(hClockButton, NULL, TRUE);
    }
}

static void Explorer_RefreshClock(int unused)
{
    HWND hShellTray_Wnd = FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL);
    if (hShellTray_Wnd) {
        HWND hTrayNotifyWnd = FindWindowExW(hShellTray_Wnd, NULL, L"TrayNotifyWnd", NULL);
        if (hTrayNotifyWnd) {
            HWND hClockButton = FindWindowExW(hTrayNotifyWnd, NULL, L"TrayClockWClass", NULL);
            if (hClockButton)
                Explorer_RefreshClockHelper(hClockButton);
        }
    }

    HWND hWnd = NULL;
    do {
        hWnd = FindWindowExW(NULL, hWnd, L"Shell_SecondaryTrayWnd", NULL);
        if (hWnd) {
            HWND hClockButton = FindWindowExW(hWnd, NULL, L"ClockButton", NULL);
            if (hClockButton)
                Explorer_RefreshClockHelper(hClockButton);
        }
    } while (hWnd);
}

static void WINAPI Explorer_RefreshUI(int src)
{
    HKEY  hKey   = NULL;
    DWORD dwSize = 0, dwTemp = 0, dwRefreshMask = 0;
    if (src == 99 || src == 1) {
        RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WOW64_64KEY, NULL, &hKey, NULL);
        if (hKey == INVALID_HANDLE_VALUE)
            hKey = NULL;
        if (hKey) {
            myRegQueryValueEx(hKey, L"TaskbarSmallIcons", &dwTaskbarSmallIcons);
            dwTemp = 0;
            myRegQueryValueEx(hKey, L"ShowTaskViewButton", &dwTemp);
            if (dwTemp != dwShowTaskViewButton) {
                dwShowTaskViewButton = dwTemp;
                dwRefreshMask |= REFRESHUI_CENTER;
            }
            dwTemp = 0;
            myRegQueryValueEx(hKey, L"TaskbarDa", &dwTemp);
            if (dwTemp != dwTaskbarDa) {
                dwTaskbarDa = dwTemp;
                dwRefreshMask |= REFRESHUI_CENTER;
            }
            RegCloseKey(hKey);
            // SearchboxTaskbarMode
        }
    }
    if (src == 99 || src == 2) {
        RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search",
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WOW64_64KEY, NULL, &hKey, NULL);
        if (hKey == INVALID_HANDLE_VALUE)
            hKey = NULL;
        if (hKey) {
            dwTemp = 0;
            myRegQueryValueEx(hKey, L"SearchboxTaskbarMode", &dwTemp);
            if (dwTemp != dwSearchboxTaskbarMode) {
                dwSearchboxTaskbarMode = dwTemp;
                dwRefreshMask |= REFRESHUI_CENTER;
            }
        }
    }
    if (src == 99)
        return;
    SendNotifyMessageW(HWND_BROADCAST, WM_WININICHANGE, 0, (LPARAM)L"TraySettings");
    Explorer_RefreshClock(0);
    if (dwRefreshMask & REFRESHUI_CENTER) {
#ifdef _WIN64
        FixUpCenteredTaskbar();
#endif
    }
}

#if 0
/*UNUSED*/
static void Explorer_TogglePeopleButton(int unused)
{
    static const unsigned TRAYUI_OFFSET_IN_CTRAY = 110;

    HWND hShellTray_Wnd = FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL);
    if (hShellTray_Wnd) {
        INT64 *CTrayInstance = (INT64 *)(GetWindowLongPtrW(hShellTray_Wnd, 0)); // -> CTray
        if (CTrayInstance) {
            INT64 *TrayUIInstance = *((INT64 *)CTrayInstance + TRAYUI_OFFSET_IN_CTRAY);
            if (TrayUIInstance)
                ((void (*)(void *))(*(INT64 *)((*(INT64 *)TrayUIInstance) + 57 * sizeof(uintptr_t))))(TrayUIInstance);
        }
    }
}

/*UNUSED*/
static void Explorer_ToggleTouchpad(int unused)
{
    HWND hShellTray_Wnd = FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL);
    if (hShellTray_Wnd) {
        INT64 *CTrayInstance = (BYTE *)(GetWindowLongPtrW(hShellTray_Wnd, 0)); // -> CTray
        if (CTrayInstance) {
            const unsigned TRAYUI_OFFSET_IN_CTRAY = 110;
            INT64         *TrayUIInstance         = *((INT64 *)CTrayInstance + TRAYUI_OFFSET_IN_CTRAY);
            if (TrayUIInstance)
                ((void (*)(void *))(*(INT64 *)((*(INT64 *)TrayUIInstance) + 60 * sizeof(uintptr_t))))(TrayUIInstance);
        }
    }
}
#endif

#pragma endregion


#pragma region "Fix taskbar for classic theme and set Explorer window hooks"
static HWND CreateWindowExWHook(
    DWORD     dwExStyle,
    LPCWSTR   lpClassName,
    LPCWSTR   lpWindowName,
    DWORD     dwStyle,
    int       X,
    int       Y,
    int       nWidth,
    int       nHeight,
    HWND      hWndParent,
    HMENU     hMenu,
    HINSTANCE hInstance,
    LPVOID    lpParam
)
{
    if (bClassicThemeMitigations && (*((WORD *)&(lpClassName) + 1)) && WStrEq(lpClassName, L"TrayNotifyWnd"))
        dwExStyle |= WS_EX_STATICEDGE;
    if (bClassicThemeMitigations && (*((WORD *)&(lpClassName) + 1)) &&
        WStrEq(lpClassName, L"NotifyIconOverflowWindow"))
    {
        dwExStyle |= WS_EX_STATICEDGE;
    }
    if (bClassicThemeMitigations && (*((WORD *)&(lpClassName) + 1)) &&
        (WStrEq(lpClassName, L"SysListView32") || WStrEq(lpClassName, L"SysTreeView32"))) // WStrEq(lpClassName, L"FolderView")
    {
        wchar_t wszClassName[200];
        ZeroMemory(wszClassName, 200);
        GetClassNameW(GetAncestor(hWndParent, GA_ROOT), wszClassName, 200);
        if (WStrEq(wszClassName, L"CabinetWClass"))
            dwExStyle |= WS_EX_CLIENTEDGE;
    }
    if (bIsExplorerProcess && bToolbarSeparators && (*((WORD *)&(lpClassName) + 1)) && WStrEq(lpClassName, L"ReBarWindow32")) {
        wchar_t wszClassName[200];
        ZeroMemory(wszClassName, 200);
        GetClassNameW(hWndParent, wszClassName, 200);
        if (WStrEq(wszClassName, L"Shell_TrayWnd"))
            dwStyle |= RBS_BANDBORDERS;
    }

    HWND hWnd = CreateWindowExWFunc(dwExStyle, lpClassName, lpWindowName, dwStyle,
                                    X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

#ifdef _WIN64
    if (bIsExplorerProcess && (*((WORD *)&(lpClassName) + 1)) &&
        (WStrEq(lpClassName, L"TrayClockWClass") || WStrEq(lpClassName, L"ClockButton")))
    {
        SetWindowSubclass(hWnd, ClockButtonSubclassProc, ClockButtonSubclassProc, 0);
    }
    else if (bIsExplorerProcess && (*((WORD *)&(lpClassName) + 1)) &&
             WStrEq(lpClassName, L"TrayShowDesktopButtonWClass"))
    {
        SetWindowSubclass(hWnd, ShowDesktopSubclassProc, ShowDesktopSubclassProc, 0);
    }
    else if (bIsExplorerProcess && (*((WORD *)&(lpClassName) + 1)) &&
             WStrEq(lpClassName, L"Shell_TrayWnd"))
    {
        SetWindowSubclass(hWnd, Shell_TrayWndSubclassProc, Shell_TrayWndSubclassProc, TRUE);
        hShell_TrayWndMouseHook = SetWindowsHookExW(WH_MOUSE, Shell_TrayWndMouseProc, NULL, GetCurrentThreadId());
    }
    else if (bIsExplorerProcess && (*((WORD *)&(lpClassName) + 1)) &&
             WStrEq(lpClassName, L"Shell_SecondaryTrayWnd"))
    {
        SetWindowSubclass(hWnd, Shell_TrayWndSubclassProc, Shell_TrayWndSubclassProc, FALSE);
    }
    else if (bIsExplorerProcess && (*((WORD *)&(lpClassName) + 1)) &&
             WStrIEq(lpClassName, L"ReBarWindow32") &&
             hWndParent == FindWindowW(L"Shell_TrayWnd", NULL))
    {
        SetWindowSubclass(hWnd, ReBarWindow32SubclassProc, ReBarWindow32SubclassProc, FALSE);
    }
#endif

#if 0
    if (bClassicThemeMitigations && (*((WORD*)&(lpClassName)+1)) && (WStrEq(lpClassName, L"FolderView")))
    {
        wchar_t wszClassName[200];
        GetClassNameW(GetAncestor(hWndParent, GA_ROOT), wszClassName, 200);
        if (WStrEq(wszClassName, L"CabinetWClass"))
        {
            SendMessageW(hWnd, 0x108, 0, 0);
        }
    }
    // SetWindowTheme(hWnd, L" ", L" ");
#endif

    return hWnd;
}

static LONG_PTR SetWindowLongPtrWHook(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
    WCHAR lpClassName[256];
    GetClassNameW(hWnd, lpClassName, _countof(lpClassName));
    HWND hWndParent = GetParent(hWnd);

    if (bClassicThemeMitigations && *((WORD *)&lpClassName + 1) &&
        WStrEq(lpClassName, L"TrayNotifyWnd") && nIndex == GWL_EXSTYLE)
    {
        dwNewLong |= WS_EX_STATICEDGE;
    }
    if (bClassicThemeMitigations && *((WORD *)&lpClassName + 1) &&
        WStrEq(lpClassName, L"NotifyIconOverflowWindow") && nIndex == GWL_EXSTYLE)
    {
        dwNewLong |= WS_EX_STATICEDGE;
    }
    if (bClassicThemeMitigations && (*((WORD *)&(lpClassName) + 1)) &&
        (WStrEq(lpClassName, L"SysListView32") || WStrEq(lpClassName, L"SysTreeView32"))) // WStrEq(lpClassName, L"FolderView")
    {
        wchar_t wszClassName[200];
        ZeroMemory(wszClassName, 200);
        GetClassNameW(GetAncestor(hWndParent, GA_ROOT), wszClassName, 200);
        if (WStrEq(wszClassName, L"CabinetWClass"))
            if (nIndex == GWL_EXSTYLE)
                dwNewLong |= WS_EX_CLIENTEDGE;
    }
    if (bIsExplorerProcess && bToolbarSeparators && (*((WORD *)&(lpClassName) + 1))
        && WStrEq(lpClassName, L"ReBarWindow32"))
    {
        wchar_t wszClassName[200];
        ZeroMemory(wszClassName, 200);
        GetClassNameW(hWndParent, wszClassName, 200);
        if (WStrEq(wszClassName, L"Shell_TrayWnd"))
            if (nIndex == GWL_STYLE)
                dwNewLong |= RBS_BANDBORDERS;
    }

    return SetWindowLongPtrWFunc(hWnd, nIndex, dwNewLong);
}

#ifdef _WIN64
static HRESULT explorer_SetWindowThemeHook(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList)
{
    if (bClassicThemeMitigations) {
        fputws(L"SetWindowTheme\n", stdout);
        return explorer_SetWindowThemeFunc(hwnd, L" ", L" ");
    }
    return explorer_SetWindowThemeFunc(hwnd, pszSubAppName, pszSubIdList);
}

static HRESULT explorer_DrawThemeBackground(
    HTHEME  hTheme,
    HDC     hdc,
    int     iPartId,
    int     iStateId,
    LPCRECT pRect,
    LPCRECT pClipRect
)
{
    if (dwOrbStyle && hOrbCollection) {
        for (unsigned i = 0; i < DPA_GetPtrCount(hOrbCollection); ++i) {
            OrbInfo *oi = DPA_FastGetPtr(hOrbCollection, i);
            if (oi->hTheme == hTheme) {
                BITMAPINFO bi = {
                    .bmiHeader.biSize        = sizeof(BITMAPINFOHEADER),
                    .bmiHeader.biWidth       = 1,
                    .bmiHeader.biHeight      = 1,
                    .bmiHeader.biPlanes      = 1,
                    .bmiHeader.biBitCount    = 32,
                    .bmiHeader.biCompression = BI_RGB,
                };
                RGBQUAD transparent = {0, 0, 0, 0};
                RGBQUAD color       = {0xFF, 0xFF, 0xFF, 0xFF};

                if (dwOrbStyle == ORB_STYLE_WINDOWS11) {
                    UINT separator = oi->dpi / 96;
                    // printf(">>> SEPARATOR %p %d %d\n", oi->hTheme, oi->dpi, separator);

                    // Background
                    StretchDIBits(hdc, pRect->left + (separator % 2),
                                  pRect->top + (separator % 2),
                                  pRect->right - pRect->left - (separator % 2),
                                  pRect->bottom - pRect->top - (separator % 2),
                                  0, 0, 1, 1, &color, &bi, DIB_RGB_COLORS, SRCCOPY);
                    // Middle vertical line
                    StretchDIBits(hdc, pRect->left + ((pRect->right - pRect->left) / 2) - (separator / 2),
                                  pRect->top,
                                  separator,
                                  pRect->bottom - pRect->top,
                                  0, 0, 1, 1, &transparent, &bi, DIB_RGB_COLORS, SRCCOPY);
                    // Middle horizontal line
                    StretchDIBits(hdc, pRect->left,
                                  pRect->top + ((pRect->bottom - pRect->top) / 2) - (separator / 2),
                                  pRect->right - pRect->left,
                                  separator,
                                  0, 0, 1, 1, &transparent, &bi, DIB_RGB_COLORS, SRCCOPY);
                } else if (dwOrbStyle == ORB_STYLE_TRANSPARENT) {
                    StretchDIBits(hdc, pRect->left,
                                  pRect->top,
                                  pRect->right - pRect->left,
                                  pRect->bottom - pRect->top,
                                  0, 0, 1, 1, &transparent, &bi, DIB_RGB_COLORS, SRCCOPY);
                }
                return S_OK;
            }
        }
    }

    if (bClassicThemeMitigations) {
        if (iPartId == 4 && iStateId == 1) {
            COLORREF bc   = GetBkColor(hdc);
            COLORREF fc   = GetTextColor(hdc);
            int      mode = SetBkMode(hdc, TRANSPARENT);

            SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));

            NONCLIENTMETRICSW ncm;
            ncm.cbSize = sizeof(NONCLIENTMETRICSW);
            SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);

            HFONT hFont = CreateFontIndirectW(&(ncm.lfCaptionFont));

            UINT    dpiX, dpiY;
            HRESULT hr = GetDpiForMonitor(MonitorFromWindow(WindowFromDC(hdc), MONITOR_DEFAULTTOPRIMARY), MDT_DEFAULT, &dpiX, &dpiY);
            double dx = dpiX / 96.0, dy = dpiY / 96.0;

            HGDIOBJ hOldFont    = SelectObject(hdc, hFont);
            DWORD   dwTextFlags = DT_SINGLELINE | DT_CENTER | DT_VCENTER;
            RECT    rc          = *pRect;
            rc.bottom -= 7 * dy;
            DrawTextW(hdc, L"\u2026", -1, &rc, dwTextFlags);
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
            SetBkColor(hdc, bc);
            SetTextColor(hdc, fc);
            SetBkMode(hdc, mode);
        }
        return S_OK;
    }

    return DrawThemeBackground(hTheme, hdc, iPartId, iStateId, pRect, pClipRect);
}

static HRESULT explorer_CloseThemeData(HTHEME hTheme)
{
    HRESULT hr = CloseThemeData(hTheme);
    if (SUCCEEDED(hr) && hOrbCollection) {
        for (unsigned i = 0; i < DPA_GetPtrCount(hOrbCollection); ++i) {
            OrbInfo *oi = DPA_FastGetPtr(hOrbCollection, i);
            if (oi->hTheme == hTheme) {
                // printf(">>> DELETE DPA %p %d\n", oi->hTheme, oi->dpi);
                DPA_DeletePtr(hOrbCollection, i);
                free(oi);
                break;
            }
        }
    }
    return hr;
}

static HTHEME explorer_OpenThemeDataForDpi(HWND hwnd, LPCWSTR pszClassList, UINT dpi)
{
    if ((*((WORD *)&(pszClassList) + 1)) && WStrEq(pszClassList, L"TaskbarPearl")) {
        if (!hOrbCollection)
            hOrbCollection = DPA_Create(MAX_NUM_MONITORS);

        HTHEME hTheme = OpenThemeDataForDpi(hwnd, pszClassList, dpi);
        if (hTheme && hOrbCollection) {
            OrbInfo *oi = malloc(sizeof(OrbInfo));
            if (oi) {
                oi->hTheme = hTheme;
                oi->dpi    = dpi;
                // printf(">>> APPEND DPA %p %d\n", oi->hTheme, oi->dpi);
                DPA_AppendPtr(hOrbCollection, oi);
            }
        }
        return hTheme;
    } else if ((*((WORD *)&(pszClassList) + 1)) && WStrEq(pszClassList, L"TaskbarShowDesktop")) {
        DWORD dwVal = 0, dwSize = sizeof(DWORD);
        RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                     L"TaskbarSD", RRF_RT_REG_DWORD, NULL, &dwVal, &dwSize);
        if (dwVal == 2)
            return NULL;
        return OpenThemeDataForDpi(hwnd, pszClassList, dpi);
    }

    // task list - Taskband2 from CTaskListWnd::_HandleThemeChanged
    if (bClassicThemeMitigations && (*((WORD *)&(pszClassList) + 1)) && WStrEq(pszClassList, L"Taskband2")) {
        return 0xDEADBEEF;
    }
    // system tray notification area more icons
    else if (bClassicThemeMitigations && (*((WORD *)&(pszClassList) + 1)) && WStrEq(pszClassList, L"TrayNotifyFlyout")) {
        return 0xABADBABE;
    }
#if 0
    else if (bClassicThemeMitigations && (*((WORD*)&(pszClassList)+1)) && wcsstr(pszClassList, L"::Taskband2"))
    {
        wprintf(L"%s\n", pszClassList);
        return 0xB16B00B5;
    }
#endif
    return OpenThemeDataForDpi(hwnd, pszClassList, dpi);
}

static HRESULT explorer_GetThemeMetric(
    HTHEME hTheme,
    HDC    hdc,
    int    iPartId,
    int    iStateId,
    int    iPropId,
    int   *piVal
)
{
    if (!bClassicThemeMitigations || (hTheme != 0xABADBABE))
        return GetThemeMetric(hTheme, hdc, iPartId, iStateId, iPropId, piVal);
    const int TMT_WIDTH  = 2416;
    const int TMT_HEIGHT = 2417;
    if (hTheme == 0xABADBABE && iPropId == TMT_WIDTH && iPartId == 3 && iStateId == 0)
        *piVal = GetSystemMetrics(SM_CXICON);
    else if (hTheme == 0xABADBABE && iPropId == TMT_HEIGHT && iPartId == 3 && iStateId == 0)
        *piVal = GetSystemMetrics(SM_CYICON);
    return S_OK;
}

static HRESULT explorer_GetThemeMargins(
    HTHEME   hTheme,
    HDC      hdc,
    int      iPartId,
    int      iStateId,
    int      iPropId,
    LPCRECT  prc,
    MARGINS *pMargins
)
{
    if (!bClassicThemeMitigations || (hTheme != 0xDEADBEEF && hTheme != 0xABADBABE)) {
        HRESULT hr = GetThemeMargins(hTheme, hdc, iPartId, iStateId, iPropId, prc, pMargins);
        return hr;
    }
    const int TMT_SIZINGMARGINS  = 3601;
    const int TMT_CONTENTMARGINS = 3602;
    HRESULT   hr                 = S_OK;
    if (hTheme)
        hr = GetThemeMargins(hTheme, hdc, iPartId, iStateId, iPropId, prc, pMargins);
#if 0
    if (hTheme == 0xB16B00B5)
    {
        printf(
            "GetThemeMargins %d %d %d - %d %d %d %d\n",
            iPartId,
            iStateId,
            iPropId,
            pMargins->cxLeftWidth,
            pMargins->cyTopHeight,
            pMargins->cxRightWidth,
            pMargins->cyBottomHeight
        );
    }
#endif
    if (hTheme == 0xDEADBEEF && iPropId == TMT_CONTENTMARGINS && iPartId == 5 && iStateId == 1) {
        // task list button measurements
        pMargins->cxLeftWidth    = 4;
        pMargins->cyTopHeight    = 3;
        pMargins->cxRightWidth   = 4;
        pMargins->cyBottomHeight = 3;
    } else if (hTheme == 0xDEADBEEF && iPropId == TMT_CONTENTMARGINS && iPartId == 1 && iStateId == 0) {
        // task list measurements
        pMargins->cxLeftWidth    = 0;
        pMargins->cyTopHeight    = 0;
        pMargins->cxRightWidth   = 4;
        pMargins->cyBottomHeight = 0;
    } else if (hTheme == 0xDEADBEEF && iPropId == TMT_SIZINGMARGINS && iPartId == 5 && iStateId == 1) {
        pMargins->cxLeftWidth    = 10;
        pMargins->cyTopHeight    = 10;
        pMargins->cxRightWidth   = 10;
        pMargins->cyBottomHeight = 10;
    } else if (hTheme = 0xABADBABE && iPropId == TMT_CONTENTMARGINS && iPartId == 3 && iStateId == 0) {
        pMargins->cxLeftWidth    = 6; // GetSystemMetrics(SM_CXICONSPACING);
        pMargins->cyTopHeight    = 6; // GetSystemMetrics(SM_CYICONSPACING);
        pMargins->cxRightWidth   = 6; // GetSystemMetrics(SM_CXICONSPACING);
        pMargins->cyBottomHeight = 6; // GetSystemMetrics(SM_CYICONSPACING);
    }

    HWND hShell_TrayWnd = FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL);
    if (hShell_TrayWnd) {
        LONG dwStyle = 0;
        dwStyle      = GetWindowLongW(hShell_TrayWnd, GWL_STYLE);
        dwStyle |= WS_DLGFRAME;
        SetWindowLongW(hShell_TrayWnd, GWL_STYLE, dwStyle);
        dwStyle &= ~WS_DLGFRAME;
        SetWindowLongW(hShell_TrayWnd, GWL_STYLE, dwStyle);
    }
    HWND hWnd = NULL;
    do {
        hWnd = FindWindowExW(NULL, hWnd, L"Shell_SecondaryTrayWnd", NULL);
        if (hWnd) {
            LONG dwStyle = 0;
            dwStyle      = GetWindowLongW(hWnd, GWL_STYLE);
            dwStyle |= WS_DLGFRAME;
            SetWindowLongW(hWnd, GWL_STYLE, dwStyle);
            dwStyle &= ~WS_DLGFRAME;
            SetWindowLongW(hWnd, GWL_STYLE, dwStyle);
        }
    } while (hWnd);
    return S_OK;
}

static HRESULT explorer_DrawThemeTextEx(
    HTHEME         hTheme,
    HDC            hdc,
    int            iPartId,
    int            iStateId,
    LPCWSTR        pszText,
    int            cchText,
    DWORD          dwTextFlags,
    LPRECT         pRect,
    const DTTOPTS *pOptions
)
{
    if (!bClassicThemeMitigations)
        return DrawThemeTextEx(hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, pRect, pOptions);

    COLORREF bc   = GetBkColor(hdc);
    COLORREF fc   = GetTextColor(hdc);
    int      mode = SetBkMode(hdc, TRANSPARENT);

    wchar_t text[200];
    GetWindowTextW(GetForegroundWindow(), text, 200);

    BOOL bIsActiveUnhovered   = (iPartId == 5 && iStateId == 5);
    BOOL bIsInactiveUnhovered = (iPartId == 5 && iStateId == 1);
    BOOL bIsInactiveHovered   = (iPartId == 5 && iStateId == 2);
    BOOL bIsActiveHovered     = bIsInactiveHovered && WStrEq(text, pszText);

    SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));

    NONCLIENTMETRICSW ncm;
    ncm.cbSize = sizeof(NONCLIENTMETRICSW);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);

    HFONT hFont = NULL;
    if (bIsActiveUnhovered)
        hFont = CreateFontIndirectW(&(ncm.lfCaptionFont));
    else if (bIsInactiveUnhovered)
        hFont = CreateFontIndirectW(&(ncm.lfMenuFont));
    else if (bIsActiveHovered)
        hFont = CreateFontIndirectW(&(ncm.lfCaptionFont));
    else if (bIsInactiveHovered)
        hFont = CreateFontIndirectW(&(ncm.lfMenuFont));
    else {
        hFont = CreateFontIndirectW(&(ncm.lfMenuFont));
        // wprintf(L"DrawThemeTextEx %d %d %s\n", iPartId, iStateId, pszText);
    }

    if (iPartId == 5 && iStateId == 0) // clock
        pRect->top += 2;

    HGDIOBJ hOldFont = SelectObject(hdc, hFont);
    DrawTextW(hdc, pszText, cchText, pRect, dwTextFlags);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
    SetBkColor(hdc, bc);
    SetTextColor(hdc, fc);
    SetBkMode(hdc, mode);
    return S_OK;
}
#endif
#pragma endregion


#pragma region "Change links"
static int ExplorerFrame_CompareStringOrdinal(const WCHAR *a1, int a2, const WCHAR *a3, int a4, BOOL bIgnoreCase)
{
    static wchar_t const *const pRedirects[] = {
        L"::{BB06C0E4-D293-4F75-8A90-CB05B6477EEE}", // System                     (default: redirected to
                                                     // Settings app)
        L"::{7B81BE6A-CE2B-4676-A29E-EB907A5126C5}", // Programs and Features      (default: not redirected)
        NULL,
        // The following are unused but available for the future
        L"::{D450A8A1-9568-45C7-9C0E-B4F9FB4537BD}", // Installed Updates          (default: not redirected)
        L"::{17CD9488-1228-4B2F-88CE-4298E93E0966}", // Default Programs           (default: not redirected)
        L"::{8E908FC9-BECC-40F6-915B-F4CA0E70D03D}", // Network and Sharing Center (default: not redirected)
        L"::{7007ACC7-3202-11D1-AAD2-00805FC1270E}", // Network Connections        (default: not redirected)
        L"Advanced", // Network and Sharing Center -> Change advanced sharing options (default: not redirected)
        L"::{A8A91A66-3A7D-4424-8D24-04E180695C7A}", // Devices and Printers       (default: not redirected)
        NULL,
    };

    int ret = CompareStringOrdinal(a1, a2, a3, a4, bIgnoreCase);
    if (ret != CSTR_EQUAL)
        return ret;

    int i = 0;
    while (1) {
        BOOL bCond = FALSE;
        if (i == 0)
            bCond = bDoNotRedirectSystemToSettingsApp;
        else if (i == 1)
            bCond = bDoNotRedirectProgramsAndFeaturesToSettingsApp;
        if (CompareStringOrdinal(a3, -1, pRedirects[i], -1, FALSE) == CSTR_EQUAL && bCond)
            break;
        i++;
        if (pRedirects[i] == NULL)
            return ret;
    }

    return CSTR_GREATER_THAN;
}


#ifdef _WIN64

DEFINE_GUID(IID_EnumExplorerCommand,
            0xA88826F8, 0x186F, 0x4987, 0xAA, 0xDE, 0xEA, 0x0C, 0xEF, 0x8F, 0xBF, 0xE8);
DEFINE_GUID(GUID_UICommand_System,
            0x4C202CF0, 0xC4DC, 0x4251, 0xA3, 0x71, 0xB6, 0x22, 0xB4, 0x3D, 0x59, 0x2B);
DEFINE_GUID(GUID_UICommand_ProgramsAndFeatures,
            0xA2E6D9CC, 0xF866, 0x40B6, 0xA4, 0xB2, 0xEE, 0x9E, 0x10, 0x04, 0xBD, 0xFC);


typedef struct EnumExplorerCommandVtbl {
    BEGIN_INTERFACE

    HRESULT(STDMETHODCALLTYPE *QueryInterface)
    (EnumExplorerCommand *This,
     /* [in] */ REFIID    riid,
     /* [annotation][iid_is][out] */
     _COM_Outptr_ void **ppvObject);

    ULONG(STDMETHODCALLTYPE *AddRef)(EnumExplorerCommand *This);
    ULONG(STDMETHODCALLTYPE *Release)(EnumExplorerCommand *This);
    HRESULT(STDMETHODCALLTYPE *Next)(EnumExplorerCommand *This, unsigned a2, void **a3, void *a4);

    END_INTERFACE
} EnumExplorerCommandVtbl;

interface EnumExplorerCommand {
    CONST_VTBL struct EnumExplorerCommandVtbl *lpVtbl;
};

typedef struct UICommandVtbl {
    BEGIN_INTERFACE

    HRESULT(STDMETHODCALLTYPE *QueryInterface)
    (UICommand        *This,
     /* [in] */ REFIID riid,
     /* [annotation][iid_is][out] */
     _COM_Outptr_ void **ppvObject);

    ULONG  (STDMETHODCALLTYPE *AddRef)(UICommand *This);
    ULONG  (STDMETHODCALLTYPE *Release)(UICommand *This);
    HRESULT(STDMETHODCALLTYPE *GetTitle)(UICommand *This);
    HRESULT(STDMETHODCALLTYPE *GetIcon)(UICommand *This);
    HRESULT(STDMETHODCALLTYPE *GetTooltip)(UICommand *This);
    HRESULT(STDMETHODCALLTYPE *GetCanonicalName)(UICommand *This, GUID *guid);
    HRESULT(STDMETHODCALLTYPE *GetState)(UICommand *This);
    HRESULT(STDMETHODCALLTYPE *Invoke)(UICommand *This, void *a2, void *a3);
    HRESULT(STDMETHODCALLTYPE *GetFlags)(UICommand *This);
    HRESULT(STDMETHODCALLTYPE *EnumSubCommands)(UICommand *This);

    END_INTERFACE
} UICommandVtbl;

interface UICommand {
    CONST_VTBL struct UICommandVtbl *lpVtbl;
};

static HRESULT shell32_UICommand_InvokeHook(UICommand *_this, void *a2, void *a3)
{
    // Guid = {A2E6D9CC-F866-40B6-A4B2-EE9E1004BDFC} Programs and Features
    // Guid = {4C202CF0-C4DC-4251-A371-B622B43D592B} System
    GUID guid;
    ZeroMemory(&guid, sizeof(GUID));
    _this->lpVtbl->GetCanonicalName(_this, &guid);
    BOOL bIsSystem              = bDoNotRedirectSystemToSettingsApp && IsEqualGUID(&guid, &GUID_UICommand_System);
    BOOL bIsProgramsAndFeatures = bDoNotRedirectProgramsAndFeaturesToSettingsApp &&
                                  IsEqualGUID(&guid, &GUID_UICommand_ProgramsAndFeatures);

    if (bIsSystem || bIsProgramsAndFeatures) {
        IOpenControlPanel *pOpenControlPanel = NULL;
        CoCreateInstance(&CLSID_OpenControlPanel, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                         &IID_OpenControlPanel, &pOpenControlPanel);

        if (pOpenControlPanel) {
            WCHAR *pszWhat = L"";
            if (bIsSystem)
                pszWhat = L"Microsoft.System";
            else if (bIsProgramsAndFeatures)
                pszWhat = L"Microsoft.ProgramsAndFeatures";
            pOpenControlPanel->lpVtbl->Open(pOpenControlPanel, pszWhat, NULL, NULL);
            pOpenControlPanel->lpVtbl->Release(pOpenControlPanel);
            return S_OK;
        }
    }
    return shell32_UICommand_InvokeFunc(_this, a2, a3);
}

static BOOL explorer_ShellExecuteExW(SHELLEXECUTEINFOW *pExecInfo)
{
    if (bDoNotRedirectSystemToSettingsApp && pExecInfo && pExecInfo->lpFile &&
        WStrEq(pExecInfo->lpFile, L"ms-settings:about"))
    {
        IOpenControlPanel *pOpenControlPanel = NULL;
        CoCreateInstance(&CLSID_OpenControlPanel, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                         &IID_OpenControlPanel, &pOpenControlPanel);
        if (pOpenControlPanel) {
            pOpenControlPanel->lpVtbl->Open(pOpenControlPanel, L"Microsoft.System", NULL, NULL);
            pOpenControlPanel->lpVtbl->Release(pOpenControlPanel);
            return 1;
        }
    }
    return ShellExecuteExW(pExecInfo);
}

static HINSTANCE explorer_ShellExecuteW(
    HWND    hwnd,
    LPCWSTR lpOperation,
    LPCWSTR lpFile,
    LPCWSTR lpParameters,
    LPCWSTR lpDirectory,
    INT     nShowCmd
)
{
    if (bDoNotRedirectNotificationIconsToSettingsApp && WStrEq(lpFile, L"ms-settings:notifications")) {
        return ShellExecuteW(hwnd, lpOperation, L"shell:::{05d7b0f4-2121-4eff-bf6b-ed3f69b894d9}",
                             lpParameters, lpDirectory, nShowCmd);
    } else if (bDoNotRedirectDateAndTimeToSettingsApp && WStrEq(lpFile, L"ms-settings:dateandtime")) {
        return ShellExecuteW(hwnd, lpOperation, L"shell:::{E2E7934B-DCE5-43C4-9576-7FE4F75E7480}",
                             lpParameters, lpDirectory, nShowCmd);
    }
#if 0
    else if (WStrEq(lpFile, L"ms-settings:taskbar"))
    {
        LaunchPropertiesGUI(hModule);
        return 0;
    }
#endif
    return ShellExecuteW(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
}

#endif
#pragma endregion


#pragma region "Classic Drive Grouping"
#ifdef _WIN64

static const struct {
    DWORD dwDescriptionId;
    UINT  uResourceId;
} driveCategoryMap[] = {
    {SHDID_FS_DIRECTORY,        9338                                 }, //  shell32
    {SHDID_COMPUTER_SHAREDDOCS, 9338                                 }, //  shell32
    {SHDID_COMPUTER_FIXED,      IDS_DRIVECATEGORY_HARDDISKDRIVES     },
    {SHDID_COMPUTER_DRIVE35,    IDS_DRIVECATEGORY_REMOVABLESTORAGE   },
    {SHDID_COMPUTER_REMOVABLE,  IDS_DRIVECATEGORY_REMOVABLESTORAGE   },
    {SHDID_COMPUTER_CDROM,      IDS_DRIVECATEGORY_REMOVABLESTORAGE   },
    {SHDID_COMPUTER_DRIVE525,   IDS_DRIVECATEGORY_REMOVABLESTORAGE   },
    {SHDID_COMPUTER_NETDRIVE,   9340                                 }, //  shell32
    {SHDID_COMPUTER_OTHER,      IDS_DRIVECATEGORY_OTHER              },
    {SHDID_COMPUTER_RAMDISK,    IDS_DRIVECATEGORY_OTHER              },
    {SHDID_COMPUTER_IMAGING,    IDS_DRIVECATEGORY_IMAGING            },
    {SHDID_COMPUTER_AUDIO,      IDS_DRIVECATEGORY_PORTABLEMEDIADEVICE},
    {SHDID_MOBILE_DEVICE,       IDS_DRIVECATEGORY_PORTABLEDEVICE     }
};

// Represents the true data structure that is returned from shell32!DllGetClassObject
typedef struct {
    const IClassFactoryVtbl *lpVtbl;
    ULONG                    flags;
    REFCLSID                 rclsid;
    HRESULT (*pfnCreateInstance)(IUnknown *, REFIID, void **);
} Shell32ClassFactoryEntry;

// Represents a custom ICategorizer/IShellExtInit
typedef struct _EPCategorizer {
    ICategorizerVtbl  *categorizer;
    IShellExtInitVtbl *shellExtInit;

    ULONG          ulRefCount;
    IShellFolder2 *pShellFolder;
} EPCategorizer;

# pragma region "EPCategorizer: ICategorizer"

static HRESULT STDMETHODCALLTYPE
EPCategorizer_ICategorizer_QueryInterface(ICategorizer *_this, REFIID riid, void **ppvObject)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICategorizer)) {
        *ppvObject = _this;
    } else if (IsEqualIID(riid, &IID_IShellExtInit)) {
        *ppvObject = &((EPCategorizer *)_this)->shellExtInit;
    } else {
        ppvObject = NULL;
        return E_NOINTERFACE;
    }

    _this->lpVtbl->AddRef(_this);

    return S_OK;
}

static ULONG STDMETHODCALLTYPE
EPCategorizer_ICategorizer_AddRef(ICategorizer *_this)
{
    return InterlockedIncrement(&((EPCategorizer *)_this)->ulRefCount);
}

static ULONG STDMETHODCALLTYPE
EPCategorizer_ICategorizer_Release(ICategorizer *_this)
{
    ULONG ulNewCount = InterlockedDecrement(&((EPCategorizer *)_this)->ulRefCount);

    // When the window is closed or refreshed the object is finally freed
    if (ulNewCount == 0) {
        EPCategorizer *epCategorizer = (EPCategorizer *)_this;

        if (epCategorizer->pShellFolder != NULL) {
            epCategorizer->pShellFolder->lpVtbl->Release(epCategorizer->pShellFolder);
            epCategorizer->pShellFolder = NULL;
        }

        free(epCategorizer);
    }

    return ulNewCount;
}

static HRESULT STDMETHODCALLTYPE
EPCategorizer_ICategorizer_GetDescription(ICategorizer *_this, LPWSTR pszDesc, UINT cch)
{
    // As of writing returns the string "Type". Same implementation as
    // shell32!CStorageSystemTypeCategorizer::GetDescription
    LoadStringW(hShell32, 0x3105, pszDesc, cch);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
EPCategorizer_ICategorizer_GetCategory(
    ICategorizer         *_this,
    UINT                  cidl,
    PCUITEMID_CHILD_ARRAY apidl,
    DWORD                *rgCategoryIds
)
{
    EPCategorizer *epCategorizer = (EPCategorizer *)_this;

    HRESULT hr = S_OK;

    for (UINT i = 0; i < cidl; i++) {
        rgCategoryIds[i] = IDS_DRIVECATEGORY_OTHER;

        PROPERTYKEY key = {FMTID_ShellDetails, PID_DESCRIPTIONID};
        VARIANT     variant;
        VariantInit(&variant);

        hr = epCategorizer->pShellFolder->lpVtbl->GetDetailsEx(epCategorizer->pShellFolder, apidl[i], &key, &variant);

        if (SUCCEEDED(hr)) {
            SHDESCRIPTIONID did;

            if (SUCCEEDED(VariantToBuffer(&variant, &did, sizeof(did)))) {
                for (int j = 0; j < _countof(driveCategoryMap); j++) {
                    if (did.dwDescriptionId == driveCategoryMap[j].dwDescriptionId) {
                        rgCategoryIds[i] = driveCategoryMap[j].uResourceId;
                        break;
                    }
                }
            }

            VariantClear(&variant);
        }
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE
EPCategorizer_ICategorizer_GetCategoryInfo(
    ICategorizer  *_this,
    DWORD          dwCategoryId,
    CATEGORY_INFO *pci
)
{
    // Now retrieve the display name to use for the resource ID dwCategoryId.
    // pci is already populated with most of the information it needs, we just need to fill in the wszName
    if (!LoadStringW(hModule, dwCategoryId, pci->wszName, _countof(pci->wszName)))
        LoadStringW(hShell32, dwCategoryId, pci->wszName, _countof(pci->wszName));

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
EPCategorizer_ICategorizer_CompareCategory(
    ICategorizer *_this,
    CATSORT_FLAGS csfFlags,
    DWORD         dwCategoryId1,
    DWORD         dwCategoryId2
)
{
    // Typically a categorizer would use the resource IDs containing the names of each category as the
    // "category ID" as well. In our case however, we're using a combination of resource/category IDs provided
    // by shell32 and resource/category IDs we're overriding ourselves. As a result, we are forced to compare
    // not by the value of the category/resource IDs themselves, but by their _position_ within our category
    // ID map

    ssize_t firstPos  = -1;
    ssize_t secondPos = -1;

    for (ssize_t i = 0; i < _countof(driveCategoryMap); ++i) {
        if (driveCategoryMap[i].uResourceId == dwCategoryId1) {
            firstPos = i;
            break;
        }
    }

    for (ssize_t i = 0; i < _countof(driveCategoryMap); ++i) {
        if (driveCategoryMap[i].uResourceId == dwCategoryId2) {
            secondPos = i;
            break;
        }
    }

    ssize_t diff = firstPos - secondPos;

    if (diff < 0)
        return 0xFFFF;

    return diff > 0;
}

# pragma endregion
# pragma region "EPCategorizer: IShellExtInit"

// Adjustor Thunks: https://devblogs.microsoft.com/oldnewthing/20040206-00/?p=40723
static HRESULT STDMETHODCALLTYPE
EPCategorizer_IShellExtInit_QueryInterface(IShellExtInit *_this, REFIID riid, void **ppvObject)
{
    return EPCategorizer_ICategorizer_QueryInterface(
        (ICategorizer *)((char *)_this - sizeof(IShellExtInitVtbl *)), riid, ppvObject);
}

static ULONG STDMETHODCALLTYPE
EPCategorizer_IShellExtInit_AddRef(IShellExtInit *_this)
{
    return EPCategorizer_ICategorizer_AddRef(
        (ICategorizer *)((char *)_this - sizeof(IShellExtInitVtbl *)));
}

static ULONG STDMETHODCALLTYPE
EPCategorizer_IShellExtInit_Release(IShellExtInit *_this)
{
    return EPCategorizer_ICategorizer_Release(
        (ICategorizer *)((char *)_this - sizeof(IShellExtInitVtbl *)));
}

static HRESULT STDMETHODCALLTYPE
EPCategorizer_IShellExtInit_Initialize(
    IShellExtInit    *_this,
    PCIDLIST_ABSOLUTE pidlFolder,
    IDataObject      *pdtobj,
    HKEY              hkeyProgID
)
{
    EPCategorizer *epCategorizer = (EPCategorizer *)((char *)_this - sizeof(IShellExtInitVtbl *));

    return SHBindToObject(NULL, pidlFolder, NULL, &IID_IShellFolder2, (void **)&epCategorizer->pShellFolder);
}

# pragma endregion

static const ICategorizerVtbl EPCategorizer_categorizerVtbl = {
    EPCategorizer_ICategorizer_QueryInterface,
    EPCategorizer_ICategorizer_AddRef,
    EPCategorizer_ICategorizer_Release,
    EPCategorizer_ICategorizer_GetDescription,
    EPCategorizer_ICategorizer_GetCategory,
    EPCategorizer_ICategorizer_GetCategoryInfo,
    EPCategorizer_ICategorizer_CompareCategory
};

static const IShellExtInitVtbl EPCategorizer_shellExtInitVtbl = {
    EPCategorizer_IShellExtInit_QueryInterface,
    EPCategorizer_IShellExtInit_AddRef,
    EPCategorizer_IShellExtInit_Release,
    EPCategorizer_IShellExtInit_Initialize
};

static HRESULT shell32_DriveTypeCategorizer_CreateInstanceHook(
    IUnknown *pUnkOuter,
    REFIID    riid,
    void    **ppvObject
)
{
    if (bUseClassicDriveGrouping && IsEqualIID(riid, &IID_ICategorizer)) {
        EPCategorizer *epCategorizer = malloc(sizeof(EPCategorizer));
        epCategorizer->categorizer   = &EPCategorizer_categorizerVtbl;
        epCategorizer->shellExtInit  = &EPCategorizer_shellExtInitVtbl;
        epCategorizer->ulRefCount    = 1;
        epCategorizer->pShellFolder  = NULL;

        *ppvObject = epCategorizer;
        return S_OK;
    }

    return shell32_DriveTypeCategorizer_CreateInstanceFunc(pUnkOuter, riid, ppvObject);
}

#endif
#pragma endregion


#pragma region "File Explorer command bar and ribbon support"
DEFINE_GUID(CLSID_UIRibbonFramework,
            0x926749FA, 0x2615, 0x4987, 0x88, 0x45, 0xC3, 0x3E, 0x65, 0xF2, 0xB9, 0x57);
DEFINE_GUID(IID_UIRibbonFramework,
            0xF4F0385D, 0x6872, 0x43A8, 0xAD, 0x09, 0x4C, 0x33, 0x9C, 0xB3, 0xF5, 0xC5);

static HRESULT ExplorerFrame_CoCreateInstanceHook(
    REFCLSID  rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD     dwClsContext,
    REFIID    riid,
    LPVOID   *ppv
)
{
    if (dwFileExplorerCommandUI != 0 &&
        *(INT64 *)&rclsid->Data1 == 0x4D1E5A836480100B &&
        *(INT64 *)rclsid->Data4 == 0x339A8EA8E58A699F)
    {
        return REGDB_E_CLASSNOTREG;
    }
    if (dwFileExplorerCommandUI == 2 &&
        IsEqualCLSID(rclsid, &CLSID_UIRibbonFramework) &&
        IsEqualIID(riid, &IID_UIRibbonFramework))
    {
        return REGDB_E_CLASSNOTREG;
    }

    return CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}
#pragma endregion


#pragma region "Change language UI style"
#ifdef _WIN64
DEFINE_GUID(CLSID_InputSwitchControl,
            0xB9BC2A50, 0x43C3, 0x41AA, 0xA0, 0x86, 0x5D, 0xB1, 0x4E, 0x18, 0x4B, 0xAE);
DEFINE_GUID(IID_InputSwitchControl,
            0xB9BC2A50, 0x43C3, 0x41AA, 0xA0, 0x82, 0x5D, 0xB1, 0x4E, 0x18, 0x4B, 0xAE);

# define LANGUAGEUI_STYLE_DESKTOP       0 // Windows 11 style
# define LANGUAGEUI_STYLE_TOUCHKEYBOARD 1 // Windows 10 style
# define LANGUAGEUI_STYLE_LOGONUI       2
# define LANGUAGEUI_STYLE_UAC           3
# define LANGUAGEUI_STYLE_SETTINGSPANE  4
# define LANGUAGEUI_STYLE_OOBE          5
# define LANGUAGEUI_STYLE_OTHER         100

#if 0
char  mov_edx_val[6] = {0xBA, 0x00, 0x00, 0x00, 0x00, 0xC3};
char *ep_pf          = NULL;
#endif

typedef struct IInputSwitchControlVtbl {
    BEGIN_INTERFACE

    HRESULT(STDMETHODCALLTYPE *QueryInterface)
    (IInputSwitchControl *This,
     /* [in] */ REFIID    riid,
     /* [annotation][iid_is][out] */
     _COM_Outptr_ void **ppvObject);

    ULONG(STDMETHODCALLTYPE *AddRef)(IInputSwitchControl *This);

    ULONG(STDMETHODCALLTYPE *Release)(IInputSwitchControl *This);

    HRESULT(STDMETHODCALLTYPE *Init)
    (IInputSwitchControl *This,
     /* [in] */ unsigned clientType);

    HRESULT(STDMETHODCALLTYPE *SetCallback)
    (IInputSwitchControl *This,
     /* [in] */ void     *pInputSwitchCallback);

    HRESULT(STDMETHODCALLTYPE *ShowInputSwitch)
    (IInputSwitchControl *This,
     /* [in] */ RECT     *lpRect);

    HRESULT(STDMETHODCALLTYPE *GetProfileCount)
    (IInputSwitchControl *This,
     /* [in] */ unsigned *pOutNumberOfProfiles,
     /* [in] */ int      *a3);

    // ...

    END_INTERFACE
} IInputSwitchControlVtbl;

interface IInputSwitchControl {
    CONST_VTBL struct IInputSwitchControlVtbl *lpVtbl;
};

static HRESULT CInputSwitchControl_InitHook(IInputSwitchControl *_this, unsigned dwOriginalIMEStyle)
{
    return CInputSwitchControl_InitFunc(_this, dwIMEStyle ? dwIMEStyle : dwOriginalIMEStyle);
}

static HRESULT CInputSwitchControl_ShowInputSwitchHook(IInputSwitchControl *_this, RECT *lpRect)
{
    if (!dwIMEStyle) // impossible case (this is not called for the Windows 11 language switcher), but just in case
        return CInputSwitchControl_ShowInputSwitchFunc(_this, lpRect);

    unsigned dwNumberOfProfiles = 0;
    int      a3                 = 0;
    _this->lpVtbl->GetProfileCount(_this, &dwNumberOfProfiles, &a3);

    HWND hWndTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);

    UINT    dpiX = 96, dpiY = 96;
    HRESULT hr = GetDpiForMonitor(MonitorFromWindow(hWndTaskbar, MONITOR_DEFAULTTOPRIMARY), MDT_DEFAULT, &dpiX, &dpiY);
    double  dpix = dpiX / 96.0;
    double  dpiy = dpiY / 96.0;

    // printf("RECT %d %d %d %d - %d %d\n", lpRect->left, lpRect->right, lpRect->top, lpRect->bottom,
    // dwNumberOfProfiles, a3);

    RECT rc;
    GetWindowRect(hWndTaskbar, &rc);
    POINT pt;
    pt.x       = rc.left;
    pt.y       = rc.top;
    UINT tbPos = GetTaskbarLocationAndSize(pt, &rc);
    if (tbPos == TB_POS_BOTTOM) {
    } else if (tbPos == TB_POS_TOP) {
        if (dwIMEStyle == 1) // Windows 10 (with Language preferences link)
        {
            lpRect->top = rc.top + (rc.bottom - rc.top) +
                          (UINT)(((double)dwNumberOfProfiles * (60.0 * dpiy)) +
                                 (5.0 * dpiy * 4.0) + (dpiy) + (48.0 * dpiy));
        } else if (dwIMEStyle == 2 || dwIMEStyle == 3 || dwIMEStyle == 4 || dwIMEStyle == 5) // LOGONUI, UAC, Windows 10, OOBE
        {
            lpRect->top = rc.top + (rc.bottom - rc.top) +
                          (UINT)(((double)dwNumberOfProfiles * (60.0 * dpiy)) + (5.0 * dpiy * 2.0));
        }
    } else if (tbPos == TB_POS_LEFT) {
        if (dwIMEStyle == 1 || dwIMEStyle == 2 || dwIMEStyle == 3 || dwIMEStyle == 4 || dwIMEStyle == 5)
        {
            lpRect->right = rc.left + (rc.right - rc.left) + (UINT)((double)(300.0 * dpix));
            lpRect->top += (lpRect->bottom - lpRect->top);
        }
    }
    if (tbPos == TB_POS_RIGHT) {
        if (dwIMEStyle == 1 || dwIMEStyle == 2 || dwIMEStyle == 3 || dwIMEStyle == 4 || dwIMEStyle == 5)
        {
            lpRect->right = lpRect->right - (rc.right - rc.left);
            lpRect->top += (lpRect->bottom - lpRect->top);
        }
    }

    if (dwIMEStyle == 4)
        lpRect->right -= (UINT)((double)(300.0 * dpix)) - (lpRect->right - lpRect->left);

    return CInputSwitchControl_ShowInputSwitchFunc(_this, lpRect);
}

static HRESULT explorer_CoCreateInstanceHook(
    REFCLSID   rclsid,
    LPUNKNOWN  pUnkOuter,
    DWORD      dwClsContext,
    REFIID     riid,
    IUnknown **ppv
)
{
    if (IsEqualCLSID(rclsid, &CLSID_InputSwitchControl) && IsEqualIID(riid, &IID_InputSwitchControl)) {
        HRESULT hr = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
        if (SUCCEEDED(hr)) {
            // The commented method below is no longer required as I have now came to patching
            // the interface's vtable.
            // Also, make sure to read the explanation below as well, it's useful for understanding
            // how this worked.
            IInputSwitchControl *pInputSwitchControl = *ppv;
            DWORD flOldProtect = 0;
            if (VirtualProtect(pInputSwitchControl->lpVtbl, sizeof(IInputSwitchControlVtbl),
                               PAGE_EXECUTE_READWRITE, &flOldProtect))
            {
                CInputSwitchControl_ShowInputSwitchFunc      = pInputSwitchControl->lpVtbl->ShowInputSwitch;
                pInputSwitchControl->lpVtbl->ShowInputSwitch = CInputSwitchControl_ShowInputSwitchHook;
                CInputSwitchControl_InitFunc                 = pInputSwitchControl->lpVtbl->Init;
                pInputSwitchControl->lpVtbl->Init            = CInputSwitchControl_InitHook;
                VirtualProtect(pInputSwitchControl->lpVtbl, sizeof(IInputSwitchControlVtbl), flOldProtect, &flOldProtect);
            }

            // Pff... how this works:
            //
            // * This `CoCreateInstance` call will get a pointer to an IInputSwitchControl interface
            // (the call to here is made from `explorer!CTrayInputIndicator::_RegisterInputSwitch`);
            // the next call on this pointer will be on the `IInputSwitchControl::Init` function.
            //
            // * `IInputSwitchControl::Init`'s second parameter is a number (x) which tells which
            // language switcher UI to prepare (check `IsUtil::MapClientTypeToString` in
            // `InputSwitch.dll`). "explorer" requests the "DESKTOP" UI (x = 0), which is the new
            // Windows 11 UI; if we replace that number with something else, some other UI will
            // be created
            //
            // * ~~We cannot patch the vtable of the COM object because the executable is protected
            // by control flow guard and we would make a jump to an invalid site (maybe there is
            // some clever workaround fpr this as well, somehow telling the compiler to place a certain
            // canary before our trampoline, so it matches with what the runtime support for CFG expects,
            // but we'd have to keep that canary in sync with the one in explorer.exe, so not very
            // future proof).~~ Edit: Not true after all.
            //
            // * Taking advantage of the fact that the call to `IInputSwitchControl::Init` is the thing
            // that happens right after we return from here, and looking on the disassembly, we see nothing
            // else changes `rdx` (which is the second argument to a function call), basically x, besides the
            // very `xor edx, edx` instruction before the call. Thus, we patch that out, and we also do
            // `mov edx, whatever` here; afterwards, we do NOTHING else, but just return and hope that
            // edx will stick
            //
            // * Needless to say this is **HIGHLY** amd64
#if 0
            char pattern[2] = {0x33, 0xD2};
            DWORD dwOldProtect;
            char* p_mov_edx_val = mov_edx_val;
            if (!ep_pf)
            {
                ep_pf = memmem(_ReturnAddress(), 200, pattern, 2);
                if (ep_pf)
                {
                    // Cancel out `xor edx, edx`
                    VirtualProtect(ep_pf, 2, PAGE_EXECUTE_READWRITE, &dwOldProtect);
                    memset(ep_pf, 0x90, 2);
                    VirtualProtect(ep_pf, 2, dwOldProtect, &dwOldProtect);
                }
                VirtualProtect(p_mov_edx_val, 6, PAGE_EXECUTE_READWRITE, &dwOldProtect);
            }
            if (ep_pf)
            {
                // Craft a "function" which does `mov edx, whatever; ret` and call it
                DWORD* pVal = mov_edx_val + 1;
                *pVal = dwIMEStyle;
                void(*pf_mov_edx_val)() = p_mov_edx_val;
                pf_mov_edx_val();
            }
#endif
        }
        return hr;
    }
    return CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}
#endif
#pragma endregion


#pragma region "Explorer Registry Hooks"
static LSTATUS explorer_RegCreateKeyExW(
    HKEY                 a1,
    const WCHAR         *a2,
    DWORD                a3,
    WCHAR               *a4,
    DWORD                a5,
    REGSAM               a6,
    SECURITY_ATTRIBUTES *a7,
    HKEY                *a8,
    DWORD               *a9
)
{
    const wchar_t *v13; // rdx
    int            v14; // eax

    if (a2 && !lWStrEq(a2, L"MMStuckRects3")) {
        v14 = lstrcmpW(a2, L"StuckRects3");
        v13 = L"StuckRectsLegacy";
        if (v14 != 0)
            v13 = a2;
    } else {
        v13 = L"MMStuckRectsLegacy";
    }

    return RegCreateKeyExW(a1, v13, a3, a4, a5, a6, a7, a8, a9);
}

static LSTATUS explorer_SHGetValueW(
    HKEY         a1,
    const WCHAR *a2,
    const WCHAR *a3,
    DWORD       *a4,
    void        *a5,
    DWORD       *a6
)
{
    const WCHAR *v10; // rdx
    int          v11; // eax

    if (a2 && !lWStrEq(a2, L"MMStuckRects3")) {
        v11 = lstrcmpW(a2, L"StuckRects3");
        v10 = L"StuckRectsLegacy";
        if (v11 != 0)
            v10 = a2;
    } else {
        v10 = L"MMStuckRectsLegacy";
    }

    return SHGetValueW(a1, v10, a3, a4, a5, a6);
}

static LSTATUS explorer_OpenRegStream(HKEY hkey, PCWSTR pszSubkey, PCWSTR pszValue, DWORD grfMode)
{
    DWORD flOldProtect[6];

    if (pszValue && lWStrIEq(pszValue, L"TaskbarWinXP") &&
        VirtualProtect(pszValue, 0xC8ui64, 0x40u, flOldProtect))
    {
        lstrcpyW(pszValue, L"TaskbarWinEP");
        VirtualProtect(pszValue, 0xC8ui64, flOldProtect[0], flOldProtect);
    }

    return OpenRegStream(hkey, pszSubkey, pszValue, grfMode);
}

static LSTATUS explorer_RegOpenKeyExW(HKEY a1, WCHAR *a2, DWORD a3, REGSAM a4, HKEY *a5)
{
    DWORD flOldProtect[6];

    if (a2 && lWStrIEq(a2, L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\TrayNotify") &&
        VirtualProtect(a2, 0xC8ui64, 0x40u, flOldProtect))
    {
        lstrcpyW(a2, L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\TrayNotSIB");
        VirtualProtect(a2, 0xC8ui64, flOldProtect[0], flOldProtect);
    }

    return RegOpenKeyExW(a1, a2, a3, a4, a5);
}

static LSTATUS explorer_RegSetValueExW(
    HKEY        hKey,
    LPCWSTR     lpValueName,
    DWORD       Reserved,
    DWORD       dwType,
    const BYTE *lpData,
    DWORD       cbData
)
{
    if (IsWindows11() && lpValueName && lWStrEq(lpValueName, L"ShowCortanaButton")) {
        if (cbData == sizeof(DWORD) && *(DWORD *)lpData == 1) {
            DWORD dwData = 2;
            return RegSetValueExW(hKey, L"TaskbarDa", Reserved, dwType, &dwData, cbData);
        }
        return RegSetValueExW(hKey, L"TaskbarDa", Reserved, dwType, lpData, cbData);
    }

    return RegSetValueExW(hKey, lpValueName, Reserved, dwType, lpData, cbData);
}

static LSTATUS explorer_RegGetValueW(
    HKEY    hkey,
    LPCWSTR lpSubKey,
    LPCWSTR lpValue,
    DWORD   dwFlags,
    LPDWORD pdwType,
    PVOID   pvData,
    LPDWORD pcbData
)
{
    DWORD   flOldProtect;
    BOOL    bShowTaskViewButton = FALSE;
    LSTATUS lRes;

    if (IsWindows11() && lpValue && lWStrEq(lpValue, L"ShowCortanaButton"))
    {
        lRes = RegGetValueW(hkey, lpSubKey, L"TaskbarDa", dwFlags, pdwType, pvData, pcbData);
        if (*(DWORD *)pvData == 2)
            *(DWORD *)pvData = 1;
    } else if (IsWindows11() && lpValue &&
               (lWStrEq(lpValue, L"TaskbarGlomLevel") || lWStrEq(lpValue, L"MMTaskbarGlomLevel")))
    {
        lRes = RegGetValueW(HKEY_CURRENT_USER, L"" REGPATH, lpValue, dwFlags, pdwType, pvData, pcbData);
        if (lRes != ERROR_SUCCESS) {
            *(DWORD *)pvData  = (lpValue[0] == L'T' ? TASKBARGLOMLEVEL_DEFAULT : MMTASKBARGLOMLEVEL_DEFAULT);
            *(DWORD *)pcbData = sizeof(DWORD32);
            lRes              = ERROR_SUCCESS;
        }
    }
#if 0
    else if (WStrEq(lpValue, L"PeopleBand")) {
        lRes = RegGetValueW(hkey, lpSubKey, L"TaskbarMn", dwFlags, pdwType, pvData, pcbData);
    }
#endif
    else {
        lRes = RegGetValueW(hkey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);
    }

    if (IsWindows11() && lpValue && lWStrEq(lpValue, L"SearchboxTaskbarMode")) {
        if (*(DWORD *)pvData)
            *(DWORD *)pvData = 1;

        lRes = ERROR_SUCCESS;
    }

    return lRes;
}

static LSTATUS twinuipcshell_RegGetValueW(
    HKEY    hkey,
    LPCWSTR lpSubKey,
    LPCWSTR lpValue,
    DWORD   dwFlags,
    LPDWORD pdwType,
    PVOID   pvData,
    LPDWORD pcbData
)
{
    LSTATUS lRes = RegGetValueW(hkey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);

    if (lpValue && lpValue && lWStrEq(lpValue, L"AltTabSettings")) {
        if (lRes == ERROR_SUCCESS && *(DWORD *)pvData)
            *(DWORD *)pvData = *(DWORD *)pvData == 3 ? 0 : 1;

        if (!bOldTaskbar && hWin11AltTabInitialized) {
            SetEvent(hWin11AltTabInitialized);
            CloseHandle(hWin11AltTabInitialized);
            hWin11AltTabInitialized = NULL;
        }

        lRes = ERROR_SUCCESS;
    }

    return lRes;
}

static HRESULT WINAPI explorer_SHCreateStreamOnModuleResourceWHook(
    HMODULE hModule,
    LPCWSTR pwszName,
    LPCWSTR pwszType,
    IStream **ppStream
)
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(hModule, path, MAX_PATH);

    if ((*((WORD *)&(pwszName) + 1))) {
        wprintf(L"%ls - %ls %ls\n", path, pwszName, pwszType);
    } else {
        wprintf(L"%ls - %d %ls\n", path, pwszName, pwszType);

        IStream *pStream = NULL;
        if (pwszName < 124) {
            if (S_Icon_Dark_TaskView) {
                pStream = SHCreateMemStream(P_Icon_Dark_TaskView, S_Icon_Dark_TaskView);
                if (pStream) {
                    *ppStream = pStream;
                    return S_OK;
                }
            }
        } else if (pwszName >= 151) {
            if (pwszName < 163) {
                if (S_Icon_Dark_Search) {
                    pStream = SHCreateMemStream(P_Icon_Dark_Search, S_Icon_Dark_Search);
                    if (pStream) {
                        *ppStream = pStream;
                        return S_OK;
                    }
                }
            }
            if (pwszName < 201) {
                if (S_Icon_Light_Search) {
                    pStream = SHCreateMemStream(P_Icon_Light_Search, S_Icon_Light_Search);
                    if (pStream) {
                        *ppStream = pStream;
                        return S_OK;
                    }
                }
            }
            if (pwszName < 213) {
                if (S_Icon_Dark_Widgets) {
                    wprintf(L">>> %p %ld\n", P_Icon_Dark_Widgets, S_Icon_Dark_Widgets);
                    pStream = SHCreateMemStream(P_Icon_Dark_Widgets, S_Icon_Dark_Widgets);
                    if (pStream) {
                        *ppStream = pStream;
                        return S_OK;
                    }
                }
            }
            if (pwszName < 251) {
                if (S_Icon_Light_Widgets) {
                    pStream = SHCreateMemStream(P_Icon_Light_Widgets, S_Icon_Light_Widgets);
                    if (pStream) {
                        *ppStream = pStream;
                        return S_OK;
                    }
                }
            }
        } else if (pwszName < 307) {
            if (S_Icon_Light_TaskView) {
                pStream = SHCreateMemStream(P_Icon_Light_TaskView, S_Icon_Light_TaskView);
                if (pStream) {
                    *ppStream = pStream;
                    return S_OK;
                }
            }
        }
    }
    return explorer_SHCreateStreamOnModuleResourceWFunc(hModule, pwszName, pwszType, ppStream);
}
#pragma endregion


#pragma region "Remember primary taskbar positioning"
static BOOL explorer_SetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom)
{
    static BOOL bTaskbarFirstTimePositioning = FALSE;
    static BOOL bTaskbarSet                  = FALSE;

    BOOL bIgnore = FALSE;
    if (bTaskbarFirstTimePositioning) {
        bIgnore = bTaskbarSet;
    } else {
        bTaskbarFirstTimePositioning = TRUE;
        bIgnore     = (GetSystemMetrics(SM_CMONITORS) == 1);
        bTaskbarSet = bIgnore;
    }

    if (bIgnore)
        return SetRect(lprc, xLeft, yTop, xRight, yBottom);
    if (xLeft)
        return SetRect(lprc, xLeft, yTop, xRight, yBottom);
    if (yTop)
        return SetRect(lprc, xLeft, yTop, xRight, yBottom);
    if (xRight != GetSystemMetrics(SM_CXSCREEN))
        return SetRect(lprc, xLeft, yTop, xRight, yBottom);
    if (yBottom != GetSystemMetrics(SM_CYSCREEN))
        return SetRect(lprc, xLeft, yTop, xRight, yBottom);

    bTaskbarSet = TRUE;

    StuckRectsData srd;
    DWORD          pcbData = sizeof(StuckRectsData);
    RegGetValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRectsLegacy",
                 L"Settings", REG_BINARY, NULL, &srd, &pcbData);

    if (pcbData != sizeof(StuckRectsData))
        return SetRect(lprc, xLeft, yTop, xRight, yBottom);
    if (srd.pvData[0] != sizeof(StuckRectsData))
        return SetRect(lprc, xLeft, yTop, xRight, yBottom);
    if (srd.pvData[1] != -2)
        return SetRect(lprc, xLeft, yTop, xRight, yBottom);

    HMONITOR    hMonitor = MonitorFromRect(&(srd.rc), MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi       = {.cbSize = sizeof(MONITORINFO)};
    if (!GetMonitorInfoW(hMonitor, &mi))
        return SetRect(lprc, xLeft, yTop, xRight, yBottom);

    if (lprc) {
        *lprc = mi.rcMonitor;
        return TRUE;
    }

    return FALSE;
}
#pragma endregion


#pragma region "Disable Office Hotkeys"
static BOOL explorer_RegisterHotkeyHook(HWND hWnd, int id, UINT fsModifiers, UINT vk)
{
    if (bDisableOfficeHotkeys && fsModifiers == (MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_NOREPEAT) && (
        vk == office_hotkeys[0] ||
        vk == office_hotkeys[1] ||
        vk == office_hotkeys[2] ||
        vk == office_hotkeys[3] ||
        vk == office_hotkeys[4] ||
        vk == office_hotkeys[5] ||
        vk == office_hotkeys[6] ||
        vk == office_hotkeys[7] ||
        vk == office_hotkeys[8] ||
        vk == office_hotkeys[9] ||
        !vk))
    static const UINT office_hotkeys[10] = {0x57, 0x54, 0x59, 0x4F, 0x50, 0x44, 0x4C, 0x58, 0x4E, 0x20};

    if (fsModifiers == (MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_NOREPEAT) &&
        (vk == office_hotkeys[0] || vk == office_hotkeys[1] || vk == office_hotkeys[2] ||
         vk == office_hotkeys[3] || vk == office_hotkeys[4] || vk == office_hotkeys[5] ||
         vk == office_hotkeys[6] || vk == office_hotkeys[7] || vk == office_hotkeys[8] ||
         vk == office_hotkeys[9] || !vk))
    {
        SetLastError(ERROR_HOTKEY_ALREADY_REGISTERED);
        return FALSE;
    }

    BOOL result = RegisterHotKey(hWnd, id, fsModifiers, vk);

    static BOOL bWinBHotkeyRegistered = FALSE;
    if (!bWinBHotkeyRegistered && fsModifiers == (MOD_WIN | MOD_NOREPEAT) && vk == 'D') // right after Win+D
    {
#if USE_MOMENT_3_FIXES_ON_MOMENT_2
        BOOL bPerformMoment2Patches = IsWindows11Version22H2Build1413OrHigher();
#else
        BOOL bPerformMoment2Patches = IsWindows11Version22H2Build2134OrHigher();
#endif
        if (bPerformMoment2Patches && global_rovi.dwBuildNumber == 22621 && bOldTaskbar)
        {
            // Might be better if we scan the GlobalKeylist array to prevent hardcoded numbers?
            RegisterHotKey(hWnd, 500, MOD_WIN | MOD_NOREPEAT, 'A');
            RegisterHotKey(hWnd, 514, MOD_WIN | MOD_NOREPEAT, 'B');
            RegisterHotKey(hWnd, 591, MOD_WIN | MOD_NOREPEAT, 'N');
            printf("Registered Win+A, Win+B, and Win+N\n");
        }
        bWinBHotkeyRegistered = TRUE;
    }

    return result;
}

BOOL twinui_RegisterHotkeyHook(HWND hWnd, int id, UINT fsModifiers, UINT vk)
{
    if (fsModifiers == (MOD_WIN | MOD_NOREPEAT) && vk == 'F')
    {
        SetLastError(ERROR_HOTKEY_ALREADY_REGISTERED);
        return FALSE;
    }
    return RegisterHotKey(hWnd, id, fsModifiers, vk);
}
#pragma endregion


#pragma region "Redirect certain library loads to other versions"
static HMODULE patched_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    WCHAR path[MAX_PATH];
    GetSystemDirectoryW(path, MAX_PATH);
    wcscat_s(path, MAX_PATH, L"\\AppResolver.dll");
    if (WStrIEq(path, lpLibFileName)) {
        GetWindowsDirectoryW(path, MAX_PATH);
        wcscat_s(path, MAX_PATH,
                 L"\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\AppResolverLegacy.dll");
        return LoadLibraryExW(path, hFile, dwFlags);
    }
    if (IsWindows11Version22H2Build1413OrHigher())
        return LoadLibraryExW(lpLibFileName, hFile, dwFlags);
    GetSystemDirectoryW(path, MAX_PATH);
    wcscat_s(path, MAX_PATH, L"\\StartTileData.dll");
    if (WStrIEq(path, lpLibFileName)) {
        GetWindowsDirectoryW(path, MAX_PATH);
        wcscat_s(path, MAX_PATH,
                 L"\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\StartTileDataLegacy.dll");
        return LoadLibraryExW(path, hFile, dwFlags);
    }
    return LoadLibraryExW(lpLibFileName, hFile, dwFlags);
}
#pragma endregion


#pragma region "Fix taskbar thumbnails and acrylic in newer OS builds (22572+)"
#ifdef _WIN64
static HRESULT explorer_DwmUpdateThumbnailPropertiesHook(HTHUMBNAIL hThumbnailId, DWM_THUMBNAIL_PROPERTIES *ptnProperties)
{
    if (ptnProperties->dwFlags == 0 || ptnProperties->dwFlags == DWM_TNP_RECTSOURCE) {
        ptnProperties->dwFlags |= DWM_TNP_SOURCECLIENTAREAONLY;
        ptnProperties->fSourceClientAreaOnly = TRUE;
    }
    return DwmUpdateThumbnailProperties(hThumbnailId, ptnProperties);
}

static BOOL WINAPI explorer_SetWindowCompositionAttribute(HWND hWnd, WINCOMPATTRDATA *pData)
{
    if (bClassicThemeMitigations)
        return TRUE;

    if (bOldTaskbar && GetTaskbarColor && GetTaskbarTheme &&
        global_rovi.dwBuildNumber >= 22581 &&
        pData->nAttribute == 19 && pData->pData &&
        pData->ulDataSize == sizeof(ACCENTPOLICY))
    {
        WORD wCw      = GetClassWord(hWnd, GCW_ATOM);
        WORD wTray    = RegisterWindowMessageW(L"Shell_TrayWnd");
        WORD wSecTray = RegisterWindowMessageW(L"Shell_SecondaryTrayWnd");

        if (wCw == wTray || wCw == wSecTray) {
            ACCENTPOLICY *pAccentPolicy = pData->pData;
            pAccentPolicy->nAccentState = (unsigned __int16)GetTaskbarTheme() >> 8 != 0 ? 4 : 1;
            pAccentPolicy->nColor       = GetTaskbarColor(0, 0);
        }
    }

    return SetWindowCompositionAttribute(hWnd, pData);
}

static void PatchExplorer_UpdateWindowAccentProperties(void)
{
    HMODULE hExplorer = GetModuleHandleW(NULL);
    if (hExplorer) {
        PIMAGE_DOS_HEADER dosHeader = hExplorer;
        if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
            PIMAGE_NT_HEADERS64 ntHeader = (PIMAGE_NT_HEADERS64)((BYTE *)dosHeader + dosHeader->e_lfanew);
            if (ntHeader->Signature == IMAGE_NT_SIGNATURE) {
                uint8_t *pPatchArea = NULL;
                // test al, al; jz rip+0x11; and ...
                uint8_t  p1[]     = {0x84, 0xC0, 0x74, 0x11, 0x83, 0x65};
                uint8_t  p2[]     = {0xF3, 0xF3, 0xF3, 0xFF};
                uint8_t *pattern1 = p1;
                int sizeof_pattern1 = 6;
                if (global_rovi.dwBuildNumber >= 22581) {
                    pattern1        = p2;
                    sizeof_pattern1 = 4;
                }
                BOOL bTwice  = FALSE;
                PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeader);
                for (unsigned i = 0; i < ntHeader->FileHeader.NumberOfSections; ++i) {
                    if (section->Characteristics & IMAGE_SCN_CNT_CODE) {
                        if (section->SizeOfRawData && !bTwice) {
                            char *pCandidate = NULL;
                            while (TRUE) {
                                pCandidate = memmem(
                                    !pCandidate ? hExplorer + section->VirtualAddress : pCandidate,
                                    !pCandidate ? section->SizeOfRawData
                                                : (uintptr_t)section->SizeOfRawData - (uintptr_t)(
                                                    (ptrdiff_t)pCandidate - (ptrdiff_t)(hExplorer + section->VirtualAddress)),
                                    pattern1, sizeof_pattern1
                                );
                                if (!pCandidate)
                                    break;
                                if (!pPatchArea)
                                    pPatchArea = pCandidate;
                                else
                                    bTwice = TRUE;
                                pCandidate += sizeof_pattern1;
                            }
                        }
                    }
                    section++;
                }
                if (pPatchArea && !bTwice) {
                    if (global_rovi.dwBuildNumber >= 22581) {
                        int           dec_size            = 200;
                        _DecodedInst *decodedInstructions = calloc(110, sizeof(_DecodedInst));
                        if (decodedInstructions) {
                            unsigned      decodedInstructionsCount = 0;
                            _DecodeResult res = distorm_decode(
                                0, pPatchArea - dec_size, dec_size + 20,
                                Decode64Bits, decodedInstructions, 100, &decodedInstructionsCount
                            );

                            int status = 0;
                            for (int i = decodedInstructionsCount - 1; i >= 0; i--) {
                                if (status == 0 && strstr(decodedInstructions[i].instructionHex.p, "f3f3f3ff")) {
                                    status = 1;
                                } else if (status == 1 && strcmp(decodedInstructions[i].instructionHex.p, "c3") == 0) {
                                    status = 2;
                                } else if (status == 2 && strcmp(decodedInstructions[i].instructionHex.p, "cc") != 0) {
                                    GetTaskbarColor = pPatchArea - dec_size + decodedInstructions[i].offset;
                                    status          = 3;
                                } else if (status == 3 && strncmp(decodedInstructions[i].instructionHex.p, "e8", 2) == 0) {
                                    status = 4;
                                } else if (status == 4 && strncmp(decodedInstructions[i].instructionHex.p, "e8", 2) == 0) {
                                    uint32_t *off   = pPatchArea - dec_size + decodedInstructions[i].offset + 1;
                                    GetTaskbarTheme = pPatchArea - dec_size + decodedInstructions[i].offset +
                                                      decodedInstructions[i].size + (*off);
                                    break;
                                }
                                if (status >= 2) {
                                    i = i + 2;
                                    if (i >= decodedInstructionsCount)
                                        break;
                                }
                            }
                            if (SetWindowCompositionAttribute && GetTaskbarColor && GetTaskbarTheme) {
                                VnPatchIAT(GetModuleHandleW(NULL), "user32.dll", "SetWindowCompositionAttribute", explorer_SetWindowCompositionAttribute);
                                wprintf(L"Patched taskbar transparency in newer OS builds\n");
                            }
                            free(decodedInstructions);
                        }
                    } else {
                        DWORD dwOldProtect;
                        VirtualProtect(pPatchArea, sizeof_pattern1, PAGE_EXECUTE_READWRITE, &dwOldProtect);
                        pPatchArea[2] = 0xEB; // replace jz with jmp
                        VirtualProtect(pPatchArea, sizeof_pattern1, dwOldProtect, &dwOldProtect);
                    }
                }
            }
        }
    }
}
#endif
#pragma endregion


#pragma region "Revert legacy copy dialog"
static BOOL SHELL32_CanDisplayWin8CopyDialogHook(void)
{
    if (bLegacyFileTransferDialog)
        return FALSE;
    return SHELL32_CanDisplayWin8CopyDialogFunc();
}
#pragma endregion


#pragma region "Windows Spotlight customization"
#ifdef _WIN64

static LSTATUS shell32_RegCreateKeyExW(
    HKEY                        hKey,
    LPCWSTR                     lpSubKey,
    DWORD                       Reserved,
    LPWSTR                      lpClass,
    DWORD                       dwOptions,
    REGSAM                      samDesired,
    const LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY                       phkResult,
    LPDWORD                     lpdwDisposition
)
{
    if (bDisableSpotlightIcon && hKey == HKEY_CURRENT_USER &&
        WStrIEq(lpSubKey, L"Software\\Classes\\CLSID\\{2cc5ca98-6485-489a-920e-b3e88a6ccce3}"))
    {
        LSTATUS lRes = RegCreateKeyExW(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired,
                                       lpSecurityAttributes, phkResult, lpdwDisposition);
        if (lRes == ERROR_SUCCESS)
            hKeySpotlight1 = *phkResult;
        return lRes;
    } else if (hKeySpotlight1 && hKey == hKeySpotlight1 && WStrIEq(lpSubKey, L"ShellFolder"))
    {
        LSTATUS lRes = RegCreateKeyExW(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired,
                                       lpSecurityAttributes, phkResult, lpdwDisposition);
        if (lRes == ERROR_SUCCESS)
            hKeySpotlight2 = *phkResult;
        return lRes;
    }
    return RegCreateKeyExW(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired,
                           lpSecurityAttributes, phkResult, lpdwDisposition);
}

static LSTATUS shell32_RegSetValueExW(
    HKEY        hKey,
    LPCWSTR     lpValueName,
    DWORD       Reserved,
    DWORD       dwType,
    const BYTE *lpData,
    DWORD       cbData
)
{
    if (hKeySpotlight1 && hKeySpotlight2 && hKey == hKeySpotlight2 &&
        WStrIEq(lpValueName, L"Attributes"))
    {
        hKeySpotlight1     = NULL;
        hKeySpotlight2     = NULL;
        DWORD dwAttributes = *(DWORD *)lpData | SFGAO_NONENUMERATED;
        SHFlushSFCache();
        return RegSetValueExW(hKey, lpValueName, Reserved, dwType, &dwAttributes, cbData);
    }
    return RegSetValueExW(hKey, lpValueName, Reserved, dwType, lpData, cbData);
}

static BOOL shell32_DeleteMenu(HMENU hMenu, UINT uPosition, UINT uFlags)
{
    if (uPosition == 0x7053 && IsSpotlightEnabled() && dwSpotlightDesktopMenuMask)
        bSpotlightIsDesktopContextMenu = TRUE;
    return DeleteMenu(hMenu, uPosition, uFlags);
}

static BOOL shell32_TrackPopupMenu(
    HMENU       hMenu,
    UINT        uFlags,
    int         x,
    int         y,
    int         nReserved,
    HWND        hWnd,
    const RECT *prcRect
)
{
    if (IsSpotlightEnabled() && dwSpotlightDesktopMenuMask &&
        (GetPropW(GetParent(hWnd), L"DesktopWindow") &&
         (RegisterWindowMessageW(L"WorkerW") == GetClassWord(GetParent(hWnd), GCW_ATOM) ||
          RegisterWindowMessageW(L"Progman") == GetClassWord(GetParent(hWnd), GCW_ATOM))) &&
        bSpotlightIsDesktopContextMenu)
    {
        SpotlightHelper(dwSpotlightDesktopMenuMask, hWnd, hMenu, NULL);
    }
    bSpotlightIsDesktopContextMenu = FALSE;
    BOOL bRet = TrackPopupMenuHook(hMenu, uFlags, x, y, nReserved, hWnd, prcRect);
    if (IsSpotlightEnabled() && dwSpotlightDesktopMenuMask) {
        MENUITEMINFOW mii = {
            .cbSize = sizeof mii,
            .fMask  = MIIM_FTYPE | MIIM_DATA,
        };
        if (GetMenuItemInfoW(hMenu, bRet, FALSE, &mii) && mii.dwItemData >= SPOP_CLICKMENU_FIRST &&
            mii.dwItemData <= SPOP_CLICKMENU_LAST) {
            SpotlightHelper(mii.dwItemData, hWnd, hMenu, NULL);
        }
    }
    return bRet;
}

#endif
#pragma endregion


#pragma region "Fix Windows 10 taskbar high DPI button width bug"
#ifdef _WIN64
static int patched_GetSystemMetrics(int nIndex)
{
    if ((bOldTaskbar && nIndex == SM_CXMINIMIZED) ||
        nIndex == SM_CXICONSPACING || nIndex == SM_CYICONSPACING)
    {
        wchar_t wszDim[MAX_PATH + 4] = {0};
        DWORD    dwSize = MAX_PATH;
        wchar_t *pVal   = L"MinWidth";

        if (nIndex == SM_CXICONSPACING)
            pVal = L"IconSpacing";
        else if (nIndex == SM_CYICONSPACING)
            pVal = L"IconVerticalSpacing";
        RegGetValueW(HKEY_CURRENT_USER, L"Control Panel\\Desktop\\WindowMetrics",
                     pVal, SRRF_RT_REG_SZ, NULL, wszDim, &dwSize);

        int dwDim = (int)wcstoll(wszDim, NULL, 0);
        if (dwDim <= 0) {
            if (nIndex == SM_CXMINIMIZED)
                return 160;
            else if (dwDim < 0)
                return MulDiv(dwDim, GetDpiForSystem(), -1440);
        }
    }

    return GetSystemMetrics(nIndex);
}
#endif
#pragma endregion


#pragma region "Fix Windows 10 taskbar redraw problem on OS builds 22621+"
static HWND Windows11v22H2_explorer_CreateWindowExW(
    DWORD     dwExStyle,
    LPCWSTR   lpClassName,
    LPCWSTR   lpWindowName,
    DWORD     dwStyle,
    int       X,
    int       Y,
    int       nWidth,
    int       nHeight,
    HWND      hWndParent,
    HMENU     hMenu,
    HINSTANCE hInstance,
    LPVOID    lpParam
)
{
    if ((*((WORD *)&(lpClassName) + 1)) && WStrEq(lpClassName, L"Shell_TrayWnd"))
        dwStyle |= WS_CLIPCHILDREN;
    return CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y,
                           nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}
#pragma endregion


#pragma region "Shrink File Explorer address bar height"
static int explorerframe_GetSystemMetricsForDpi(int nIndex, UINT dpi)
{
    if (bShrinkExplorerAddressBar && nIndex == SM_CYFIXEDFRAME)
        return 0;
    return GetSystemMetricsForDpi(nIndex, dpi);
}
#pragma endregion


#pragma region "Fix taskbar cascade and tile windows options not working"
static WORD explorer_TileWindows(
    HWND        hwndParent,
    UINT        wHow,
    const RECT *lpRect,
    UINT        cKids,
    const HWND *lpKids
)
{
    return TileWindows((hwndParent == GetShellWindow()) ? GetDesktopWindow() : hwndParent,
                       wHow, lpRect, cKids, lpKids);
}

static WORD explorer_CascadeWindows(
    HWND        hwndParent,
    UINT        wHow,
    const RECT *lpRect,
    UINT        cKids,
    const HWND *lpKids
)
{
    return CascadeWindows((hwndParent == GetShellWindow()) ? GetDesktopWindow() : hwndParent,
                          wHow, lpRect, cKids, lpKids);
}
#pragma endregion


#pragma region "Fix explorer crashing on Windows builds lower than 25158"
static HWND user32_NtUserFindWindowExHook(
    HWND    hWndParent,
    HWND    hWndChildAfter,
    LPCWSTR lpszClass,
    LPCWSTR lpszWindow,
    DWORD   dwType
)
{
    if (!NtUserFindWindowEx)
        NtUserFindWindowEx = GetProcAddress(GetModuleHandleW(L"win32u.dll"), "NtUserFindWindowEx");
    HWND hWnd = NULL;
    for (int i = 0; i < 5; ++i) {
        if (hWnd)
            break;
        hWnd = NtUserFindWindowEx(hWndParent, hWndChildAfter, lpszClass, lpszWindow, dwType);
    }
    return hWnd;
}
#pragma endregion


#pragma region "Infrastructure for reporting which OS features are enabled"
#pragma pack(push, 1)
struct RTL_FEATURE_CONFIGURATION {
    uint32_t featureId;
    uint32_t group               : 4;
    uint32_t enabledState        : 2;
    uint32_t enabledStateOptions : 1;
    uint32_t unused1             : 1;
    uint32_t variant             : 6;
    uint32_t variantPayloadKind  : 2;
    uint32_t unused2             : 16;
    uint32_t payload;
};
#pragma pack(pop)

static int RtlQueryFeatureConfigurationHook(
    UINT32  featureId,
    int     sectionType,
    INT64  *changeStamp,
    struct RTL_FEATURE_CONFIGURATION *buffer
)
{
    int rv = RtlQueryFeatureConfigurationFunc(featureId, sectionType, changeStamp, buffer);
#if !USE_MOMENT_3_FIXES_ON_MOMENT_2
    if (IsWindows11Version22H2Build1413OrHigher() && bOldTaskbar && featureId == 26008830) {
        // Disable tablet optimized taskbar feature when using the Windows 10 taskbar
        //
        // For now, this fixes Task View and Win-Tab, Alt-Tab breaking after pressing Win-Tab,
        // flyouts alignment, notification center alignment, Windows key shortcuts on
        // OS builds 22621.1413+
        //
        buffer->enabledState = FEATURE_ENABLED_STATE_DISABLED;
    }
#endif
    return rv;
}
#pragma endregion


static DWORD InjectBasicFunctions(BOOL bIsExplorer, BOOL bInstall)
{
    // Sleep(150);

    HMODULE hShlwapi = LoadLibraryW(L"Shlwapi.dll");
    if (hShlwapi) {
        if (bInstall) {
            SHRegGetValueFromHKCUHKLMFunc = GetProcAddress(hShlwapi, "SHRegGetValueFromHKCUHKLM");
        } else {
            FreeLibrary(hShlwapi);
            FreeLibrary(hShlwapi);
        }
    }

    HANDLE hShell32 = LoadLibraryW(L"shell32.dll");
    if (hShell32) {
        if (bInstall) {
#ifdef _WIN64
            if (DoesOSBuildSupportSpotlight()) {
                VnPatchIAT(hShell32, "user32.dll", "TrackPopupMenu", shell32_TrackPopupMenu);
            } else {
#endif
                VnPatchIAT(hShell32, "user32.dll", "TrackPopupMenu", TrackPopupMenuHook);
#ifdef _WIN64
            }
#endif
            VnPatchIAT(hShell32, "user32.dll", "SystemParametersInfoW", DisableImmersiveMenus_SystemParametersInfoW);
            if (!bIsExplorer) {
                CreateWindowExWFunc = CreateWindowExW;
                VnPatchIAT(hShell32, "user32.dll", "CreateWindowExW", CreateWindowExWHook);
                SetWindowLongPtrWFunc = SetWindowLongPtrW;
                VnPatchIAT(hShell32, "user32.dll", "SetWindowLongPtrW", SetWindowLongPtrWHook);
            }
        } else {
            VnPatchIAT(hShell32, "user32.dll", "TrackPopupMenu", TrackPopupMenu);
            VnPatchIAT(hShell32, "user32.dll", "SystemParametersInfoW", SystemParametersInfoW);
            if (!bIsExplorer) {
                VnPatchIAT(hShell32, "user32.dll", "CreateWindowExW", CreateWindowExW);
                VnPatchIAT(hShell32, "user32.dll", "SetWindowLongPtrW", SetWindowLongPtrW);
            }
            FreeLibrary(hShell32);
            FreeLibrary(hShell32);
        }
    }

    HANDLE hShcore = LoadLibraryW(L"shcore.dll");
    if (hShcore) {
        if (bInstall) {
            explorerframe_SHCreateWorkerWindowFunc = GetProcAddress(hShcore, (LPCSTR)188);
        } else {
            FreeLibrary(hShcore);
            FreeLibrary(hShcore);
        }
    }

    HANDLE hExplorerFrame = LoadLibraryW(L"ExplorerFrame.dll");
    if (hExplorerFrame) {
        if (bInstall) {
            VnPatchIAT(hExplorerFrame, "user32.dll", "TrackPopupMenu", TrackPopupMenuHook);
            VnPatchIAT(hExplorerFrame, "user32.dll", "SystemParametersInfoW", DisableImmersiveMenus_SystemParametersInfoW);
            VnPatchIAT(hExplorerFrame, "shcore.dll", (LPCSTR)188, explorerframe_SHCreateWorkerWindowHook); // <<<SAB>>>
            if (!bIsExplorer) {
                CreateWindowExWFunc = CreateWindowExW;
                VnPatchIAT(hExplorerFrame, "user32.dll", "CreateWindowExW", CreateWindowExWHook);
                SetWindowLongPtrWFunc = SetWindowLongPtrW;
                VnPatchIAT(hExplorerFrame, "user32.dll", "SetWindowLongPtrW", SetWindowLongPtrWHook);
            }
            VnPatchIAT(hExplorerFrame, "API-MS-WIN-CORE-STRING-L1-1-0.DLL", "CompareStringOrdinal", ExplorerFrame_CompareStringOrdinal);
            VnPatchIAT(hExplorerFrame, "user32.dll", "GetSystemMetricsForDpi", explorerframe_GetSystemMetricsForDpi);
        } else {
            VnPatchIAT(hExplorerFrame, "user32.dll", "TrackPopupMenu", TrackPopupMenu);
            VnPatchIAT(hExplorerFrame, "user32.dll", "SystemParametersInfoW", SystemParametersInfoW);
            VnPatchIAT(hExplorerFrame, "shcore.dll", (LPCSTR)188, explorerframe_SHCreateWorkerWindowFunc);
            if (!bIsExplorer) {
                VnPatchIAT(hExplorerFrame, "user32.dll", "CreateWindowExW", CreateWindowExW);
                VnPatchIAT(hExplorerFrame, "user32.dll", "SetWindowLongPtrW", SetWindowLongPtrW);
            }
            VnPatchIAT(hExplorerFrame, "API-MS-WIN-CORE-STRING-L1-1-0.DLL", "CompareStringOrdinal", CompareStringOrdinal);
            VnPatchIAT(hExplorerFrame, "user32.dll", "GetSystemMetricsForDpi", GetSystemMetricsForDpi);
            FreeLibrary(hExplorerFrame);
            FreeLibrary(hExplorerFrame);
        }
    }

    HANDLE hWindowsUIFileExplorer = LoadLibraryW(L"Windows.UI.FileExplorer.dll");
    if (hWindowsUIFileExplorer) {
        if (bInstall) {
            VnPatchDelayIAT(hWindowsUIFileExplorer, "user32.dll", "TrackPopupMenu", TrackPopupMenuHook);
            VnPatchDelayIAT(hWindowsUIFileExplorer, "user32.dll", "SystemParametersInfoW", DisableImmersiveMenus_SystemParametersInfoW);
            if (!bIsExplorer) {
                CreateWindowExWFunc = CreateWindowExW;
                VnPatchIAT(hWindowsUIFileExplorer, "user32.dll", "CreateWindowExW", CreateWindowExWHook);
                SetWindowLongPtrWFunc = SetWindowLongPtrW;
                VnPatchIAT(hWindowsUIFileExplorer, "user32.dll", "SetWindowLongPtrW", SetWindowLongPtrWHook);
            }
        } else {
            VnPatchDelayIAT(hWindowsUIFileExplorer, "user32.dll", "TrackPopupMenu", TrackPopupMenu);
            VnPatchDelayIAT(hWindowsUIFileExplorer, "user32.dll", "SystemParametersInfoW", SystemParametersInfoW);
            if (!bIsExplorer) {
                VnPatchIAT(hWindowsUIFileExplorer, "user32.dll", "CreateWindowExW", CreateWindowExW);
                VnPatchIAT(hWindowsUIFileExplorer, "user32.dll", "SetWindowLongPtrW", SetWindowLongPtrW);
            }
            FreeLibrary(hWindowsUIFileExplorer);
            FreeLibrary(hWindowsUIFileExplorer);
        }
    }

    DWORD dwPermitOldStartTileData = FALSE;
    DWORD dwSize                   = sizeof(DWORD);
    if (bInstall) {
        dwSize = sizeof(DWORD);
        RegGetValueW(
            HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
            L"Start_ShowClassicMode", RRF_RT_DWORD, NULL, &dwStartShowClassicMode, &dwSize
        );
        dwSize = sizeof(DWORD);
        RegGetValueW(
            HKEY_CURRENT_USER, L"Software\\ExplorerPatcher", L"PermitOldStartTileDataOneShot", RRF_RT_DWORD, NULL,
            &dwPermitOldStartTileData, &dwSize
        );
    }
    if (dwStartShowClassicMode && dwPermitOldStartTileData) {
        HANDLE hCombase = LoadLibraryW(L"combase.dll");
        if (hCombase) {
            if (bInstall) {
                WCHAR wszPath[MAX_PATH], wszExpectedPath[MAX_PATH];
                ZeroMemory(wszPath, MAX_PATH);
                ZeroMemory(wszExpectedPath, MAX_PATH);
                DWORD  dwLength = MAX_PATH;
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
                if (hProcess) {
                    QueryFullProcessImageNameW(hProcess, 0, wszPath, &dwLength);
                    CloseHandle(hProcess);
                }
                if (GetWindowsDirectoryW(wszExpectedPath, MAX_PATH)) {
                    wcscat_s(wszExpectedPath, MAX_PATH, L"\\explorer.exe");
                    if (WStrIEq(wszPath, wszExpectedPath)) {
                        dwPermitOldStartTileData = FALSE;
                        dwSize                   = sizeof(DWORD);
                        RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Software\\ExplorerPatcher", L"PermitOldStartTileDataOneShot");
                        VnPatchIAT(hCombase, "api-ms-win-core-libraryloader-l1-2-0.dll", "LoadLibraryExW", patched_LoadLibraryExW);
                    }
                } else {
                    VnPatchIAT(hCombase, "api-ms-win-core-libraryloader-l1-2-0.dll", "LoadLibraryExW", LoadLibraryExW);
                    FreeLibrary(hCombase);
                    FreeLibrary(hCombase);
                }
            }
        }
    }

    return 0;
}

static INT64 twinui_pcshell_IsUndockedAssetAvailableHook(INT a1, INT64 a2, INT64 a3, const char *a4)
{
    // if IsAltTab and AltTabSettings == Windows 10 or sws (Precision Touchpad gesture)
    if (a1 == 1 && (dwAltTabSettings == 3 || dwAltTabSettings == 2)) {
        return 0;
    }
    // if IsSnapAssist and SnapAssistSettings == Windows 10
    else if (a1 == 4 && dwSnapAssistSettings == 3 && !IsWindows11Version22H2OrHigher()) {
        return 0;
    }
    // else, show Windows 11 style basically
    else {
        if (twinui_pcshell_IsUndockedAssetAvailableFunc)
            return twinui_pcshell_IsUndockedAssetAvailableFunc(a1, a2, a3, a4);
        return 1;
    }
}

static INT64 twinui_pcshell_CMultitaskingViewManager__CreateXamlMTVHostHook(
    INT64    a1,
    unsigned a2,
    INT64    a3,
    INT64    a4,
    INT64   *a5
)
{
    if (!twinui_pcshell_IsUndockedAssetAvailableHook(a2, 0, 0, NULL))
        return twinui_pcshell_CMultitaskingViewManager__CreateDCompMTVHostFunc(a1, a2, a3, a4, a5);
    return twinui_pcshell_CMultitaskingViewManager__CreateXamlMTVHostFunc(a1, a2, a3, a4, a5);
}

#if _WIN64
static struct
{
    int coroInstance_rcOut; // 22621.1992: 0x10
    int coroInstance_pHardwareConfirmatorHost; // 22621.1992: 0xFD
    int hardwareConfirmatorHost_bIsInLockScreen; // 22621.1992: 0xEC
} g_Moment2PatchOffsets;

BOOL Moment2PatchActionCenter(LPMODULEINFO mi)
{
    /***
    Step 1:
    Scan within the DLL.
    ```0F 10 45 ?? F3 0F 7F 07 80 BE // rcMonitor = mi.rcMonitor; // movups - movdqu - cmp```
    22621.1992: 7E2F0
    22621.2283: 140D5

    22621.1992 has a different compiled code structure than 22621.2283 therefore we have to use a different approach:
    Short circuiting the `if (26008830 is enabled)`.
    22621.1992: 7E313

    Step 2:
    Scan within the function for the real fix.
    ```0F 10 45 ?? F3 0F 7F 07 48 // *a2 = mi.rcWork; // movups - movdqu - test```
    22621.2283: 1414B

    Step 3:
    After the first jz starting from step 1, write a jmp to the address found in step 2.
    Find within couple bytes from step 1:
    ```48 8D // lea```
    22621.2283: 140E6

    Step 4:
    Change jz to jmp after the real fix, short circuiting `if (b) unconditional_release_ref(...)`.
    +11 from the movups in step 2.
    22621.2283: 14156
    74 -> EB
    ***/

    PBYTE step1 = FindPattern(mi->lpBaseOfDll, mi->SizeOfImage, "\x0F\x10\x45\x00\xF3\x0F\x7F\x07\x80\xBE", "xxx?xxxxxx");
    if (!step1) return FALSE;
    printf("[CActionCenterExperienceManager::GetViewPosition()] step1 = %lX\n", step1 - (PBYTE)mi->lpBaseOfDll);

    if (!IsWindows11Version22H2Build2134OrHigher()) // We're on 1413-1992
    {
#if USE_MOMENT_3_FIXES_ON_MOMENT_2
        PBYTE featureCheckJz = step1 + 35;
        if (*featureCheckJz != 0x0F && *(featureCheckJz + 1) != 0x84) return FALSE;

        DWORD dwOldProtect = 0;
        PBYTE jzAddr = featureCheckJz + 6 + *(DWORD*)(featureCheckJz + 2);
        if (!VirtualProtect(featureCheckJz, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect)) return FALSE;
        featureCheckJz[0] = 0xE9;
        *(DWORD*)(featureCheckJz + 1) = (DWORD)(jzAddr - featureCheckJz - 5);
        VirtualProtect(featureCheckJz, 5, dwOldProtect, &dwOldProtect);
        goto done;
#else
        return FALSE;
#endif
    }

    PBYTE step2 = FindPattern(step1 + 1, 200, "\x0F\x10\x45\x00\xF3\x0F\x7F\x07\x48", "xxx?xxxxx");
    if (!step2) return FALSE;
    printf("[CActionCenterExperienceManager::GetViewPosition()] step2 = %lX\n", step2 - (PBYTE)mi->lpBaseOfDll);

    PBYTE step3 = FindPattern(step1 + 1, 32, "\x48\x8D", "xx");
    if (!step3) return FALSE;
    printf("[CActionCenterExperienceManager::GetViewPosition()] step3 = %lX\n", step3 - (PBYTE)mi->lpBaseOfDll);

    PBYTE step4 = step2 + 11;
    printf("[CActionCenterExperienceManager::GetViewPosition()] step4 = %lX\n", step4 - (PBYTE)mi->lpBaseOfDll);
    if (*step4 != 0x74) return FALSE;

    // Execution
    DWORD dwOldProtect = 0;
    if (!VirtualProtect(step3, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect)) return FALSE;
    step3[0] = 0xE9;
    *(DWORD*)(step3 + 1) = (DWORD)(step2 - step3 - 5);
    VirtualProtect(step3, 5, dwOldProtect, &dwOldProtect);

    dwOldProtect = 0;
    if (!VirtualProtect(step4, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect)) return FALSE;
    step4[0] = 0xEB;
    VirtualProtect(step4, 1, dwOldProtect, &dwOldProtect);

done:
    printf("[CActionCenterExperienceManager::GetViewPosition()] Patched!\n");
    return TRUE;
}

BOOL Moment2PatchControlCenter(LPMODULEINFO mi)
{
    /***
    Step 1:
    Scan within the DLL.
    ```0F 10 44 24 ?? F3 0F 7F 44 24 ?? 80 BF // rcMonitor = mi.rcMonitor; // movups - movdqu - cmp```
    22621.1992: 4B35B
    22621.2283: 65C5C

    Step 2:
    Scan within the function for the real fix. This pattern applies to both ControlCenter and ToastCenter.
    ```0F 10 45 ?? F3 0F 7F 44 24 ?? 48 // rcMonitor = mi.rcWork; // movups - movdqu - test```
    22621.1992: 4B3FD and 4B418 (The second one is compiled out in later builds)
    22621.2283: 65CE6

    Step 3:
    After the first jz starting from step 1, write a jmp to the address found in step 2.
    Find within couple bytes from step 1:
    ```48 8D // lea```
    22621.1992: 4B373
    22621.2283: 65C74

    Step 4:
    Change jz to jmp after the real fix, short circuiting `if (b) unconditional_release_ref(...)`.
    +13 from the movups in step 2.
    22621.1992: 4B40A
    22621.2283: 65CE3
    74 -> EB
    ***/

    PBYTE step1 = FindPattern(mi->lpBaseOfDll, mi->SizeOfImage, "\x0F\x10\x44\x24\x00\xF3\x0F\x7F\x44\x24\x00\x80\xBF", "xxxx?xxxxx?xx");
    if (!step1) return FALSE;
    printf("[CControlCenterExperienceManager::PositionView()] step1 = %lX\n", step1 - (PBYTE)mi->lpBaseOfDll);

    PBYTE step2 = FindPattern(step1 + 1, 256, "\x0F\x10\x45\x00\xF3\x0F\x7F\x44\x24\x00\x48", "xxx?xxxxx?x");
    if (!step2) return FALSE;
    printf("[CControlCenterExperienceManager::PositionView()] step2 = %lX\n", step2 - (PBYTE)mi->lpBaseOfDll);

    PBYTE step3 = FindPattern(step1 + 1, 32, "\x48\x8D", "xx");
    if (!step3) return FALSE;
    printf("[CControlCenterExperienceManager::PositionView()] step3 = %lX\n", step3 - (PBYTE)mi->lpBaseOfDll);

    PBYTE step4 = step2 + 13;
    printf("[CControlCenterExperienceManager::PositionView()] step4 = %lX\n", step4 - (PBYTE)mi->lpBaseOfDll);
    if (*step4 != 0x74) return FALSE;

    // Execution
    DWORD dwOldProtect = 0;
    if (!VirtualProtect(step3, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect)) return FALSE;
    step3[0] = 0xE9;
    *(DWORD*)(step3 + 1) = (DWORD)(step2 - step3 - 5);
    VirtualProtect(step3, 5, dwOldProtect, &dwOldProtect);

    dwOldProtect = 0;
    if (!VirtualProtect(step4, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect)) return FALSE;
    step4[0] = 0xEB;
    VirtualProtect(step4, 1, dwOldProtect, &dwOldProtect);

    printf("[CControlCenterExperienceManager::PositionView()] Patched!\n");
    return TRUE;
}

BOOL Moment2PatchToastCenter(LPMODULEINFO mi)
{
    /***
    Step 1:
    Scan within the DLL.
    ```0F 10 45 84 ?? 0F 7F 44 24 ?? 48 8B CF // rcMonitor = mi.rcMonitor; // movups - movdqu - mov```
    22621.1992: 40CE8
    22621.2283: 501DB

    Step 2:
    Scan within the function for the real fix. This pattern applies to both ControlCenter and ToastCenter.
    ```0F 10 45 ?? F3 0F 7F 44 24 ?? 48 // rcMonitor = mi.rcWork; // movups - movdqu - test```
    22621.1992: 40D8B
    22621.2283: 5025D

    Step 3:
    After the first jz starting from step 1, write a jmp to the address found in step 2.
    Find within couple bytes from step 1:
    ```48 8D // lea```
    22621.1992: 40D02
    22621.2283: 501F5

    Step 4:
    Change jz to jmp after the real fix, short circuiting `if (b) unconditional_release_ref(...)`.
    +13 from the movups in step 2.
    22621.1992: 40D98
    22621.2283: 5026A

    Note: We are skipping EdgeUI calls here.
    ***/

    PBYTE step1 = FindPattern(mi->lpBaseOfDll, mi->SizeOfImage, "\x0F\x10\x45\x84\x00\x0F\x7F\x44\x24\x00\x48\x8B\xCF", "xxxx?xxxx?xxx");
    if (!step1) return FALSE;
    printf("[CToastCenterExperienceManager::PositionView()] step1 = %lX\n", step1 - (PBYTE)mi->lpBaseOfDll);

    PBYTE step2 = FindPattern(step1 + 1, 200, "\x0F\x10\x45\x00\xF3\x0F\x7F\x44\x24\x00\x48", "xxx?xxxxx?x");
    if (!step2) return FALSE;
    printf("[CToastCenterExperienceManager::PositionView()] step2 = %lX\n", step2 - (PBYTE)mi->lpBaseOfDll);

    PBYTE step3 = FindPattern(step1 + 1, 32, "\x48\x8D", "xx");
    if (!step3) return FALSE;
    printf("[CToastCenterExperienceManager::PositionView()] step3 = %lX\n", step3 - (PBYTE)mi->lpBaseOfDll);

    PBYTE step4 = step2 + 13;
    printf("[CToastCenterExperienceManager::PositionView()] step4 = %lX\n", step4 - (PBYTE)mi->lpBaseOfDll);
    if (*step4 != 0x0F /*When the else block is big*/ && *step4 != 0x74) return FALSE;

    // Execution
    DWORD dwOldProtect = 0;
    if (!VirtualProtect(step3, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect)) return FALSE;
    step3[0] = 0xE9;
    *(DWORD*)(step3 + 1) = (DWORD)(step2 - step3 - 5);
    VirtualProtect(step3, 5, dwOldProtect, &dwOldProtect);

    dwOldProtect = 0;
    if (*step4 == 0x74) // Same size, just change the opcode
    {
        if (!VirtualProtect(step4, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect)) return FALSE;
        step4[0] = 0xEB;
        VirtualProtect(step4, 1, dwOldProtect, &dwOldProtect);
    }
    else // The big one
    {
        PBYTE jzAddr = step4 + 6 + *(DWORD*)(step4 + 2);
        if (!VirtualProtect(step4, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect)) return FALSE;
        step4[0] = 0xE9;
        *(DWORD*)(step4 + 1) = (DWORD)(jzAddr - step4 - 5);
        VirtualProtect(step4, 5, dwOldProtect, &dwOldProtect);
    }

    printf("[CToastCenterExperienceManager::PositionView()] Patched!\n");
    return TRUE;
}

BOOL Moment2PatchTaskView(LPMODULEINFO mi)
{
    /***
    If we're using the old taskbar, it'll be stuck in an infinite loading since it's waiting for the new one to respond.
    Let's safely skip those by NOPing the `TaskViewFrame::UpdateWorkAreaAsync()` and `WaitForCompletion()` calls, and
    turning off the COM object cleanup.

    Step 1:
    Scan within the DLL to find the beginning, which is the preparation of the 1st call.
    It should be 4C 8B or 4D 8B (mov r8, ...).
    For the patterns, they're +1 from the result since it can be either of those.

    Pattern 1 (up to 22621.2134):
    ```8B ?? 48 8D 55 ??    48 8B ?? E8 ?? ?? ?? ?? 48 8B 08 E8```
    22621.1992: 7463C
    22621.2134: 3B29C

    Pattern 2 (22621.2283+):
    ```8B ?? 48 8D 54 24 ?? 48 8B ?? E8 ?? ?? ?? ?? 48 8B 08 E8```
    22621.2283: 24A1D2

    Step 2:
    In place of the 1st call's call op (E8), overwrite it with a code to set the value of the com_ptr passed into the
    2nd argument (rdx) to 0. This is to skip the cleanup that happens right after the 2nd call.
    ```48 C7 02 00 00 00 00 mov qword ptr [rdx], 0```
    Start from -13 of the byte after 2nd call's end.
    22621.1992: 74646
    22621.2134: 3B2A6
    22621.2283: 24A1DD

    Step 3:
    NOP the rest of the 2nd call.

    Summary:
    ```
       48 8B ?? 48 8D 55 ??    48 8B ?? E8 ?? ?? ?? ?? 48 8B 08 E8 ?? ?? ?? ?? // ~22621.2134
       48 8B ?? 48 8D 54 24 ?? 48 8B ?? E8 ?? ?? ?? ?? 48 8B 08 E8 ?? ?? ?? ?? // 22621.2283~
       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^^^^^^
       1st: TaskViewFrame::UpdateWorkAreaAsync()       2nd: WaitForCompletion()
       48 8B ?? 48 8D 54 24 ?? 48 8B ?? 48 C7 02 00 00 00 00 90 90 90 90 90 90 // Result
       -------------------------------- xxxxxxxxxxxxxxxxxxxx xxxxxxxxxxxxxxxxx
       We need rdx                      Step 2               Step 3
    ```

    Notes:
    - In 22621.1992 and 22621.2134, `~AsyncOperationCompletedHandler()` is inlined, while it is not in 22621.2283. We
      can see `unconditional_release_ref()` calls right in `RuntimeClassInitialize()` of 1992 and 2134.
    - In 22621.2134, there is `33 FF xor edi, edi` before the jz for the inlined cleanup. The value of edi is used in
      two more cleanup calls after our area of interest (those covered by twoCallsLength), therefore we can't just NOP
      everything. And I think detecting such things is too much work.
    ***/

    int twoCallsLength = 1 + 18 + 4; // 4C/4D + pattern length + 4 bytes for the 2nd call's call address
    PBYTE step1 = FindPattern(mi->lpBaseOfDll, mi->SizeOfImage, "\x8B\x00\x48\x8D\x55\x00\x48\x8B\x00\xE8\x00\x00\x00\x00\x48\x8B\x08\xE8", "x?xxx?xx?x????xxxx");
    if (!step1)
    {
        twoCallsLength += 1; // Add 1 to the pattern length
        step1 = FindPattern(mi->lpBaseOfDll, mi->SizeOfImage, "\x8B\x00\x48\x8D\x54\x24\x00\x48\x8B\x00\xE8\x00\x00\x00\x00\x48\x8B\x08\xE8", "x?xxxx?xx?x????xxxx");
        if (!step1) return FALSE;
    }
    step1 -= 1; // Point to the 4C/4D
    printf("[TaskViewFrame::RuntimeClassInitialize()] step1 = %lX\n", step1 - (PBYTE)mi->lpBaseOfDll);

    PBYTE step2 = step1 + twoCallsLength - 13;
    printf("[TaskViewFrame::RuntimeClassInitialize()] step2 = %lX\n", step2 - (PBYTE)mi->lpBaseOfDll);

    PBYTE step3 = step2 + 7;

    // Execution
    DWORD dwOldProtect = 0;
    if (!VirtualProtect(step1, twoCallsLength, PAGE_EXECUTE_READWRITE, &dwOldProtect)) return FALSE;
    const BYTE step2Payload[] = { 0x48, 0xC7, 0x02, 0x00, 0x00, 0x00, 0x00 };
    memcpy(step2, step2Payload, sizeof(step2Payload));
    memset(step3, 0x90, twoCallsLength - (step3 - step1));
    VirtualProtect(step1, twoCallsLength, dwOldProtect, &dwOldProtect);

    printf("[TaskViewFrame::RuntimeClassInitialize()] Patched!\n");
    return TRUE;
}

DEFINE_GUID(SID_EdgeUi,
    0x0d189b30,
    0x0f12b, 0x4b13, 0x94, 0xcf,
    0x53, 0xcb, 0x0e, 0x0e, 0x24, 0x0d
);

DEFINE_GUID(IID_IEdgeUiManager,
    0x6e6c3c52,
    0x5a5e, 0x4b4b, 0xa0, 0xf8,
    0x7f, 0xe1, 0x26, 0x21, 0xa9, 0x3e
);

// Reimplementation of HardwareConfirmatorHost::GetDisplayRect()
void WINAPI HardwareConfirmatorShellcode(PBYTE pCoroInstance)
{
    PBYTE pHardwareConfirmatorHost = *(PBYTE*)(pCoroInstance + g_Moment2PatchOffsets.coroInstance_pHardwareConfirmatorHost);

    RECT rc;
    HMONITOR hMonitor = MonitorFromRect(&rc, MONITOR_DEFAULTTOPRIMARY);

    HRESULT hr = S_OK;
    IUnknown* pImmersiveShell = NULL;
    hr = CoCreateInstance(
        &CLSID_ImmersiveShell,
        NULL,
        CLSCTX_LOCAL_SERVER,
        &IID_IServiceProvider,
        &pImmersiveShell
    );
    if (SUCCEEDED(hr))
    {
        IImmersiveMonitorService* pMonitorService = NULL;
        IUnknown_QueryService(
            pImmersiveShell,
            &SID_IImmersiveMonitorService,
            &IID_IImmersiveMonitorService,
            &pMonitorService
        );
        if (pMonitorService)
        {
            IUnknown* pEdgeUiManager = NULL;
            pMonitorService->lpVtbl->QueryService(
                pMonitorService,
                hMonitor,
                &SID_EdgeUi,
                &IID_IEdgeUiManager,
                &pEdgeUiManager
            );
            if (pEdgeUiManager)
            {
                if (*(pHardwareConfirmatorHost + g_Moment2PatchOffsets.hardwareConfirmatorHost_bIsInLockScreen))
                {
                    // Lock screen
                    MONITORINFO mi;
                    mi.cbSize = sizeof(MONITORINFO);
                    if (GetMonitorInfoW(hMonitor, &mi))
                        rc = mi.rcMonitor;
                }
                else
                {
                    // Desktop
                    HRESULT(*pTheFunc)(IUnknown*, PRECT) = ((void**)pEdgeUiManager->lpVtbl)[19];
                    hr = pTheFunc(pEdgeUiManager, &rc);
                }

                typedef struct { float x, y, width, height; } Windows_Foundation_Rect;
                Windows_Foundation_Rect* out = pCoroInstance + g_Moment2PatchOffsets.coroInstance_rcOut;
                out->x = (float)rc.left;
                out->y = (float)rc.top;
                out->width = (float)(rc.right - rc.left);
                out->height = (float)(rc.bottom - rc.top);

                pEdgeUiManager->lpVtbl->Release(pEdgeUiManager);
            }
            pMonitorService->lpVtbl->Release(pMonitorService);
        }
        pImmersiveShell->lpVtbl->Release(pImmersiveShell);
    }

    if (FAILED(hr))
        printf("[HardwareConfirmatorShellcode] Failed. 0x%lX\n", hr);
}

BOOL Moment2PatchHardwareConfirmator(LPMODULEINFO mi)
{
    // Find required offsets

    // pHardwareConfirmatorHost and bIsInLockScreen:
    // Find in GetDisplayRectAsync$_ResumeCoro$1, inside `case 4:`
    //
    // 48 8B 83 ED 00 00 00     mov     rax, [rbx+0EDh]
    //          ^^^^^^^^^^^ pHardwareConfirmatorHost
    // 8A 80 EC 00 00 00        mov     al, [rax+0ECh]
    //       ^^^^^^^^^^^ bIsInLockScreen
    //
    // if ( ADJ(this)->pHardwareConfirmatorHost->bIsInLockScreen )
    // if ( *(_BYTE *)(*(_QWORD *)(this + 237) + 236i64) ) // 22621.2283
    //                                    ^ HCH  ^ bIsInLockScreen
    //
    // 22621.2134: 1D55D
    PBYTE match1 = FindPattern(mi->lpBaseOfDll, mi->SizeOfImage, "\x48\x8B\x83\x00\x00\x00\x00\x8A\x80\x00\x00\x00\x00", "xxx????xx????");
    printf("[HardwareConfirmatorHost::GetDisplayRectAsync$_ResumeCoro$1()] match1 = %lX\n", match1 - (PBYTE)mi->lpBaseOfDll);
    if (!match1) return FALSE;
    g_Moment2PatchOffsets.coroInstance_pHardwareConfirmatorHost = *(int*)(match1 + 3);
    g_Moment2PatchOffsets.hardwareConfirmatorHost_bIsInLockScreen = *(int*)(match1 + 9);

    // coroInstance_rcOut:
    // Also in GetDisplayRectAsync$_ResumeCoro$1, through `case 4:`
    // We also use this as the point to jump to, which is the code to set the rect and finish the coroutine.
    //
    // v27 = *(_OWORD *)(this + 16);
    // *(_OWORD *)(this - 16) = v27;
    // if ( winrt_suspend_handler ) ...
    //
    // 0F 10 43 10              movups  xmm0, xmmword ptr [rbx+10h]
    //          ^^ coroInstance_rcOut
    // 0F 11 84 24 D0 00 00 00  movups  [rsp+158h+var_88], xmm0
    //
    // 22621.2134: 1D624
    PBYTE match2 = FindPattern(mi->lpBaseOfDll, mi->SizeOfImage, "\x0F\x10\x43\x00\x0F\x11\x84\x24", "xxx?xxxx");
    printf("[HardwareConfirmatorHost::GetDisplayRectAsync$_ResumeCoro$1()] match2 = %lX\n", match2 - (PBYTE)mi->lpBaseOfDll);
    if (!match2) return FALSE;
    g_Moment2PatchOffsets.coroInstance_rcOut = *(match2 + 3);

    // Find where to put the shellcode
    // We'll overwrite from this position:
    //
    // *(_OWORD *)(this + 32) = 0i64;
    // *(_QWORD *)(this + 48) = MonitorFromRect((LPCRECT)(this + 32), 1u);
    //
    // 22621.2134: 1D21E
    PBYTE writeAt = FindPattern(mi->lpBaseOfDll, mi->SizeOfImage, "\x48\x8D\x4B\x00\x0F", "xxx?x");
    if (!writeAt) return FALSE;
    printf("[HardwareConfirmatorHost::GetDisplayRectAsync$_ResumeCoro$1()] writeAt = %lX\n", writeAt - (PBYTE)mi->lpBaseOfDll);

    // In 22621.2134+, after our jump location there is a cleanup for something we skipped. NOP them.
    // From match2, bytes +17 until +37, which is 21 bytes to be NOP'd.
    // 22621.2134: 1D635-1D64A
    PBYTE cleanupBegin = NULL, cleanupEnd = NULL;
    if (IsWindows11Version22H2Build2134OrHigher())
    {
        cleanupBegin = match2 + 17;
        cleanupEnd = match2 + 38; // Exclusive
        printf("[HardwareConfirmatorHost::GetDisplayRectAsync$_ResumeCoro$1()] cleanup = %lX-%lX\n", cleanupBegin - (PBYTE)mi->lpBaseOfDll, cleanupEnd - (PBYTE)mi->lpBaseOfDll);
        if (*cleanupBegin != 0x49 || *cleanupEnd != 0x90 /*Already NOP here*/) return FALSE;
    }

    // Craft the shellcode
    BYTE shellcode[] = {
        // lea rcx, [rbx+0] ; rbx is the `this` which is the instance of the coro, we pass it to our function
        0x48, 0x8D, 0x0B,
        // mov rax, 1111111111111111h ; placeholder for the address of HardwareConfirmatorShellcode
        0x48, 0xB8, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        // call rax
        0xFF, 0xD0
    };

    uintptr_t pattern = 0x1111111111111111;
    *(PBYTE*)(memmem(shellcode, sizeof(shellcode), &pattern, sizeof(uintptr_t))) = HardwareConfirmatorShellcode;

    // Execution
    DWORD dwOldProtect = 0;
    SIZE_T totalSize = sizeof(shellcode) + 5;
    if (!VirtualProtect(writeAt, totalSize, PAGE_EXECUTE_READWRITE, &dwOldProtect)) return FALSE;
    memcpy(writeAt, shellcode, sizeof(shellcode));
    PBYTE jmpLoc = writeAt + sizeof(shellcode);
    jmpLoc[0] = 0xE9;
    *(DWORD*)(jmpLoc + 1) = (DWORD)(match2 - jmpLoc - 5);
    VirtualProtect(writeAt, totalSize, dwOldProtect, &dwOldProtect);

    if (cleanupBegin)
    {
        dwOldProtect = 0;
        if (!VirtualProtect(cleanupBegin, cleanupEnd - cleanupBegin, PAGE_EXECUTE_READWRITE, &dwOldProtect)) return FALSE;
        memset(cleanupBegin, 0x90, cleanupEnd - cleanupBegin);
        VirtualProtect(cleanupBegin, cleanupEnd - cleanupBegin, dwOldProtect, &dwOldProtect);
    }

    printf("[HardwareConfirmatorHost::GetDisplayRectAsync$_ResumeCoro$1()] Patched!\n");
}
#endif

BOOL IsDebuggerPresentHook()
{
    return FALSE;
}

static BOOL PeopleBand_IsOS(DWORD dwOS)
{
    if (dwOS == OS_ANYSERVER)
        return FALSE;
    return IsOS(dwOS);
}

static BOOL explorer_IsOS(DWORD dwOS)
{
    if (dwOS == OS_ANYSERVER) {
        unsigned char buf[16];
        memcpy(buf, (uintptr_t)_ReturnAddress() - 16, 16);
        // check if call is made from explorer!PeopleBarPolicy::IsPeopleBarDisabled
        if (buf[0] == 0x48 && buf[1] == 0x83 && buf[2] == 0xEC &&
            buf[3] == 0x28 && buf[4] == 0xB9 && buf[5] == 0x1D &&
            buf[6] == 0x00 && buf[7] == 0x00 && buf[8] == 0x00) // sub rsp, 28h; mov ecx, 1dh
        {
            buf[0] = 0x48;
            buf[1] = 0x31;
            buf[2] = 0xC0;
            buf[3] = 0xC3; // xor rax, rax; ret
            DWORD flOldProtect = 0;
            if (VirtualProtect((uintptr_t)_ReturnAddress() - 16, 4, PAGE_EXECUTE_READWRITE, &flOldProtect)) {
                memcpy((uintptr_t)_ReturnAddress() - 16, buf, 4);
                VirtualProtect((uintptr_t)_ReturnAddress() - 16, 4, flOldProtect, &flOldProtect);
                VnPatchIAT(GetModuleHandleW(NULL), "api-ms-win-shcore-sysinfo-l1-1-0.dll", "IsOS", IsOS);
                return FALSE;
            }
        }
    }
    return IsOS(dwOS);
}


/**
 * \brief Main injection routine.
 * \return Should not return on error. Returns 0 for success.
 */
static DWORD
Inject(BOOL bIsExplorer)
{
    HANDLE hThread;
    int    rv;

#if defined(DEBUG) || defined(_DEBUG)
    ExplorerPatcher_OpenConsoleWindow();
#endif

    if (bIsExplorer) {
#ifdef _WIN64
        InitializeCriticalSection(&lock_epw);
#endif
    }

    LoadSettings(MAKELPARAM(bIsExplorer, FALSE));
    Explorer_RefreshUI(99);

#ifdef _WIN64
    if (bIsExplorer) {
        funchook = funchook_create();
        printf("funchook create %d\n", funchook != 0);
    }
#endif

    if (bIsExplorer) {
        hSwsSettingsChanged     = CreateEventW(NULL, FALSE, FALSE, NULL);
        hSwsOpacityMaybeChanged = CreateEventW(NULL, FALSE, FALSE, NULL);
    }

    unsigned numSettings = bIsExplorer ? 12 : 2;
    Setting *settings    = calloc(numSettings, sizeof(Setting));
    if (settings) {
        unsigned cs = 0;

        if (cs < numSettings) {
            settings[cs].callback = NULL;
            settings[cs].data     = NULL;
            settings[cs].hEvent   = CreateEventW(NULL, FALSE, FALSE, NULL);
            settings[cs].hKey     = NULL;
            ZeroMemory(settings[cs].name, MAX_PATH);
            settings[cs].origin = NULL;
            cs++;
        }

        if (cs < numSettings) {
            settings[cs].callback = LoadSettings;
            settings[cs].data     = MAKELPARAM(bIsExplorer, TRUE);
            settings[cs].hEvent   = NULL;
            settings[cs].hKey     = NULL;
            wcscpy_s(settings[cs].name, MAX_PATH, L"" REGPATH);
            settings[cs].origin = HKEY_CURRENT_USER;
            cs++;
        }

        if (cs < numSettings) {
            settings[cs].callback = LoadSettings;
            settings[cs].data     = MAKELPARAM(bIsExplorer, FALSE);
            settings[cs].hEvent   = NULL;
            settings[cs].hKey     = NULL;
            wcscpy_s(settings[cs].name, MAX_PATH, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartPage");
            settings[cs].origin = HKEY_CURRENT_USER;
            cs++;
        }

        if (cs < numSettings) {
            settings[cs].callback = SetEvent;
            settings[cs].data     = hSwsSettingsChanged;
            settings[cs].hEvent   = NULL;
            settings[cs].hKey     = NULL;
            wcscpy_s(settings[cs].name, MAX_PATH, REGPATH L"\\sws");
            settings[cs].origin = HKEY_CURRENT_USER;
            cs++;
        }

        if (cs < numSettings) {
            settings[cs].callback = SetEvent;
            settings[cs].data     = hSwsOpacityMaybeChanged;
            settings[cs].hEvent   = NULL;
            settings[cs].hKey     = NULL;
            wcscpy_s(settings[cs].name, MAX_PATH,
                     L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MultitaskingView\\AltTabViewHost");
            settings[cs].origin = HKEY_CURRENT_USER;
            cs++;
        }

        if (cs < numSettings) {
            settings[cs].callback = Explorer_RefreshUI;
            settings[cs].data     = 1;
            settings[cs].hEvent   = NULL;
            settings[cs].hKey     = NULL;
            wcscpy_s(settings[cs].name, MAX_PATH, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
            settings[cs].origin = HKEY_CURRENT_USER;
            cs++;
        }

        if (cs < numSettings) {
            settings[cs].callback = Explorer_RefreshUI;
            settings[cs].data     = 2;
            settings[cs].hEvent   = NULL;
            settings[cs].hKey     = NULL;
            wcscpy_s(settings[cs].name, MAX_PATH, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search");
            settings[cs].origin = HKEY_CURRENT_USER;
            cs++;
        }

        if (cs < numSettings) {
            settings[cs].callback = Explorer_RefreshUI;
            settings[cs].data     = NULL;
            settings[cs].hEvent   = NULL;
            settings[cs].hKey     = NULL;
            wcscpy_s(settings[cs].name, MAX_PATH, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\People");
            settings[cs].origin = HKEY_CURRENT_USER;
            cs++;
        }

        if (cs < numSettings) {
            settings[cs].callback = Explorer_RefreshUI;
            settings[cs].data     = NULL;
            settings[cs].hEvent   = NULL;
            settings[cs].hKey     = NULL;
            wcscpy_s(settings[cs].name, MAX_PATH, L"SOFTWARE\\Microsoft\\TabletTip\\1.7");
            settings[cs].origin = HKEY_CURRENT_USER;
            cs++;
        }

        if (cs < numSettings) {
            settings[cs].callback = SetEvent;
            settings[cs].data     = hSwsSettingsChanged;
            settings[cs].hEvent   = NULL;
            settings[cs].hKey     = NULL;
            wcscpy_s(settings[cs].name, MAX_PATH, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer");
            settings[cs].origin = HKEY_CURRENT_USER;
            cs++;
        }

        if (cs < numSettings) {
            settings[cs].callback = UpdateStartMenuPositioning;
            settings[cs].data     = MAKELPARAM(FALSE, TRUE);
            settings[cs].hEvent   = NULL;
            settings[cs].hKey     = NULL;
            wcscpy_s(settings[cs].name, MAX_PATH, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
            settings[cs].origin = HKEY_CURRENT_USER;
            cs++;
        }

        if (cs < numSettings) {
            settings[cs].callback = LoadSettings;
            settings[cs].data     = MAKELPARAM(bIsExplorer, FALSE);
            settings[cs].hEvent   = NULL;
            settings[cs].hKey     = NULL;
            wcscpy_s(settings[cs].name, MAX_PATH, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer");
            settings[cs].origin = HKEY_CURRENT_USER;
            cs++;
        }

        SettingsChangeParameters *settingsParams = calloc(1, sizeof(SettingsChangeParameters));
        if (settingsParams) {
            settingsParams->settings = settings;
            settingsParams->size     = numSettings;
            settingsParams->hThread  = CreateThread(0, 0, MonitorSettings, settingsParams, 0, 0);
        } else {
            if (numSettings && settings[0].hEvent)
                CloseHandle(settings[0].hEvent);
            free(settings);
            settings = NULL;
        }
    }

    InjectBasicFunctions(bIsExplorer, TRUE);
    // if (!hDelayedInjectionThread)
    //{
    //     hDelayedInjectionThread = CreateThread(0, 0, InjectBasicFunctions, 0, 0, 0);
    // }

    if (!bIsExplorer)
        return 0;

#ifdef _WIN64
    wprintf(L"Running on Windows %d, OS Build %d.%d.%d.%d.\n", IsWindows11() ? 11 : 10,
            global_rovi.dwMajorVersion, global_rovi.dwMinorVersion, global_rovi.dwBuildNumber, global_ubr);
#endif

    WCHAR wszPath[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, wszPath);
    wcscat_s(wszPath, MAX_PATH, L"" APP_RELATIVE_PATH);
    if (!PathFileExistsW(wszPath))
        CreateDirectoryW(wszPath, NULL);

#ifdef _WIN64
    static const size_t killSwitchSize = _countof(L"" EP_Weather_Killswitch) - 1;
    ep_generate_random_wide_string(wszEPWeatherKillswitch, _countof(wszEPWeatherKillswitch) - 1);
    memcpy(wszEPWeatherKillswitch, L"" EP_Weather_Killswitch, killSwitchSize * sizeof(wchar_t));
    wszEPWeatherKillswitch[killSwitchSize] = L'_';
    hEPWeatherKillswitch = CreateMutexW(NULL, TRUE, wszEPWeatherKillswitch);
# if 0
    wprintf(L"%s\n", wszEPWeatherKillswitch);
    while (TRUE)
    {
        hEPWeatherKillswitch = CreateMutexW(NULL, TRUE, wszEPWeatherKillswitch);
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            WaitForSingleObject(hEPWeatherKillswitch, INFINITE);
            CloseHandle(hEPWeatherKillswitch);
        } else {
            break;
        }
    }
# endif
#endif


#ifdef _WIN64
    hCanStartSws            = CreateEventW(NULL, FALSE, FALSE, NULL);
    hWin11AltTabInitialized = CreateEventW(NULL, FALSE, FALSE, NULL);
    hThread                 = CreateThread(NULL, 0, WindowSwitcher, NULL, 0, NULL);
    CloseHandle(hThread);

# ifdef USE_PRIVATE_INTERFACES
    P_Icon_Dark_Search =
        ReadFromFile(L"C:\\Users\\root\\Downloads\\pri\\resources\\Search_Dark\\png\\32.png", &S_Icon_Dark_Search);
    P_Icon_Light_Search =
        ReadFromFile(L"C:\\Users\\root\\Downloads\\pri\\resources\\Search_Light\\png\\32.png", &S_Icon_Light_Search);
    P_Icon_Dark_TaskView =
        ReadFromFile(L"C:\\Users\\root\\Downloads\\pri\\resources\\TaskView_Dark\\png\\32.png", &S_Icon_Dark_TaskView);
    P_Icon_Light_TaskView = ReadFromFile(
        L"C:\\Users\\root\\Downloads\\pri\\resources\\TaskView_Light\\png\\32.png", &S_Icon_Light_TaskView
    );
    P_Icon_Dark_Widgets =
        ReadFromFile(L"C:\\Users\\root\\Downloads\\pri\\resources\\Widgets_Dark\\png\\32.png", &S_Icon_Dark_Widgets);
    P_Icon_Light_Widgets =
        ReadFromFile(L"C:\\Users\\root\\Downloads\\pri\\resources\\Widgets_Light\\png\\32.png", &S_Icon_Dark_Widgets);
# endif

    symbols_addr symbols_PTRS;
    ZeroMemory(&symbols_PTRS, sizeof symbols_PTRS);
    if (LoadSymbols(&symbols_PTRS, hModule)) {
        if (bEnableSymbolDownload) {
            printf("Attempting to download symbol data; for now, the program may have limited functionality.\n");
            DownloadSymbolsParams *params = malloc(sizeof(DownloadSymbolsParams));
            params->hModule  = hModule;
            params->bVerbose = FALSE;
            hThread = CreateThread(NULL, 0, DownloadSymbols, params, 0, NULL);
            CloseHandle(hThread);
        }
    } else {
        printf("Loaded symbols\n");
    }

    RtlQueryFeatureConfigurationFunc = GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlQueryFeatureConfiguration");
    if (RtlQueryFeatureConfigurationFunc) {
        rv = funchook_prepare(funchook, (void **)&RtlQueryFeatureConfigurationFunc, RtlQueryFeatureConfigurationHook);
        if (rv != 0) {
            FreeLibraryAndExitThread(hModule, rv);
            return FALSE;
        }
    }
    printf("Setup ntdll functions done\n");

    HANDLE hUser32                = LoadLibraryW(L"user32.dll");
    CreateWindowInBand            = GetProcAddress(hUser32, "CreateWindowInBand");
    GetWindowBand                 = GetProcAddress(hUser32, "GetWindowBand");
    SetWindowBand                 = GetProcAddress(hUser32, "SetWindowBand");
    SetWindowCompositionAttribute = GetProcAddress(hUser32, "SetWindowCompositionAttribute");
    if (!IsWindows11BuildHigherThan25158())
        VnPatchIAT(hUser32, "win32u.dll", "NtUserFindWindowEx", user32_NtUserFindWindowExHook);
    printf("Setup user32 functions done\n");

    HANDLE hExplorer             = GetModuleHandleW(NULL);
    SetChildWindowNoActivateFunc = GetProcAddress(GetModuleHandleW(L"user32.dll"), (LPCSTR)2005);
    if (bOldTaskbar) {
        VnPatchIAT(hExplorer, "user32.dll", (LPCSTR)2005, explorer_SetChildWindowNoActivateHook);
        VnPatchDelayIAT(hExplorer, "ext-ms-win-rtcore-ntuser-window-ext-l1-1-0.dll", "SendMessageW", explorer_SendMessageW);
        VnPatchIAT(hExplorer, "api-ms-win-core-libraryloader-l1-2-0.dll", "GetProcAddress", explorer_GetProcAddressHook);
        VnPatchIAT(hExplorer, "shell32.dll", "ShellExecuteW", explorer_ShellExecuteW);
        VnPatchIAT(hExplorer, "shell32.dll", "ShellExecuteExW", explorer_ShellExecuteExW);
        VnPatchIAT(hExplorer, "API-MS-WIN-CORE-REGISTRY-L1-1-0.DLL", "RegGetValueW", explorer_RegGetValueW);
        VnPatchIAT(hExplorer, "API-MS-WIN-CORE-REGISTRY-L1-1-0.DLL", "RegSetValueExW", explorer_RegSetValueExW);
        VnPatchIAT(hExplorer, "API-MS-WIN-CORE-REGISTRY-L1-1-0.DLL", "RegCreateKeyExW", explorer_RegCreateKeyExW);
        VnPatchIAT(hExplorer, "API-MS-WIN-SHCORE-REGISTRY-L1-1-0.DLL", "SHGetValueW", explorer_SHGetValueW);
        VnPatchIAT(hExplorer, "user32.dll", "LoadMenuW", explorer_LoadMenuW);
        VnPatchIAT(hExplorer, "api-ms-win-core-shlwapi-obsolete-l1-1-0.dll", "QISearch", explorer_QISearch);

        if (IsOS(OS_ANYSERVER))
            VnPatchIAT(hExplorer, "api-ms-win-shcore-sysinfo-l1-1-0.dll", "IsOS", explorer_IsOS);
        if (IsWindows11Version22H2OrHigher())
            VnPatchDelayIAT(hExplorer, "ext-ms-win-rtcore-ntuser-window-ext-l1-1-0.dll", "CreateWindowExW", Windows11v22H2_explorer_CreateWindowExW);
    }

    VnPatchIAT(hExplorer, "user32.dll", "TileWindows", explorer_TileWindows);
    VnPatchIAT(hExplorer, "user32.dll", "CascadeWindows", explorer_CascadeWindows);
    VnPatchIAT(hExplorer, "API-MS-WIN-CORE-REGISTRY-L1-1-0.DLL", "RegOpenKeyExW", explorer_RegOpenKeyExW);
    VnPatchIAT(hExplorer, "shell32.dll", (LPCSTR)85, explorer_OpenRegStream);
    VnPatchIAT(hExplorer, "user32.dll", "TrackPopupMenuEx", explorer_TrackPopupMenuExHook);
    VnPatchIAT(hExplorer, "uxtheme.dll", "OpenThemeDataForDpi", explorer_OpenThemeDataForDpi);
    VnPatchIAT(hExplorer, "uxtheme.dll", "DrawThemeBackground", explorer_DrawThemeBackground);
    VnPatchIAT(hExplorer, "uxtheme.dll", "CloseThemeData", explorer_CloseThemeData);

    // Fix Windows 10 taskbar high DPI button width bug
    if (IsWindows11())
        VnPatchIAT(hExplorer, "api-ms-win-ntuser-sysparams-l1-1-0.dll", "GetSystemMetrics", patched_GetSystemMetrics);
    // VnPatchIAT(hExplorer, "api-ms-win-core-libraryloader-l1-2-0.dll", "LoadStringW", explorer_LoadStringWHook);

    if (bClassicThemeMitigations) {
# if 0
        explorer_SetWindowThemeFunc = SetWindowTheme;
        rv = funchook_prepare(
            funchook,
            (void**)&explorer_SetWindowThemeFunc,
            explorer_SetWindowThemeHook
        );
        if (rv != 0)
        {
            FreeLibraryAndExitThread(hModule, rv);
            return rv;
        }
# endif
        VnPatchIAT(hExplorer, "uxtheme.dll", "DrawThemeTextEx", explorer_DrawThemeTextEx);
        VnPatchIAT(hExplorer, "uxtheme.dll", "GetThemeMargins", explorer_GetThemeMargins);
        VnPatchIAT(hExplorer, "uxtheme.dll", "GetThemeMetric", explorer_GetThemeMetric);
        // VnPatchIAT(hExplorer, "uxtheme.dll", "OpenThemeDataForDpi", explorer_OpenThemeDataForDpi);
        // VnPatchIAT(hExplorer, "uxtheme.dll", "DrawThemeBackground", explorer_DrawThemeBackground);
        VnPatchIAT(hExplorer, "user32.dll", "SetWindowCompositionAttribute", explorer_SetWindowCompositionAttribute);
    }
    // VnPatchDelayIAT(hExplorer, "ext-ms-win-rtcore-ntuser-window-ext-l1-1-0.dll", "CreateWindowExW", explorer_CreateWindowExW);

    if (bOldTaskbar && dwIMEStyle)
        VnPatchIAT(hExplorer, "api-ms-win-core-com-l1-1-0.dll", "CoCreateInstance", explorer_CoCreateInstanceHook);
    if (bOldTaskbar)
        VnPatchIAT(hExplorer, "API-MS-WIN-NTUSER-RECTANGLE-L1-1-0.DLL", "SetRect", explorer_SetRect);
    if (bOldTaskbar)
        VnPatchIAT(hExplorer, "USER32.DLL", "DeleteMenu", explorer_DeleteMenu);
    if (bOldTaskbar && global_rovi.dwBuildNumber >= 22572) {
        VnPatchIAT(hExplorer, "dwmapi.dll", "DwmUpdateThumbnailProperties", explorer_DwmUpdateThumbnailPropertiesHook);
        PatchExplorer_UpdateWindowAccentProperties();
    }


    HANDLE hShcore  = LoadLibraryW(L"shcore.dll");
    SHWindowsPolicy = GetProcAddress(hShcore, (LPCSTR)190);
# ifdef USE_PRIVATE_INTERFACES
    explorer_SHCreateStreamOnModuleResourceWFunc = GetProcAddress(hShcore, (LPCSTR)109);
    VnPatchIAT(hExplorer, "shcore.dll", (LPCSTR)0x6D, explorer_SHCreateStreamOnModuleResourceWHook);
# endif

    printf("Setup explorer functions done\n");

    CreateWindowExWFunc = CreateWindowExW;
    rv = funchook_prepare(funchook, (void **)&CreateWindowExWFunc, CreateWindowExWHook);
    if (rv != 0) {
        FreeLibraryAndExitThread(hModule, rv);
        return rv;
    }
    SetWindowLongPtrWFunc = SetWindowLongPtrW;
    rv = funchook_prepare(funchook, (void **)&SetWindowLongPtrWFunc, SetWindowLongPtrWHook);
    if (rv != 0) {
        FreeLibraryAndExitThread(hModule, rv);
        return rv;
    }

    HANDLE hUxtheme                 = LoadLibraryW(L"uxtheme.dll");
    SetPreferredAppMode             = GetProcAddress(hUxtheme, (LPCSTR)0x87);
    AllowDarkModeForWindow          = GetProcAddress(hUxtheme, (LPCSTR)0x85);
    ShouldAppsUseDarkMode           = GetProcAddress(hUxtheme, (LPCSTR)0x84);
    ShouldSystemUseDarkMode         = GetProcAddress(hUxtheme, (LPCSTR)0x8A);
    GetThemeName                    = GetProcAddress(hUxtheme, (LPCSTR)0x4A);
    PeopleBand_DrawTextWithGlowFunc = GetProcAddress(hUxtheme, (LPCSTR)0x7E);
    if (bOldTaskbar)
        VnPatchIAT(hExplorer, "uxtheme.dll", (LPCSTR)0x7E, PeopleBand_DrawTextWithGlowHook);
    // DwmExtendFrameIntoClientArea hooked in LoadSettings
    printf("Setup uxtheme functions done\n");

    HANDLE hTwinuiPcshell = LoadLibraryW(L"twinui.pcshell.dll");

# define VALID(n) (symbols_PTRS.twinui_pcshell_PTRS[n] && symbols_PTRS.twinui_pcshell_PTRS[n] != 0xFFFFFFFFu)
# define GETSYMB(n) ((uintptr_t)hTwinuiPcshell + symbols_PTRS.twinui_pcshell_PTRS[n])

    if (VALID(0))
        CImmersiveContextMenuOwnerDrawHelper_s_ContextMenuWndProcFunc = (INT64(*)(HWND,INT,HWND,INT,BOOL*))GETSYMB(0);
    if (VALID(1))
        CLauncherTipContextMenu_GetMenuItemsAsyncFunc                 = (INT64(*)(LPVOID,PVOID,PVOID*))GETSYMB(1);
    if (VALID(2))
        ImmersiveContextMenuHelper_ApplyOwnerDrawToMenuFunc           = (INT64(*)(HMENU,HMENU,HWND,UINT,PVOID))GETSYMB(2);
    if (VALID(3))
        ImmersiveContextMenuHelper_RemoveOwnerDrawFromMenuFunc        = (void(*)(HMENU,HMENU,HWND))GETSYMB(3);
    if (VALID(4))
        CLauncherTipContextMenu_ExecuteShutdownCommandFunc            = (void(*)(PVOID,PVOID))GETSYMB(4);
    if (VALID(5))
        CLauncherTipContextMenu_ExecuteCommandFunc                    = (void(*)(PVOID,INT))GETSYMB(5);
    if (VALID(6)) {
        CLauncherTipContextMenu_ShowLauncherTipContextMenuFunc        = (INT64(*)(PVOID,PPOINT))GETSYMB(6);

        rv = funchook_prepare(funchook, (void **)&CLauncherTipContextMenu_ShowLauncherTipContextMenuFunc,
                              CLauncherTipContextMenu_ShowLauncherTipContextMenuHook);
        if (rv != 0) {
            printf("Funchook error number %d\n", rv);
            abort();
            // BUG This causes a deadlock.
            FreeLibraryAndExitThread(hModule, rv);
        }
    }
    if (VALID(7)) {
        if (IsWindows11Version22H2OrHigher()) {
            twinui_pcshell_CMultitaskingViewManager__CreateDCompMTVHostFunc = (INT64(*)(INT64,UINT,INT64,INT64,PINT64))GETSYMB(TWINUI_PCSHELL_SB_CNT-1);
            twinui_pcshell_CMultitaskingViewManager__CreateXamlMTVHostFunc  = (INT64(*)(INT64,UINT,INT64,INT64,PINT64))GETSYMB(7);

            rv = funchook_prepare(funchook, (void **)&twinui_pcshell_CMultitaskingViewManager__CreateXamlMTVHostFunc,
                                  twinui_pcshell_CMultitaskingViewManager__CreateXamlMTVHostHook);
            if (rv != 0) {
                FreeLibraryAndExitThread(hModule, rv);
                return rv;
            }
        } else {
            twinui_pcshell_IsUndockedAssetAvailableFunc = (INT64(*)(PVOID,PPOINT))GETSYMB(7);
            rv = funchook_prepare(funchook, (void **)&twinui_pcshell_IsUndockedAssetAvailableFunc,
                                  twinui_pcshell_IsUndockedAssetAvailableHook);
            if (rv != 0) {
                FreeLibraryAndExitThread(hModule, rv);
                return rv;
            }
        }
    }

# undef VALID
# undef GETSYMB

# if 0
    if (symbols_PTRS.twinui_pcshell_PTRS[TWINUI_PCSHELL_SB_CNT - 1] &&
    symbols_PTRS.twinui_pcshell_PTRS[TWINUI_PCSHELL_SB_CNT - 1] != 0xFFFFFFFF)
    {
        winrt_Windows_Internal_Shell_implementation_MeetAndChatManager_OnMessageFunc = (INT64(*)(void*,
    POINT*))
            ((uintptr_t)hTwinuiPcshell + symbols_PTRS.twinui_pcshell_PTRS[TWINUI_PCSHELL_SB_CNT - 1]);
        rv = funchook_prepare(
            funchook,
            (void**)&winrt_Windows_Internal_Shell_implementation_MeetAndChatManager_OnMessageFunc,
            winrt_Windows_Internal_Shell_implementation_MeetAndChatManager_OnMessageHook
        );
        if (rv != 0)
        {
            FreeLibraryAndExitThread(hModule, rv);
            return rv;
        }
    }*/

#if _WIN64
#if USE_MOMENT_3_FIXES_ON_MOMENT_2
    // Use this only for testing, since the RtlQueryFeatureConfiguration() hook is perfect.
    // Only tested on 22621.1992.
    BOOL bPerformMoment2Patches = IsWindows11Version22H2Build1413OrHigher();
#else
    // This is the only way to fix stuff since the flag "26008830" and the code when it's not enabled are gone.
    // Tested on 22621.2134, 22621.2283, and 22621.2359 (RP).
    BOOL bPerformMoment2Patches = IsWindows11Version22H2Build2134OrHigher();
#endif
    bPerformMoment2Patches &= global_rovi.dwBuildNumber == 22621 && bOldTaskbar;
    if (bPerformMoment2Patches) // TODO Test for 23H2
    {
        MODULEINFO miTwinuiPcshell;
        GetModuleInformation(GetCurrentProcess(), hTwinuiPcshell, &miTwinuiPcshell, sizeof(MODULEINFO));

        // Fix flyout placement: Our goal with these patches is to get `mi.rcWork` assigned
        Moment2PatchActionCenter(&miTwinuiPcshell);
        Moment2PatchControlCenter(&miTwinuiPcshell);
        Moment2PatchToastCenter(&miTwinuiPcshell);

        // Fix task view
        Moment2PatchTaskView(&miTwinuiPcshell);

        // Fix volume and brightness popups
        HANDLE hHardwareConfirmator = LoadLibraryW(L"Windows.Internal.HardwareConfirmator.dll");
        MODULEINFO miHardwareConfirmator;
        GetModuleInformation(GetCurrentProcess(), hHardwareConfirmator, &miHardwareConfirmator, sizeof(MODULEINFO));
        Moment2PatchHardwareConfirmator(&miHardwareConfirmator);
    }
#endif

    VnPatchIAT(hTwinuiPcshell, "API-MS-WIN-CORE-REGISTRY-L1-1-0.DLL", "RegGetValueW", twinuipcshell_RegGetValueW);
    // VnPatchIAT(hTwinuiPcshell, "api-ms-win-core-debug-l1-1-0.dll", "IsDebuggerPresent",
    // IsDebuggerPresentHook);
    printf("Setup twinui.pcshell functions done\n");


    if (IsWindows11Version22H2OrHigher()) {
        HANDLE hCombase = LoadLibraryW(L"combase.dll");
        // Fixed a bug that crashed Explorer when a folder window was opened after a first one was closed on
        // OS builds 22621+
        VnPatchIAT(hCombase, "api-ms-win-core-libraryloader-l1-2-0.dll", "LoadLibraryExW", Windows11v22H2_combase_LoadLibraryExW);
        printf("Setup combase functions done\n");
    }


    HANDLE hTwinui = LoadLibraryW(L"twinui.dll");
    if (!IsWindows11())
        VnPatchIAT(hTwinui, "user32.dll", "TrackPopupMenu", twinui_TrackPopupMenuHook);
    if (bDisableWinFHotkey)
        VnPatchIAT(hTwinui, "user32.dll", "RegisterHotKey", twinui_RegisterHotkeyHook);
    printf("Setup twinui functions done\n");


    HANDLE hStobject = LoadLibraryW(L"stobject.dll");
    VnPatchIAT(hStobject, "api-ms-win-core-registry-l1-1-0.dll", "RegGetValueW", stobject_RegGetValueW);
    VnPatchIAT(hStobject, "api-ms-win-core-com-l1-1-0.dll", "CoCreateInstance", stobject_CoCreateInstanceHook);
    if (IsWindows11()) {
        VnPatchDelayIAT(hStobject, "user32.dll", "TrackPopupMenu",   stobject_TrackPopupMenuHook);
        VnPatchDelayIAT(hStobject, "user32.dll", "TrackPopupMenuEx", stobject_TrackPopupMenuExHook);
    } else {
        VnPatchIAT(hStobject, "user32.dll", "TrackPopupMenu",   stobject_TrackPopupMenuHook);
        VnPatchIAT(hStobject, "user32.dll", "TrackPopupMenuEx", stobject_TrackPopupMenuExHook);
    }
# ifdef USE_PRIVATE_INTERFACES
    if (bSkinIcons)
        VnPatchDelayIAT(hStobject, "user32.dll", "LoadImageW", SystemTray_LoadImageWHook);
# endif
    printf("Setup stobject functions done\n");


    HANDLE hBthprops = LoadLibraryW(L"bthprops.cpl");
    VnPatchIAT(hBthprops, "user32.dll", "TrackPopupMenuEx", bthprops_TrackPopupMenuExHook);
# ifdef USE_PRIVATE_INTERFACES
    if (bSkinIcons)
        VnPatchIAT(hBthprops, "user32.dll", "LoadImageW", SystemTray_LoadImageWHook);
# endif
    printf("Setup bthprops functions done\n");


    HANDLE hPnidui = LoadLibraryW(L"pnidui.dll");
    VnPatchIAT(hPnidui, "api-ms-win-core-com-l1-1-0.dll", "CoCreateInstance", pnidui_CoCreateInstanceHook);
    VnPatchIAT(hPnidui, "user32.dll", "TrackPopupMenu", pnidui_TrackPopupMenuHook);
# ifdef USE_PRIVATE_INTERFACES
    if (bSkinIcons)
        VnPatchIAT(hPnidui, "user32.dll", "LoadImageW", SystemTray_LoadImageWHook);
# endif
    printf("Setup pnidui functions done\n");


# ifdef _WIN64
    if (global_rovi.dwBuildNumber < 22567)
        PatchSndvolsso();
# endif

    hShell32 = GetModuleHandleW(L"shell32.dll");
    if (hShell32) {
        // Patch ribbon to handle redirects to classic CPLs
        HRESULT(*SHELL32_Create_IEnumUICommand)(IUnknown *, int *, int, IUnknown **) = GetProcAddress(hShell32, (LPCSTR)0x2E8);

        if (SHELL32_Create_IEnumUICommand) {
            char WVTASKITEM[80];
            ZeroMemory(WVTASKITEM, 80);
            IUnknown *pEnumUICommand = NULL;
            SHELL32_Create_IEnumUICommand(NULL, WVTASKITEM, 1, &pEnumUICommand);
            if (pEnumUICommand) {
                EnumExplorerCommand *pEnumExplorerCommand = NULL;
                pEnumUICommand->lpVtbl->QueryInterface(pEnumUICommand, &IID_EnumExplorerCommand, &pEnumExplorerCommand);
                pEnumUICommand->lpVtbl->Release(pEnumUICommand);
                if (pEnumExplorerCommand) {
                    UICommand *pUICommand = NULL;
                    pEnumExplorerCommand->lpVtbl->Next(pEnumExplorerCommand, 1, &pUICommand, NULL);
                    pEnumExplorerCommand->lpVtbl->Release(pEnumExplorerCommand);
                    if (pUICommand) {
                        DWORD flOldProtect = 0;
                        if (VirtualProtect(pUICommand->lpVtbl, sizeof(UICommandVtbl), PAGE_EXECUTE_READWRITE, &flOldProtect))
                        {
                            shell32_UICommand_InvokeFunc = pUICommand->lpVtbl->Invoke;
                            pUICommand->lpVtbl->Invoke   = shell32_UICommand_InvokeHook;
                            VirtualProtect(pUICommand->lpVtbl, sizeof(UICommandVtbl), flOldProtect, &flOldProtect);
                        }
                        pUICommand->lpVtbl->Release(pUICommand);
                    }
                }
            }
        }

        // Allow clasic drive groupings in This PC
        HRESULT(*SHELL32_DllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID * ppv) = GetProcAddress(hShell32, "DllGetClassObject");
        if (SHELL32_DllGetClassObject) {
            IClassFactory *pClassFactory = NULL;
            SHELL32_DllGetClassObject(&CLSID_DriveTypeCategorizer, &IID_IClassFactory, &pClassFactory);

            if (pClassFactory) {
                // DllGetClassObject hands out a unique "factory entry" data structure for each type of CLSID,
                // containing a pointer to an IClassFactoryVtbl as well as some other members including the
                // _true_ create instance function that should be called (in this instance,
                // shell32!CDriveTypeCategorizer_CreateInstance). When the IClassFactory::CreateInstance
                // method is called, shell32!ECFCreateInstance will cast the IClassFactory* passed to it back
                // into a factory entry, and then invoke the pfnCreateInstance function defined in that entry
                // directly. Thus, rather than hooking the shared shell32!ECFCreateInstance function found on
                // the IClassFactoryVtbl* shared by all class objects returned by shell32!DllGetClassObject,
                // we get the real CreateInstance function that will be called and hook that instead
                Shell32ClassFactoryEntry *pClassFactoryEntry = (Shell32ClassFactoryEntry *)pClassFactory;

                DWORD flOldProtect = 0;

                if (VirtualProtect(pClassFactoryEntry, sizeof(Shell32ClassFactoryEntry),
                                   PAGE_EXECUTE_READWRITE, &flOldProtect))
                {
                    shell32_DriveTypeCategorizer_CreateInstanceFunc = pClassFactoryEntry->pfnCreateInstance;
                    pClassFactoryEntry->pfnCreateInstance           = shell32_DriveTypeCategorizer_CreateInstanceHook;
                    VirtualProtect(pClassFactoryEntry, sizeof(Shell32ClassFactoryEntry), flOldProtect, &flOldProtect);
                }

                pClassFactory->lpVtbl->Release(pClassFactory);
            }
        }

        // Disable Windows Spotlight icon
        if (DoesOSBuildSupportSpotlight()) {
            VnPatchIAT(hShell32, "API-MS-WIN-CORE-REGISTRY-L1-1-0.DLL", "RegCreateKeyExW", shell32_RegCreateKeyExW);
            VnPatchIAT(hShell32, "API-MS-WIN-CORE-REGISTRY-L1-1-0.DLL", "RegSetValueExW", shell32_RegSetValueExW);
            VnPatchIAT(hShell32, "user32.dll", "DeleteMenu", shell32_DeleteMenu);
        }

        // Fix high DPI wrong (desktop) icon spacing bug
        if (IsWindows11())
            VnPatchIAT(hShell32, "user32.dll", "GetSystemMetrics", patched_GetSystemMetrics);
    }
    printf("Setup shell32 functions done\n");


    HANDLE hExplorerFrame = GetModuleHandleW(L"ExplorerFrame.dll");
    VnPatchIAT(hExplorerFrame, "api-ms-win-core-com-l1-1-0.dll", "CoCreateInstance", ExplorerFrame_CoCreateInstanceHook);
    printf("Setup explorerframe functions done\n");


    HANDLE hWindowsStorage               = LoadLibraryW(L"windows.storage.dll");
    SHELL32_CanDisplayWin8CopyDialogFunc = GetProcAddress(hShell32, "SHELL32_CanDisplayWin8CopyDialog");
    if (SHELL32_CanDisplayWin8CopyDialogFunc)
        VnPatchDelayIAT(hWindowsStorage, "ext-ms-win-shell-exports-internal-l1-1-0.dll", "SHELL32_CanDisplayWin8CopyDialog", SHELL32_CanDisplayWin8CopyDialogHook);
    printf("Setup windows.storage functions done\n");


    if (IsWindows11()) {
        HANDLE hInputSwitch = LoadLibraryW(L"InputSwitch.dll");
        if (bOldTaskbar)
        {
            printf("[IME] Context menu patch status: %d\n", PatchContextMenuOfNewMicrosoftIME(NULL));
        }
        if (hInputSwitch)
        {
            VnPatchIAT(hInputSwitch, "user32.dll", "TrackPopupMenuEx", inputswitch_TrackPopupMenuExHook);
            printf("Setup inputswitch functions done\n");
        }

        HANDLE hWindowsudkShellcommon = LoadLibraryW(L"windowsudk.shellcommon.dll");
        HANDLE hSLC                   = LoadLibraryW(L"slc.dll");
        if (hWindowsudkShellcommon && hSLC) {
            SLGetWindowsInformationDWORDFunc = GetProcAddress(hSLC, "SLGetWindowsInformationDWORD");

            if (SLGetWindowsInformationDWORDFunc)
                VnPatchDelayIAT(hWindowsudkShellcommon, "ext-ms-win-security-slc-l1-1-0.dll", "SLGetWindowsInformationDWORD", windowsudkshellcommon_SLGetWindowsInformationDWORDHook);

            printf("Setup windowsudk.shellcommon functions done\n");
        }
    }


    HANDLE hPeopleBand = LoadLibraryW(L"PeopleBand.dll");
    if (hPeopleBand) {
        if (IsOS(OS_ANYSERVER))
            VnPatchIAT(hPeopleBand, "SHLWAPI.dll", (LPCSTR)437, PeopleBand_IsOS);
        VnPatchIAT(hPeopleBand, "api-ms-win-core-largeinteger-l1-1-0.dll", "MulDiv", PeopleBand_MulDivHook);
        printf("Setup peopleband functions done\n");
    }


    rv = funchook_install(funchook, 0);
    if (rv != 0) {
        FreeLibraryAndExitThread(hModule, rv);
        return rv;
    }
    printf("Installed hooks.\n");

#if 0
    HANDLE hEvent = CreateEventEx(
        0,
        L"ShellDesktopSwitchEvent",
        CREATE_EVENT_MANUAL_RESET,
        EVENT_ALL_ACCESS
    );
    if (GetLastError() != ERROR_ALREADY_EXISTS)
    {
        printf("Created ShellDesktopSwitchEvent event.\n");
        ResetEvent(hEvent);
    }
#endif

    if (bOldTaskbar) {
        if (IsWindows11()) {
            printf("Starting PlayStartupSound thread...\n");
            hThread = CreateThread(NULL, 0, PlayStartupSound, NULL, 0, NULL);
            CloseHandle(hThread);
        }
    }


    if (bOldTaskbar) {
        if (IsWindows11()) {
            hThread = CreateThread(NULL, 0, SignalShellReady, (void *)((uintptr_t)dwExplorerReadyDelay), 0, NULL);
            CloseHandle(hThread);
            printf("Signal shell ready...\n");
        }
    } else {
        hThread = CreateThread(NULL, 0, FixTaskbarAutohide, NULL, 0, NULL);
        CloseHandle(hThread);
        RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarGlomLevel");
        RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"MMTaskbarGlomLevel");
    }

    if (IsWindows11Version22H2OrHigher() && bOldTaskbar) {
        DWORD dwRes  = 1;
        DWORD dwSize = sizeof(DWORD);
        if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Search", L"SearchboxTaskbarMode", RRF_RT_DWORD, NULL, &dwRes, &dwSize) != ERROR_SUCCESS) {
            RegSetKeyValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Search", L"SearchboxTaskbarMode", REG_DWORD, &dwRes, sizeof(DWORD));
        }
    }


# if 0
    if (IsWindows11())
    {
        DWORD dwSize = 0;
        if (SHRegGetValueFromHKCUHKLMFunc && SHRegGetValueFromHKCUHKLMFunc(
            L"Control Panel\\Desktop\\WindowMetrics",
            L"MinWidth",
            SRRF_RT_REG_SZ,
            NULL,
            NULL,
            (LPDWORD)(&dwSize)
        ) != ERROR_SUCCESS)
        {
            RegSetKeyValueW(
                HKEY_CURRENT_USER,
                L"Control Panel\\Desktop\\WindowMetrics",
                L"MinWidth",
                REG_SZ,
                L"38",
                sizeof(L"38")
            );
        }
    }
# endif


    printf("Open Start on monitor thread\n");
    hThread = CreateThread(NULL, 0, OpenStartOnCurentMonitorThread, NULL, 0, NULL);
    CloseHandle(hThread);

    printf("Open EP Service Window thread\n");
    hServiceWindowThread = CreateThread(NULL, 0, EP_ServiceWindowThread, NULL, 0, NULL);


    hServiceWindowThread = CreateThread(
        0,
        0,
        EP_ServiceWindowThread,
        0,
        0,
        0
    );
    printf("EP Service Window thread\n");



    // if (bDisableOfficeHotkeys)
    {
        VnPatchIAT(hExplorer, "user32.dll", "RegisterHotKey", explorer_RegisterHotkeyHook);


    if (bEnableArchivePlugin) {
        ArchiveMenuThreadParams *params = calloc(1, sizeof(ArchiveMenuThreadParams));
        params->CreateWindowInBand = CreateWindowInBand;
        params->hWnd               = &hArchivehWnd;
        params->wndProc            = CLauncherTipContextMenu_WndProc;
        hThread = CreateThread(NULL, 0, &ArchiveMenuThread, params, 0, NULL);
        CloseHandle(hThread);
    }

    hThread = CreateThread(NULL, 0, CheckForUpdatesThread, (void *)5000ULL, 0, NULL);
    CloseHandle(hThread);

# if 0
    WCHAR wszExtraLibPath[MAX_PATH];
    if (GetWindowsDirectoryW(wszExtraLibPath, MAX_PATH)) {
        wcscat_s(wszExtraLibPath, MAX_PATH, L"\\ep_extra.dll");
        if (FileExistsW(wszExtraLibPath)) {
            HMODULE hExtra = LoadLibraryW(wszExtraLibPath);
            if (hExtra) {
                wprintf(L"[Extra] Found library: %p.\n", hExtra);
                FARPROC ep_extra_entrypoint = GetProcAddress(hExtra, "ep_extra_EntryPoint");
                if (ep_extra_entrypoint) {
                    wprintf(L"[Extra] Running entry point...\n");
                    ep_extra_entrypoint();
                    wprintf(L"[Extra] Finished running entry point.\n");
                }
            } else {
                wprintf(L"[Extra] LoadLibraryW failed with 0x%lx.", GetLastError());
            }
        }
    }

    if (bHookStartMenu)
    {
        HookStartMenuParams* params2 = calloc(1, sizeof(HookStartMenuParams));
        params2->dwTimeout = 1000;
        params2->hModule = hModule;
        params2->proc = InjectStartFromExplorer;
        GetModuleFileNameW(hModule, params2->wszModulePath, MAX_PATH);
        CreateThread(0, 0, HookStartMenu, params2, 0, 0);
    }
# endif

    VnPatchDelayIAT(hExplorer, "ext-ms-win-rtcore-ntuser-window-ext-l1-1-0.dll", "GetClientRect", TaskbarCenter_GetClientRectHook);
    VnPatchIAT(hExplorer, "SHCORE.dll", (LPCSTR)190, TaskbarCenter_SHWindowsPolicy);
    wprintf(L"Initialized taskbar centering module.\n");

    // CreateThread(NULL, 0, PositionStartMenuTimeout, NULL, 0, NULL);

# if 0
    else
    {
        if (bIsExplorer) {
            // deinject all
            rv = funchook_uninstall(funchook, 0);
            if (rv != 0) {
                FreeLibraryAndExitThread(hModule, rv);
                return rv;
            }
            rv = funchook_destroy(funchook);
            if (rv != 0) {
                FreeLibraryAndExitThread(hModule, rv);
                return rv;
            }
        }

        //SetEvent(hExitSettingsMonitor);
        //WaitForSingleObject(hSettingsMonitorThread, INFINITE);
        //CloseHandle(hExitSettingsMonitor);
        //free(settingsParams);
        //free(settings);
        //InjectBasicFunctions(FALSE, FALSE);
        FreeLibraryAndExitThread(hModule, 0);
    }
# endif
#endif

    return 0;
}


#ifdef _WIN64

static char VisibilityChangedEventArguments_GetVisible(__int64 a1)
{
    int  v1;
    char v3[8] = {0};

    v1 = (*(INT64 (__fastcall **)(INT64, char *))(*(INT64 *)a1 + 48))(a1, v3);
    if (v1 < 0)
        return 0;

    return v3[0];
}

static void StartMenu_LoadSettings(BOOL bRestartIfChanged)
{
    HKEY  hKey = NULL;
    DWORD dwSize, dwVal;

    RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartPage",
                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, NULL);
    if (hKey == INVALID_HANDLE_VALUE)
        hKey = NULL;
    if (hKey) {
        dwSize = sizeof(DWORD);
        RegQueryValueExW(hKey, L"MakeAllAppsDefault", 0, NULL, &StartMenu_ShowAllApps, &dwSize);
        dwSize = sizeof(DWORD);
        RegQueryValueExW(hKey, L"MonitorOverride", 0, NULL, &bMonitorOverride, &dwSize);
        RegCloseKey(hKey);
    }

    RegCreateKeyExW(HKEY_CURRENT_USER, L"" REGPATH_STARTMENU,
                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, NULL);

    if (hKey == INVALID_HANDLE_VALUE)
        hKey = NULL;
    if (hKey) {
        dwSize = sizeof(DWORD);
        dwVal  = 6;
        RegQueryValueExW(hKey, L"Start_MaximumFrequentApps", 0, NULL, &dwVal, &dwSize);
        if (bRestartIfChanged && dwVal != StartMenu_maximumFreqApps)
            exit(0);
        StartMenu_maximumFreqApps = dwVal;

        dwSize = sizeof(DWORD);
        dwVal  = FALSE;
        RegQueryValueExW(hKey, L"StartDocked_DisableRecommendedSection", 0, NULL, &dwVal, &dwSize);
        if (dwVal != StartDocked_DisableRecommendedSection)
            StartDocked_DisableRecommendedSectionApply = TRUE;
        StartDocked_DisableRecommendedSection = dwVal;

        dwSize = sizeof(DWORD);
        dwVal  = FALSE;
        RegQueryValueExW(hKey, L"StartUI_EnableRoundedCorners", 0, NULL, &dwVal, &dwSize);
        if (dwVal != StartUI_EnableRoundedCorners)
            StartUI_EnableRoundedCornersApply = TRUE;
        StartUI_EnableRoundedCorners = dwVal;

        dwSize = sizeof(DWORD);
        dwVal  = FALSE;
        RegQueryValueExW(hKey, L"StartUI_ShowMoreTiles", 0, NULL, &dwVal, &dwSize);
        if (bRestartIfChanged && dwStartShowClassicMode && dwVal != StartUI_ShowMoreTiles)
            exit(0);
        StartUI_ShowMoreTiles = dwVal;

        RegCloseKey(hKey);
    }

    RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, NULL);
    if (hKey == INVALID_HANDLE_VALUE)
        hKey = NULL;
    if (hKey) {
        dwSize = sizeof(DWORD);
        if (IsWindows11())
            dwVal = 0;
        else
            dwVal = 1;
        RegQueryValueExW(hKey, L"Start_ShowClassicMode", 0, NULL, &dwVal, &dwSize);
        if (bRestartIfChanged && dwVal != dwStartShowClassicMode)
            exit(0);
        dwStartShowClassicMode = dwVal;

        dwSize = sizeof(DWORD);
        if (IsWindows11())
            dwVal = 1;
        else
            dwVal = 0;
        RegQueryValueExW(hKey, L"TaskbarAl", 0, NULL, &dwVal, &dwSize);
        if (InterlockedExchange64(&dwTaskbarAl, dwVal) != dwVal) {
            StartUI_EnableRoundedCornersApply          = TRUE;
            StartDocked_DisableRecommendedSectionApply = TRUE;
        }

        RegCloseKey(hKey);
    }

    RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer",
                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, NULL);
    if (hKey == INVALID_HANDLE_VALUE)
        hKey = NULL;
    if (hKey) {
        dwSize = sizeof(DWORD);
        dwVal  = 0;
        RegQueryValueExW(hKey, L"ForceStartSize", 0, NULL, &dwVal, &dwSize);
        if (bRestartIfChanged && dwVal != Start_ForceStartSize)
            exit(0);
        Start_ForceStartSize = dwVal;

        RegCloseKey(hKey);
    }

    RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, NULL);
    if (hKey == INVALID_HANDLE_VALUE)
        hKey = NULL;
    if (hKey) {
        dwSize = sizeof(DWORD);
        dwVal  = 0;
        RegQueryValueExW(hKey, L"NoStartMenuMorePrograms", 0, NULL, &dwVal, &dwSize);
        if (bRestartIfChanged && dwVal != Start_NoStartMenuMorePrograms)
            exit(0);
        Start_NoStartMenuMorePrograms = dwVal;

        RegCloseKey(hKey);
    }
}

static INT64 StartDocked_LauncherFrame_OnVisibilityChangedHook(void *_this, INT64 a2, void *VisibilityChangedEventArguments)
{
    INT64 r = 0;
    if (StartDocked_LauncherFrame_OnVisibilityChangedFunc)
        r = StartDocked_LauncherFrame_OnVisibilityChangedFunc(_this, a2, VisibilityChangedEventArguments);
    if (StartMenu_ShowAllApps) {
        // if (VisibilityChangedEventArguments_GetVisible(VisibilityChangedEventArguments))
        {
            if (StartDocked_LauncherFrame_ShowAllAppsFunc)
                StartDocked_LauncherFrame_ShowAllAppsFunc(_this);
        }
    }
    return r;
}

static INT64 StartDocked_SystemListPolicyProvider_GetMaximumFrequentAppsHook(void *_this)
{
    return StartMenu_maximumFreqApps;
}

static INT64 StartUI_SystemListPolicyProvider_GetMaximumFrequentAppsHook(void *_this)
{
    return StartMenu_maximumFreqApps;
}

static INT64 StartDocked_StartSizingFrame_StartSizingFrameHook(void *_this)
{
    INT64   rv      = StartDocked_StartSizingFrame_StartSizingFrameFunc(_this);
    HMODULE hModule = LoadLibraryW(L"Shlwapi.dll");
    if (hModule) {
        DWORD   dwStatus = 0, dwSize = sizeof(DWORD);
        FARPROC SHRegGetValueFromHKCUHKLMFunc = GetProcAddress(hModule, "SHRegGetValueFromHKCUHKLM");
        if (!SHRegGetValueFromHKCUHKLMFunc ||
            SHRegGetValueFromHKCUHKLMFunc(
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarAl",
                SRRF_RT_REG_DWORD, NULL, &dwStatus, (LPDWORD)(&dwSize)
            ) != ERROR_SUCCESS)
        {
            dwStatus = 0;
        }
        FreeLibrary(hModule);
        *(((char *)_this + 387)) = dwStatus;
    }
    return rv;
}

static HANDLE StartUI_CreateFileW(
    LPCWSTR               lpFileName,
    DWORD                 dwDesiredAccess,
    DWORD                 dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD                 dwCreationDisposition,
    DWORD                 dwFlagsAndAttributes,
    HANDLE                hTemplateFile
)
{
    WCHAR path[MAX_PATH];
    GetWindowsDirectoryW(path, MAX_PATH);
    wcscat_s(path, MAX_PATH, L"\\SystemResources\\Windows.UI.ShellCommon\\Windows.UI.ShellCommon.pri");
    if (_wcsicmp(path, lpFileName) == 0) {
        GetWindowsDirectoryW(path, MAX_PATH);
        wcscat_s(
            path, MAX_PATH,
            L"\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\Windows.UI."
            L"ShellCommon.pri"
        );
        return CreateFileW(
            path, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
            dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile
        );
    }
    GetWindowsDirectoryW(path, MAX_PATH);
    wcscat_s(path, MAX_PATH, L"\\SystemResources\\Windows.UI.ShellCommon\\pris");
    int len = wcslen(path);
    if (!_wcsnicmp(path, lpFileName, len)) {
        GetWindowsDirectoryW(path, MAX_PATH);
        wcscat_s(path, MAX_PATH, L"\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\pris2");
        wcscat_s(path, MAX_PATH, lpFileName + len);
        return CreateFileW(
            path, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
            dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile
        );
    }
    return CreateFileW(
        lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
        dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile
    );
}

static BOOL StartUI_GetFileAttributesExW(LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
{
    WCHAR path[MAX_PATH];
    GetWindowsDirectoryW(path, MAX_PATH);
    wcscat_s(path, MAX_PATH, L"\\SystemResources\\Windows.UI.ShellCommon\\Windows.UI.ShellCommon.pri");
    if (_wcsicmp(path, lpFileName) == 0) {
        GetWindowsDirectoryW(path, MAX_PATH);
        wcscat_s(
            path, MAX_PATH,
            L"\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\Windows.UI."
            L"ShellCommon.pri"
        );
        return GetFileAttributesExW(path, fInfoLevelId, lpFileInformation);
    }
    GetWindowsDirectoryW(path, MAX_PATH);
    wcscat_s(path, MAX_PATH, L"\\SystemResources\\Windows.UI.ShellCommon\\pris");
    size_t len = wcslen(path);
    if (_wcsnicmp(path, lpFileName, len) == 0) {
        GetWindowsDirectoryW(path, MAX_PATH);
        wcscat_s(path, MAX_PATH, L"\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\pris2");
        wcscat_s(path, MAX_PATH, lpFileName + len);
        return GetFileAttributesExW(path, fInfoLevelId, lpFileInformation);
    }
    return GetFileAttributesExW(lpFileName, fInfoLevelId, lpFileInformation);
}

static HANDLE StartUI_FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
    WCHAR path[MAX_PATH];
    GetWindowsDirectoryW(path, MAX_PATH);
    wcscat_s(path, MAX_PATH, L"\\SystemResources\\Windows.UI.ShellCommon\\Windows.UI.ShellCommon.pri");
    if (_wcsicmp(path, lpFileName) == 0) {
        GetWindowsDirectoryW(path, MAX_PATH);
        wcscat_s(path, MAX_PATH, L"\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\Windows.UI.ShellCommon.pri");
        return FindFirstFileW(path, lpFindFileData);
    }
    GetWindowsDirectoryW(path, MAX_PATH);
    wcscat_s(path, MAX_PATH, L"\\SystemResources\\Windows.UI.ShellCommon\\pris");
    int len = wcslen(path);
    if (_wcsnicmp(path, lpFileName, len) == 0) {
        GetWindowsDirectoryW(path, MAX_PATH);
        wcscat_s(path, MAX_PATH, L"\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\pris2");
        wcscat_s(path, MAX_PATH, lpFileName + len);
        return FindFirstFileW(path, lpFindFileData);
    }
    return FindFirstFileW(lpFileName, lpFindFileData);
}

static LSTATUS StartUI_RegGetValueW(
    HKEY    hkey,
    LPCWSTR lpSubKey,
    LPCWSTR lpValue,
    DWORD   dwFlags,
    LPDWORD pdwType,
    PVOID   pvData,
    LPDWORD pcbData
)
{
    if (hkey == HKEY_LOCAL_MACHINE &&
        _wcsicmp(lpSubKey, L"Software\\Microsoft\\Windows\\CurrentVersion\\Mrt\\_Merged") == 0 &&
        _wcsicmp(lpValue, L"ShouldMergeInProc") == 0)
    {
        *(DWORD *)pvData = 1;
        return ERROR_SUCCESS;
    }
    return RegGetValueW(hkey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);
}

static LSTATUS StartUI_RegOpenKeyExW(
    HKEY    hKey,
    LPCWSTR lpSubKey,
    DWORD   ulOptions,
    REGSAM  samDesired,
    PHKEY   phkResult
)
{
    if (wcsstr(lpSubKey, L"$start.tilegrid$windows.data.curatedtilecollection.tilecollection\\Current")) {
        LSTATUS lRes = RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
        if (lRes == ERROR_SUCCESS)
            hKey_StartUI_TileGrid = *phkResult;
        return lRes;
    }
    return RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

static LSTATUS StartUI_RegQueryValueExW(
    HKEY    hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
)
{
    if (hKey == hKey_StartUI_TileGrid) {
        if (WStrIEq(lpValueName, L"Data")) {
            LSTATUS lRes = RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
            if (lRes == ERROR_SUCCESS && lpData && *lpcbData >= 26)
                lpData[25] = (StartUI_ShowMoreTiles ? 16 : 12);
            return lRes;
        }
    }
    return RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

static LSTATUS StartUI_RegCloseKey(HKEY hKey)
{
    if (hKey == hKey_StartUI_TileGrid)
        hKey_StartUI_TileGrid = NULL;
    return RegCloseKey(hKey);
}

static int Start_SetWindowRgn(HWND hWnd, HRGN hRgn, BOOL bRedraw)
{
    WCHAR   wszDebug[MAX_PATH];
    BOOL    bIsWindowVisible = FALSE;
    HRESULT hr               = IsThreadCoreWindowVisible(&bIsWindowVisible);
    if (SUCCEEDED(hr)) {
        if (IsWindows11())
            ShowWindow(hWnd, bIsWindowVisible ? SW_SHOW : SW_HIDE);
        DWORD TaskbarAl = InterlockedAdd(&dwTaskbarAl, 0);
        if (bIsWindowVisible && (!TaskbarAl ? (dwStartShowClassicMode ? StartUI_EnableRoundedCornersApply
                                                                      : StartDocked_DisableRecommendedSectionApply)
                                            : 1)) {
            HWND hWndTaskbar = NULL;
            if (TaskbarAl) {
                HMONITOR hMonitorOfStartMenu = NULL;
                if (bMonitorOverride == 1 || !bMonitorOverride) {
                    POINT pt;
                    if (!bMonitorOverride)
                        GetCursorPos(&pt);
                    else {
                        pt.x = 0;
                        pt.y = 0;
                    }
                    hMonitorOfStartMenu = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
                } else {
                    MonitorOverrideData mod;
                    mod.cbIndex  = 2;
                    mod.dwIndex  = bMonitorOverride;
                    mod.hMonitor = NULL;
                    EnumDisplayMonitors(NULL, NULL, ExtractMonitorByIndex, &mod);
                    if (mod.hMonitor == NULL) {
                        POINT pt;
                        pt.x                = 0;
                        pt.y                = 0;
                        hMonitorOfStartMenu = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
                    } else {
                        hMonitorOfStartMenu = mod.hMonitor;
                    }
                }

                HWND hWndTemp = NULL;

                HWND hShellTray_Wnd = FindWindowExW(NULL, NULL, L"Shell_TrayWnd", NULL);
                if (hShellTray_Wnd && !hWndTaskbar &&
                    hMonitorOfStartMenu == MonitorFromWindow(hShellTray_Wnd, MONITOR_DEFAULTTOPRIMARY) &&
                    dwOldTaskbarAl) {
                    hWndTaskbar = hShellTray_Wnd;
                }

                if (!hWndTaskbar) {
                    do {
                        hWndTemp = FindWindowExW(NULL, hWndTemp, L"Shell_SecondaryTrayWnd", NULL);
                        if (hWndTemp && !hWndTaskbar &&
                            hMonitorOfStartMenu == MonitorFromWindow(hWndTemp, MONITOR_DEFAULTTOPRIMARY) &&
                            dwMMOldTaskbarAl) {
                            hWndTaskbar = hWndTemp;
                            break;
                        }
                    } while (hWndTemp);
                }

                if (!hWndTaskbar)
                    hWndTaskbar = hShellTray_Wnd;
            }
            MONITORINFO mi;
            ZeroMemory(&mi, sizeof(MONITORINFO));
            mi.cbSize = sizeof(MONITORINFO);
            GetMonitorInfoW(MonitorFromWindow(hWndTaskbar ? hWndTaskbar : hWnd, MONITOR_DEFAULTTOPRIMARY), &mi);
            DWORD dwPos = 0;
            RECT  rcC;
            if (hWndTaskbar) {
                GetWindowRect(hWndTaskbar, &rcC);
                rcC.left -= mi.rcMonitor.left;
                rcC.right -= mi.rcMonitor.left;
                rcC.top -= mi.rcMonitor.top;
                rcC.bottom -= mi.rcMonitor.top;
                if (rcC.left < 5 && rcC.top > 5)
                    dwPos = TB_POS_BOTTOM;
                else if (rcC.left < 5 && rcC.top < 5 && rcC.right > rcC.bottom)
                    dwPos = TB_POS_TOP;
                else if (rcC.left < 5 && rcC.top < 5 && rcC.right < rcC.bottom)
                    dwPos = TB_POS_LEFT;
                else if (rcC.left > 5 && rcC.top < 5)
                    dwPos = TB_POS_RIGHT;
            }
            RECT rc;
            if (dwStartShowClassicMode) {
                LVT_StartUI_EnableRoundedCorners(hWnd, StartUI_EnableRoundedCorners, dwPos, hWndTaskbar, &rc);
                if (!StartUI_EnableRoundedCorners)
                    StartUI_EnableRoundedCornersApply = FALSE;
            } else {
                LVT_StartDocked_DisableRecommendedSection(hWnd, StartDocked_DisableRecommendedSection, &rc);
                // StartDocked_DisableRecommendedSectionApply = FALSE;
            }
            if (hWndTaskbar) {
                if (rcC.left < 5 && rcC.top > 5) {
                    if (dwStartShowClassicMode) {
                        SetWindowPos(
                            hWnd, NULL,
                            mi.rcMonitor.left + (((mi.rcMonitor.right - mi.rcMonitor.left) - (rc.right - rc.left)) / 2),
                            mi.rcMonitor.top, 0, 0, SWP_NOSIZE | SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS
                        );
                    } else {
                        // Windows 11 Start menu knows how to center itself when the taskbar is at the bottom
                        // of the screen
                        SetWindowPos(
                            hWnd, NULL, mi.rcMonitor.left, mi.rcMonitor.top, 0, 0,
                            SWP_NOSIZE | SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS
                        );
                    }
                } else if (rcC.left < 5 && rcC.top < 5 && rcC.right > rcC.bottom) {
                    SetWindowPos(
                        hWnd, NULL,
                        mi.rcMonitor.left + (((mi.rcMonitor.right - mi.rcMonitor.left) - (rc.right - rc.left)) / 2),
                        mi.rcMonitor.top + (rcC.bottom - rcC.top), 0, 0,
                        SWP_NOSIZE | SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS
                    );
                } else if (rcC.left < 5 && rcC.top < 5 && rcC.right < rcC.bottom) {
                    SetWindowPos(
                        hWnd, NULL, mi.rcMonitor.left + (rcC.right - rcC.left),
                        mi.rcMonitor.top + (((mi.rcMonitor.bottom - mi.rcMonitor.top) - (rc.bottom - rc.top)) / 2), 0,
                        0, SWP_NOSIZE | SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS
                    );
                } else if (rcC.left > 5 && rcC.top < 5) {
                    SetWindowPos(
                        hWnd, NULL, mi.rcMonitor.left,
                        mi.rcMonitor.top + (((mi.rcMonitor.bottom - mi.rcMonitor.top) - (rc.bottom - rc.top)) / 2), 0,
                        0, SWP_NOSIZE | SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS
                    );
                }
            } else {
                SetWindowPos(
                    hWnd, NULL, mi.rcWork.left, mi.rcWork.top, 0, 0, SWP_NOSIZE | SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS
                );
            }
        }
    }
    return SetWindowRgn(hWnd, hRgn, bRedraw);
}


static int WINAPI SetupMessage(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    return 0;
    LPCWSTR lpOldText    = lpText;
    LPCWSTR lpOldCaption = lpCaption;
    wchar_t wszText[MAX_PATH];
    ZeroMemory(wszText, MAX_PATH * sizeof(wchar_t));
    wchar_t wszCaption[MAX_PATH];
    ZeroMemory(wszCaption, MAX_PATH * sizeof(wchar_t));
    LoadStringW(hModule, IDS_PRODUCTNAME, wszCaption, MAX_PATH);
    switch (dllCode) {
    case 1:
        LoadStringW(hModule, IDS_INSTALL_SUCCESS_TEXT, wszText, MAX_PATH);
        break;
    case -1:
        LoadStringW(hModule, IDS_INSTALL_ERROR_TEXT, wszText, MAX_PATH);
        break;
    case 2:
        LoadStringW(hModule, IDS_UNINSTALL_SUCCESS_TEXT, wszText, MAX_PATH);
        break;
    case -2:
        LoadStringW(hModule, IDS_UNINSTALL_ERROR_TEXT, wszText, MAX_PATH);
        break;
    default:
        LoadStringW(hModule, IDS_OPERATION_NONE, wszText, MAX_PATH);
        break;
    }
    int ret      = MessageBoxW(hWnd, wszText, wszCaption, uType);
    lpText       = lpOldText;
    lpOldCaption = lpOldCaption;
    return ret;
}

static void Setup_Regsvr32(BOOL bInstall)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (!IsAppRunningAsAdminMode()) {
        wchar_t wszPath[MAX_PATH];
        wchar_t wszCurrentDirectory[MAX_PATH];

        if (GetModuleFileNameW(NULL, wszPath, _countof(wszPath)) &&
            GetCurrentDirectoryW(_countof(wszCurrentDirectory), wszCurrentDirectory + (bInstall ? 1 : 4))) {
            wszCurrentDirectory[0] = L'"';
            if (!bInstall) {
                wszCurrentDirectory[0] = L'/';
                wszCurrentDirectory[1] = L'u';
                wszCurrentDirectory[2] = L' ';
                wszCurrentDirectory[3] = L'"';
                wszCurrentDirectory[4] = L'\0';
            } else {
                wszCurrentDirectory[1] = L'\0';
            }

            wcscat_s(wszCurrentDirectory, _countof(wszCurrentDirectory), L"\\ExplorerPatcher.amd64.dll\"");

            SHELLEXECUTEINFOW sei = {
                .cbSize       = sizeof sei,
                .lpVerb       = L"runas",
                .lpFile       = wszPath,
                .lpParameters = wszCurrentDirectory,
                .hwnd         = NULL,
                .nShow        = SW_NORMAL,
            };

            if (!ShellExecuteExW(&sei)) {
                DWORD dwError = GetLastError();
                if (dwError == ERROR_CANCELLED) {
                    wchar_t wszText[MAX_PATH];
                    wchar_t wszCaption[MAX_PATH];
                    LoadStringW(hModule, IDS_PRODUCTNAME, wszCaption, _countof(wszText));
                    LoadStringW(hModule, IDS_INSTALL_ERROR_TEXT, wszText, _countof(wszText));
                    MessageBoxW(NULL, wszText, wszCaption, MB_ICONINFORMATION);
                }
            }

            exit(0);
        }
    }

    VnPatchDelayIAT(GetModuleHandleW(NULL), "ext-ms-win-ntuser-dialogbox-l1-1-0.dll", "MessageBoxW", SetupMessage);
}

# ifdef _WIN64
#  pragma comment(linker, "/export:DllRegisterServer=_DllRegisterServer")
# endif
HRESULT WINAPI
_DllRegisterServer(void)
{
    DWORD   dwLastError = ERROR_SUCCESS;
    HKEY    hKey        = NULL;
    DWORD   dwSize      = 0;
    wchar_t wszFilename[MAX_PATH];
    wchar_t wszInstallPath[MAX_PATH];

    Setup_Regsvr32(TRUE);

    if (!dwLastError) {
        if (!GetModuleFileNameW(hModule, wszFilename, MAX_PATH))
            dwLastError = GetLastError();
    }
    if (!dwLastError) {
        dwLastError = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID\\" EP_CLSID L"\\InProcServer32", 0,
                                      NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY, NULL, &hKey, NULL);
        if (hKey == INVALID_HANDLE_VALUE)
            hKey = NULL;
        if (hKey) {
            dwLastError = RegSetValueExW(hKey, NULL, 0, REG_SZ, wszFilename, (wcslen(wszFilename) + 1) * sizeof(wchar_t));
            dwLastError = RegSetValueExW(hKey, L"ThreadingModel", 0, REG_SZ, L"Apartment", 10 * sizeof(wchar_t));
            RegCloseKey(hKey);
        }
    }
    if (!dwLastError) {
        PathRemoveExtensionW(wszFilename);
        PathRemoveExtensionW(wszFilename);
        wcscat_s(wszFilename, MAX_PATH, L".IA-32.dll");
    }
    if (!dwLastError) {
        dwLastError = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Classes\\CLSID\\" EP_CLSID L"\\InProcServer32",
                                      0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY, NULL, &hKey, NULL);
        if (hKey == INVALID_HANDLE_VALUE)
            hKey = NULL;
        if (hKey) {
            dwLastError = RegSetValueExW(hKey, NULL, 0, REG_SZ, wszFilename, (wcslen(wszFilename) + 1) * sizeof(wchar_t));
            dwLastError = RegSetValueExW(hKey, L"ThreadingModel", 0, REG_SZ, L"Apartment", 10 * sizeof(wchar_t));
            RegCloseKey(hKey);
        }
    }
    if (!dwLastError) {
        dwLastError = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\Drive\\shellex\\FolderExtensions\\" EP_CLSID,
                                      0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY, NULL, &hKey, NULL);
        if (hKey == INVALID_HANDLE_VALUE)
            hKey = NULL;
        if (hKey) {
            DWORD dwDriveMask = 255;
            dwLastError       = RegSetValueExW(hKey, L"DriveMask", 0, REG_DWORD, &dwDriveMask, sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }

#if 0
    if (!dwLastError) {
        dwLastError = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects\\" EP_CLSID,
                                      0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY, NULL, &hKey, NULL);
        if (hKey == INVALID_HANDLE_VALUE)
            hKey = NULL;
        if (hKey) {
            DWORD dwNoInternetExplorer = 1;
            dwLastError = RegSetValueExW(hKey, L"NoInternetExplorer", 0, REG_DWORD, &dwNoInternetExplorer, sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }
#endif

    dllCode = 1;
    if (dwLastError)
        dllCode = -dllCode;

    // ZZRestartExplorer(0, 0, 0, 0);

    return dwLastError == 0 ? S_OK : HRESULT_FROM_WIN32(dwLastError);
}

# ifdef _WIN64
#  pragma comment(linker, "/export:DllUnregisterServer=_DllUnregisterServer")
# endif
HRESULT WINAPI
_DllUnregisterServer(void)
{
    DWORD   dwLastError = ERROR_SUCCESS;
    HKEY    hKey        = NULL;
    DWORD   dwSize      = 0;
    wchar_t wszFilename[MAX_PATH];

    Setup_Regsvr32(FALSE);

    if (!dwLastError) {
        if (!GetModuleFileNameW(hModule, wszFilename, MAX_PATH))
            dwLastError = GetLastError();
    }
    if (!dwLastError) {
        dwLastError = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID\\" EP_CLSID, &hKey);
        if (hKey == INVALID_HANDLE_VALUE)
            hKey = NULL;
        if (hKey) {
            dwLastError = RegDeleteTreeW(hKey, 0);
            RegCloseKey(hKey);
            if (!dwLastError)
                RegDeleteTreeW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID\\" EP_CLSID);
        }
    }
    if (!dwLastError) {
        PathRemoveExtensionW(wszFilename);
        PathRemoveExtensionW(wszFilename);
        wcscat_s(wszFilename, MAX_PATH, L".IA-32.dll");
    }
    if (!dwLastError) {
        dwLastError = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Classes\\CLSID\\" EP_CLSID, &hKey);
        if (hKey == INVALID_HANDLE_VALUE)
            hKey = NULL;
        if (hKey) {
            dwLastError = RegDeleteTreeW(hKey, 0);
            RegCloseKey(hKey);
            if (!dwLastError)
                RegDeleteTreeW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Classes\\CLSID\\" EP_CLSID);
        }
    }
    if (!dwLastError) {
        dwLastError = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\Drive\\shellex\\FolderExtensions\\" EP_CLSID, &hKey
        );
        if (hKey == INVALID_HANDLE_VALUE)
            hKey = NULL;
        if (hKey) {
            dwLastError = RegDeleteTreeW(hKey, 0);
            RegCloseKey(hKey);
            if (!dwLastError)
                RegDeleteTreeW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\Drive\\shellex\\FolderExtensions\\" EP_CLSID);
        }
    }

#if 0
    if (!dwLastError)
    {
        dwLastError = RegOpenKeyW(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects\\" EP_CLSID, &hKey
        );
        if (hKey == INVALID_HANDLE_VALUE)
        {
            hKey = NULL;
        }
        if (hKey)
        {
            dwLastError = RegDeleteTreeW(
                hKey,
                0
            );
            RegCloseKey(hKey);
            if (!dwLastError)
            {
                RegDeleteTreeW(
                    HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects\\" EP_CLSID
                );
            }
        }
    }
#endif

    dllCode = 2;
    if (dwLastError)
        dllCode = -dllCode;

    // ZZRestartExplorer(0, 0, 0, 0);

    return dwLastError == 0 ? S_OK : HRESULT_FROM_WIN32(dwLastError);
}

#endif


#ifdef _WIN64
# pragma comment(linker, "/export:DllCanUnloadNow=_DllCanUnloadNow")
#else
# pragma comment(linker, "/export:DllCanUnloadNow=__DllCanUnloadNow@0")
#endif
HRESULT WINAPI
_DllCanUnloadNow(void)
{
    return S_FALSE;
}


static int InjectStartMenu(void)
{
#ifdef _WIN64
    funchook = funchook_create();

    HANDLE hStartDocked = NULL;
    HANDLE hStartUI     = NULL;

    if (!IsWindows11())
        dwTaskbarAl = 0;

    StartMenu_LoadSettings(FALSE);

    if (dwStartShowClassicMode || !IsWindows11()) {
        LoadLibraryW(L"StartUI.dll");
        hStartUI = GetModuleHandleW(L"StartUI.dll");

        // Fixes hang when Start menu closes
        VnPatchDelayIAT(hStartUI, "ext-ms-win-ntuser-draw-l1-1-0.dll", "SetWindowRgn", Start_SetWindowRgn);

        if (IsWindows11()) {
            // Redirects to StartTileData from 22000.51 which works with the legacy menu
            LoadLibraryW(L"combase.dll");
            HANDLE hCombase = GetModuleHandleW(L"combase.dll");
            VnPatchIAT(hCombase, "api-ms-win-core-libraryloader-l1-2-0.dll", "LoadLibraryExW", patched_LoadLibraryExW);

            // Redirects to pri files from 22000.51 which work with the legacy menu
            LoadLibraryW(L"MrmCoreR.dll");
            HANDLE hMrmCoreR = GetModuleHandleW(L"MrmCoreR.dll");
            VnPatchIAT(hMrmCoreR, "api-ms-win-core-file-l1-1-0.dll", "CreateFileW", StartUI_CreateFileW);
            VnPatchIAT(hMrmCoreR, "api-ms-win-core-file-l1-1-0.dll", "GetFileAttributesExW", StartUI_GetFileAttributesExW);
            VnPatchIAT(hMrmCoreR, "api-ms-win-core-file-l1-1-0.dll", "FindFirstFileW", StartUI_FindFirstFileW);
            VnPatchIAT(hMrmCoreR, "api-ms-win-core-registry-l1-1-0.dll", "RegGetValueW", StartUI_RegGetValueW);

            // Enables "Show more tiles" setting
            LoadLibraryW(L"Windows.CloudStore.dll");
            HANDLE hWindowsCloudStore = GetModuleHandleW(L"Windows.CloudStore.dll");
            VnPatchIAT(hWindowsCloudStore, "api-ms-win-core-registry-l1-1-0.dll", "RegOpenKeyExW", StartUI_RegOpenKeyExW);
            VnPatchIAT(hWindowsCloudStore, "api-ms-win-core-registry-l1-1-0.dll", "RegQueryValueExW", StartUI_RegQueryValueExW);
            VnPatchIAT(hWindowsCloudStore, "api-ms-win-core-registry-l1-1-0.dll", "RegCloseKey", StartUI_RegCloseKey);
        }
    } else {
        LoadLibraryW(L"StartDocked.dll");
        hStartDocked = GetModuleHandleW(L"StartDocked.dll");

        VnPatchDelayIAT(hStartDocked, "ext-ms-win-ntuser-draw-l1-1-0.dll", "SetWindowRgn", Start_SetWindowRgn);
    }

    Setting *settings    = malloc(6 * sizeof(Setting));
    settings[0].callback = NULL;
    settings[0].data     = NULL;
    settings[0].hEvent   = CreateEventW(NULL, FALSE, FALSE, NULL);
    settings[0].hKey     = NULL;
    settings[0].origin   = NULL;

    settings[1].callback = StartMenu_LoadSettings;
    settings[1].data     = FALSE;
    settings[1].hEvent   = NULL;
    settings[1].hKey     = NULL;
    settings[1].origin   = HKEY_CURRENT_USER;

    settings[2].callback = StartMenu_LoadSettings;
    settings[2].data     = TRUE;
    settings[2].hEvent   = NULL;
    settings[2].hKey     = NULL;
    settings[2].origin   = HKEY_CURRENT_USER;

    settings[3].callback = StartMenu_LoadSettings;
    settings[3].data     = TRUE;
    settings[3].hEvent   = NULL;
    settings[3].hKey     = NULL;
    settings[3].origin   = HKEY_CURRENT_USER;

    settings[4].callback = StartMenu_LoadSettings;
    settings[4].data     = TRUE;
    settings[4].hEvent   = NULL;
    settings[4].hKey     = NULL;
    settings[4].origin   = HKEY_CURRENT_USER;

    settings[5].callback = StartMenu_LoadSettings;
    settings[5].data     = TRUE;
    settings[5].hEvent   = NULL;
    settings[5].hKey     = NULL;
    settings[5].origin = HKEY_CURRENT_USER;

    settings[0].name[0] = L'\0';
    wcscpy(settings[1].name, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartPage");
    wcscpy(settings[2].name, L"" REGPATH_STARTMENU);
    wcscpy(settings[3].name, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
    wcscpy(settings[4].name, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer");
    wcscpy(settings[5].name, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer");

    SettingsChangeParameters *params = malloc(sizeof *params);
    params->settings = settings;
    params->size     = 6;
    params->hThread  = NULL;

    HANDLE hThread = CreateThread(NULL, 0, MonitorSettings, params, 0, NULL);
    CloseHandle(hThread);

    int     rv;
    DWORD   dwVals[5] = {0x62254, 0x188EBC, 0x187120, 0x3C10, 0};
    HMODULE hModule   = LoadLibraryW(L"Shlwapi.dll");
    if (hModule) {
        SHRegGetValueFromHKCUHKLMFunc_t SHRegGetValueFromHKCUHKLMFunc = GetProcAddress(hModule, "SHRegGetValueFromHKCUHKLM");

        if (SHRegGetValueFromHKCUHKLMFunc) {
            DWORD dwSize = sizeof(DWORD);
            SHRegGetValueFromHKCUHKLMFunc(REGPATH_STARTMENU L"\\" STARTDOCKED_SB_NAME, L"" STARTDOCKED_SB_0, SRRF_RT_REG_DWORD, NULL, &dwVals[0], (LPDWORD)(&dwSize));
            SHRegGetValueFromHKCUHKLMFunc(REGPATH_STARTMENU L"\\" STARTDOCKED_SB_NAME, L"" STARTDOCKED_SB_1, SRRF_RT_REG_DWORD, NULL, &dwVals[1], (LPDWORD)(&dwSize));
            SHRegGetValueFromHKCUHKLMFunc(REGPATH_STARTMENU L"\\" STARTDOCKED_SB_NAME, L"" STARTDOCKED_SB_2, SRRF_RT_REG_DWORD, NULL, &dwVals[2], (LPDWORD)(&dwSize));
            SHRegGetValueFromHKCUHKLMFunc(REGPATH_STARTMENU L"\\" STARTDOCKED_SB_NAME, L"" STARTDOCKED_SB_3, SRRF_RT_REG_DWORD, NULL, &dwVals[3], (LPDWORD)(&dwSize));
            SHRegGetValueFromHKCUHKLMFunc(REGPATH_STARTMENU L"\\" STARTUI_SB_NAME,     L"" STARTUI_SB_0,     SRRF_RT_REG_DWORD, NULL, &dwVals[4], (LPDWORD)(&dwSize));
        }
        FreeLibrary(hModule);
    }

    if (dwVals[1] && dwVals[1] != 0xFFFFFFFF && hStartDocked)
        StartDocked_LauncherFrame_ShowAllAppsFunc = (INT64(*)(void *))((uintptr_t)hStartDocked + dwVals[1]);
    if (dwVals[2] && dwVals[2] != 0xFFFFFFFF && hStartDocked) {
        StartDocked_LauncherFrame_OnVisibilityChangedFunc = (INT64(*)(void *, INT64, void *))((uintptr_t)hStartDocked + dwVals[2]);
        rv = funchook_prepare(funchook, (void **)&StartDocked_LauncherFrame_OnVisibilityChangedFunc,
                              StartDocked_LauncherFrame_OnVisibilityChangedHook);
        if (rv != 0) {
            FreeLibraryAndExitThread(hModule, rv);
            return rv;
        }
    }
    if (dwVals[3] && dwVals[3] != 0xFFFFFFFF && hStartDocked) {
        StartDocked_SystemListPolicyProvider_GetMaximumFrequentAppsFunc = (INT64(*)(void *, INT64, void *))((uintptr_t)hStartDocked + dwVals[3]);
        rv = funchook_prepare(funchook, (void **)&StartDocked_SystemListPolicyProvider_GetMaximumFrequentAppsFunc,
                              StartDocked_SystemListPolicyProvider_GetMaximumFrequentAppsHook);
        if (rv != 0) {
            FreeLibraryAndExitThread(hModule, rv);
            return rv;
        }
    }
    if (dwVals[4] && dwVals[4] != 0xFFFFFFFF && hStartUI) {
        StartUI_SystemListPolicyProvider_GetMaximumFrequentAppsFunc = (INT64(*)(void *, INT64, void *))((uintptr_t)hStartUI + dwVals[4]);
        rv = funchook_prepare(funchook, (void **)&StartUI_SystemListPolicyProvider_GetMaximumFrequentAppsFunc,
                              StartUI_SystemListPolicyProvider_GetMaximumFrequentAppsHook);
        if (rv != 0) {
            FreeLibraryAndExitThread(hModule, rv);
            return rv;
        }
    }

    rv = funchook_install(funchook, 0);
    if (rv != 0) {
        FreeLibraryAndExitThread(hModule, rv);
        return rv;
    }

#endif
    return 0;
}

static void InjectShellExperienceHost(void)
{
#ifdef _WIN64
    HKEY hKey;
    if (RegOpenKeyW(HKEY_CURRENT_USER, L"" SEH_REGPATH, &hKey) != ERROR_SUCCESS)
        return;
    RegCloseKey(hKey);
    HMODULE hQA = LoadLibraryW(L"Windows.UI.QuickActions.dll");
    if (hQA) {
        PIMAGE_DOS_HEADER dosHeader = hQA;
        if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
            PIMAGE_NT_HEADERS64 ntHeader = (PIMAGE_NT_HEADERS64)((u_char *)dosHeader + dosHeader->e_lfanew);
            if (ntHeader->Signature == IMAGE_NT_SIGNATURE) {
                unsigned char *pSEHPatchArea    = NULL;
                unsigned char seh_pattern1[14] = {
                    // mov al, 1
                    0xB0, 0x01,
                    // jmp + 2
                    0xEB, 0x02,
                    // xor al, al
                    0x32, 0xC0,
                    // add rsp, 0x20
                    0x48, 0x83, 0xC4, 0x20,
                    // pop rdi
                    0x5F,
                    // pop rsi
                    0x5E,
                    // pop rbx
                    0x5B,
                    // ret
                    0xC3
                };
                unsigned char seh_off = 12;
                unsigned char seh_pattern2[5] = {
                    // mov r8b, 3
                    0x41, 0xB0, 0x03,
                    // mov dl, 1
                    0xB2, 0x01
                };

                BOOL                  bTwice  = FALSE;
                PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeader);

                for (unsigned i = 0; i < ntHeader->FileHeader.NumberOfSections; ++i) {
                    if (section->Characteristics & IMAGE_SCN_CNT_CODE) {
                        if (section->SizeOfRawData && !bTwice) {
                            DWORD dwOldProtect;
                            // VirtualProtect(hQA + section->VirtualAddress, section->SizeOfRawData,
                            // PAGE_EXECUTE_READWRITE, &dwOldProtect);
                            unsigned char *pCandidate = NULL;
                            while (TRUE) {
                                pCandidate = memmem(
                                    !pCandidate ? hQA + section->VirtualAddress : pCandidate,
                                    !pCandidate ? section->SizeOfRawData
                                                : (uintptr_t)section->SizeOfRawData - (uintptr_t)(
                                                    (ptrdiff_t)pCandidate - (ptrdiff_t)(hQA + section->VirtualAddress)),
                                    seh_pattern1, sizeof(seh_pattern1)
                                );
                                if (!pCandidate)
                                    break;
                                unsigned char *pCandidate2 = pCandidate - seh_off - sizeof(seh_pattern2);
                                if (pCandidate2 > section->VirtualAddress) {
                                    if (memmem(pCandidate2, sizeof(seh_pattern2), seh_pattern2, sizeof(seh_pattern2))) {
                                        if (!pSEHPatchArea)
                                            pSEHPatchArea = pCandidate;
                                        else
                                            bTwice = TRUE;
                                    }
                                }
                                pCandidate += sizeof(seh_pattern1);
                            }
                            // VirtualProtect(hQA + section->VirtualAddress, section->SizeOfRawData,
                            // dwOldProtect, &dwOldProtect);
                        }
                    }
                    section++;
                }
                if (pSEHPatchArea && !bTwice) {
                    DWORD dwOldProtect;
                    VirtualProtect(pSEHPatchArea, sizeof(seh_pattern1), PAGE_EXECUTE_READWRITE, &dwOldProtect);
                    pSEHPatchArea[2] = 0x90;
                    pSEHPatchArea[3] = 0x90;
                    VirtualProtect(pSEHPatchArea, sizeof(seh_pattern1), dwOldProtect, &dwOldProtect);
                }
            }
        }
    }
#endif
}

// On 22H2 builds, the Windows 10 flyouts for network and battery can be enabled
// by patching either of the following functions in ShellExperienceHost. I didn't
// see any differences when patching with any of the 3 methods, although
// `SharedUtilities::IsWindowsLite` seems to be invoked in more places, whereas `GetProductInfo`
// and `RtlGetDeviceFamilyInfoEnum` are only called in `FlightHelper::CalculateRepaintEnabled`
// and either seems to get the job done. YMMV

static LSTATUS SEH_RegGetValueW(
    HKEY    hkey,
    LPCWSTR lpSubKey,
    LPCWSTR lpValue,
    DWORD   dwFlags,
    LPDWORD pdwType,
    PVOID   pvData,
    LPDWORD pcbData
)
{
    if (lpValue && lWStrEq(lpValue, L"UseLiteLayout")) {
        *(DWORD *)pvData = 1;
        return ERROR_SUCCESS;
    }
    return RegGetValueW(hkey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);
}

static BOOL SEH_RtlGetDeviceFamilyInfoEnum(INT64 u0, PDWORD u1, INT64 u2)
{
    *u1 = 10;
    return TRUE;
}

static BOOL SEH_GetProductInfo(
    DWORD  dwOSMajorVersion,
    DWORD  dwOSMinorVersion,
    DWORD  dwSpMajorVersion,
    DWORD  dwSpMinorVersion,
    PDWORD pdwReturnedProductType
)
{
    *pdwReturnedProductType = 119;
    return TRUE;
}

static void InjectShellExperienceHostFor22H2OrHigher(void)
{
#ifdef _WIN64
    HKEY hKey;
    if (RegOpenKeyW(HKEY_CURRENT_USER, L"" SEH_REGPATH, &hKey) != ERROR_SUCCESS)
        return;
    RegCloseKey(hKey);
    HMODULE hQA = LoadLibraryW(L"Windows.UI.QuickActions.dll");
    if (hQA)
        VnPatchIAT(hQA, "api-ms-win-core-sysinfo-l1-2-0.dll", "GetProductInfo", SEH_GetProductInfo);
        // if (hQA) VnPatchIAT(hQA, "ntdll.dll", "RtlGetDeviceFamilyInfoEnum",
        // SEH_RtlGetDeviceFamilyInfoEnum); if (hQA) VnPatchIAT(hQA, "api-ms-win-core-registry-l1-1-0.dll",
        // "RegGetValueW", SEH_RegGetValueW);
#endif
}

#ifdef _WIN64
static HRESULT InformUserAboutCrashCallback(
    HWND     hwnd,
    UINT     msg,
    WPARAM   wParam,
    LPARAM   lParam,
    LONG_PTR lpRefData
)
{
    if (msg == TDN_HYPERLINK_CLICKED) {
        if (wcschr(lParam, L'\'')) {
            for (int i = 0; i < wcslen(lParam); ++i)
                if (*(((wchar_t *)lParam) + i) == L'\'')
                    *(((wchar_t *)lParam) + i) = L'"';

            PROCESS_INFORMATION pi;
            STARTUPINFOW si = {.cb = sizeof si};

            if (CreateProcessW(NULL, lParam, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            for (int i = 0; i < wcslen(lParam); ++i)
                if (*(((wchar_t *)lParam) + i) == L'"')
                    *(((wchar_t *)lParam) + i) = L'\'';
        }
        else if (!wcsncmp(lParam, L"eplink://update", 15)) {
            if (!wcsncmp(lParam, L"eplink://update/stable", 22)) {
                DWORD no = 0;
                RegSetKeyValueW(HKEY_CURRENT_USER, L"" REGPATH, L"UpdatePreferStaging", REG_DWORD, &no, sizeof(DWORD));
            } else if (!wcsncmp(lParam, L"eplink://update/staging", 23)) {
                DWORD yes = 1;
                RegSetKeyValueW(HKEY_CURRENT_USER, L"" REGPATH, L"UpdatePreferStaging", REG_DWORD, &yes, sizeof(DWORD));
            }
            SetLastError(0);
            HANDLE sigInstallUpdates = CreateEventW(NULL, FALSE, FALSE, L"EP_Ev_InstallUpdates_" EP_CLSID);
            if (sigInstallUpdates && GetLastError() == ERROR_ALREADY_EXISTS) {
                SetEvent(sigInstallUpdates);
            } else {
                HANDLE hThread = CreateThread(NULL, 0, CheckForUpdatesThread, 0, 0, NULL);
                CloseHandle(hThread);
                Sleep(100);
                SetLastError(0);
                sigInstallUpdates = CreateEventW(NULL, FALSE, FALSE, L"EP_Ev_InstallUpdates_" EP_CLSID);
                if (sigInstallUpdates && GetLastError() == ERROR_ALREADY_EXISTS)
                    SetEvent(sigInstallUpdates);
            }
        }
        else {
            ShellExecuteW(NULL, L"open", lParam, L"", NULL, SW_SHOWNORMAL);
        }

        return S_FALSE;
    }

    return S_OK;
}

static DWORD InformUserAboutCrash(LPVOID msg)
{
    TASKDIALOG_BUTTON buttons[1];
    buttons[0].nButtonID     = IDNO;
    buttons[0].pszButtonText = L"Dismiss";

    TASKDIALOGCONFIG td = {
        .cbSize             = sizeof(TASKDIALOGCONFIG),
        .hInstance          = hModule,
        .hwndParent         = NULL,
        .dwFlags            = TDF_SIZE_TO_CONTENT | TDF_ENABLE_HYPERLINKS,
        .pszWindowTitle     = L"ExplorerPatcher",
        .pszMainInstruction = L"Unfortunately, File Explorer is crashing :(",
        .pszContent         = msg,
        .cButtons           = sizeof buttons / sizeof buttons[0],
        .pButtons           = buttons,
        .cRadioButtons      = 0,
        .pRadioButtons      = NULL,
        .cxWidth            = 0,
        .pfCallback         = InformUserAboutCrashCallback,
    };

    HRESULT (*pfTaskDialogIndirect)(const TASKDIALOGCONFIG *, int *, int *, BOOL *) = NULL;
    HMODULE hComCtl32 = NULL;
    int     res       = td.nDefaultButton;

    if (!(hComCtl32 = GetModuleHandleA("Comctl32.dll")) ||
        !(pfTaskDialogIndirect = GetProcAddress(hComCtl32, "TaskDialogIndirect")) ||
        FAILED(pfTaskDialogIndirect(&td, &res, NULL, NULL)))
    {
        wcscat_s(msg, 10000, L" Would you like to open the ExplorerPatcher status web page on GitHub in your default browser?");
        res = MessageBoxW(NULL, msg, L"ExplorerPatcher", MB_ICONASTERISK | MB_YESNO);
    }
    if (res == IDYES)
        ShellExecuteW(NULL, L"open",
                      L"https://github.com/valinet/ExplorerPatcher/discussions/1102",
                      L"", NULL, SW_SHOWNORMAL);
    free(msg);
    return 0;
}

static DWORD WINAPI ClearCrashCounter(INT64 timeout)
{
    Sleep(timeout);
    DWORD zero = 0;
    RegSetKeyValueW(HKEY_CURRENT_USER, L"" REGPATH, L"CrashCounter", REG_DWORD, &zero, sizeof(DWORD));
    return 0;
}
#endif

#define DLL_INJECTION_METHOD_DXGI            0
#define DLL_INJECTION_METHOD_COM             1
#define DLL_INJECTION_METHOD_START_INJECTION 2

static HRESULT EntryPoint(DWORD dwMethod)
{
    if (bInstanced)
        return E_NOINTERFACE;

    //extern void ExplorerPatcher_BasicIOWatchdog(void);
    //ExplorerPatcher_BasicIOWatchdog();

    InitializeGlobalVersionAndUBR();

    WCHAR exePath[MAX_PATH], dllName[MAX_PATH];
    GetModuleFileNameW(hModule, dllName, MAX_PATH);
    PathStripPathW(dllName);
    BOOL bIsDllNameDXGI = WStrIEq(dllName, L"dxgi.dll");
    if (dwMethod == DLL_INJECTION_METHOD_DXGI && !bIsDllNameDXGI)
        return E_NOINTERFACE;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
    if (!hProcess)
        return E_NOINTERFACE;
    DWORD dwLength = MAX_PATH;
    QueryFullProcessImageNameW(hProcess, 0, exePath, &dwLength);
    CloseHandle(hProcess);

    WCHAR wszSearchIndexerPath[MAX_PATH];
    GetSystemDirectoryW(wszSearchIndexerPath, MAX_PATH);
    wcscat_s(wszSearchIndexerPath, MAX_PATH, L"\\SearchIndexer.exe");
    if (WStrIEq(exePath, wszSearchIndexerPath))
        return E_NOINTERFACE;

    WCHAR wszExplorerExpectedPath[MAX_PATH];
    GetWindowsDirectoryW(wszExplorerExpectedPath, MAX_PATH);
    wcscat_s(wszExplorerExpectedPath, MAX_PATH, L"\\explorer.exe");
    BOOL bIsThisExplorer = WStrIEq(exePath, wszExplorerExpectedPath);

    WCHAR wszStartExpectedPath[MAX_PATH];
    GetWindowsDirectoryW(wszStartExpectedPath, MAX_PATH);
    wcscat_s(wszStartExpectedPath, MAX_PATH,
             L"\\SystemApps\\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\\StartMenuExperienceHost.exe");
    BOOL bIsThisStartMEH = WStrIEq(exePath, wszStartExpectedPath);

    WCHAR wszShellExpectedPath[MAX_PATH];
    GetWindowsDirectoryW(wszShellExpectedPath, MAX_PATH);
    wcscat_s(wszShellExpectedPath, MAX_PATH,
             L"\\SystemApps\\ShellExperienceHost_cw5n1h2txyewy\\ShellExperienceHost.exe");
    BOOL bIsThisShellEH = WStrIEq(exePath, wszShellExpectedPath);

    if (dwMethod == DLL_INJECTION_METHOD_DXGI) {
        if (!(bIsThisExplorer || bIsThisStartMEH || bIsThisShellEH))
            return E_NOINTERFACE;
        WCHAR wszRealDXGIPath[MAX_PATH];
        GetSystemDirectoryW(wszRealDXGIPath, MAX_PATH);
        wcscat_s(wszRealDXGIPath, MAX_PATH, L"\\dxgi.dll");
#ifdef _WIN64
        SetupDXGIImportFunctions(LoadLibraryW(wszRealDXGIPath));
#endif
    }

    if (dwMethod == DLL_INJECTION_METHOD_COM && (bIsThisExplorer || bIsThisStartMEH || bIsThisShellEH))
        return E_NOINTERFACE;
    if (dwMethod == DLL_INJECTION_METHOD_START_INJECTION && !bIsThisStartMEH)
        return E_NOINTERFACE;

    bIsExplorerProcess = bIsThisExplorer;
    if (bIsThisExplorer) {
        BOOL desktopExists = IsDesktopWindowAlreadyPresent();
#ifdef _WIN64
        if (!desktopExists) {
            DWORD crashCounterDisabled = 0, crashCounter = 0, crashThresholdTime = 10000, crashCounterThreshold = 3,
                  dwTCSize = sizeof(DWORD);

            RegGetValueW(HKEY_CURRENT_USER, L"" REGPATH, L"CrashCounterDisabled",
                         RRF_RT_DWORD, NULL, &crashCounterDisabled, &dwTCSize);

            dwTCSize = sizeof(DWORD);
            if (crashCounterDisabled != 0 && crashCounterDisabled != 1)
                crashCounterDisabled = 0;
            RegGetValueW(HKEY_CURRENT_USER, L"" REGPATH, L"CrashCounter",
                         RRF_RT_DWORD, NULL, &crashCounter, &dwTCSize);

            dwTCSize = sizeof(DWORD);
            if (crashCounter < 0)
                crashCounter = 0;
            RegGetValueW(HKEY_CURRENT_USER, L"" REGPATH, L"CrashThresholdTime",
                         RRF_RT_DWORD, NULL, &crashThresholdTime, &dwTCSize);

            dwTCSize = sizeof(DWORD);
            if (crashThresholdTime < 100 || crashThresholdTime > 60000)
                crashThresholdTime = 10000;
            RegGetValueW(HKEY_CURRENT_USER, L"" REGPATH, L"CrashCounterThreshold",
                         RRF_RT_DWORD, NULL, &crashCounterThreshold, &dwTCSize);

            dwTCSize = sizeof(DWORD);
            if (crashCounterThreshold <= 1 || crashCounterThreshold >= 10)
                crashCounterThreshold = 3;

            if (!crashCounterDisabled) {
                if (crashCounter >= crashCounterThreshold) {
                    crashCounter = 0;
                    RegSetKeyValueW(HKEY_CURRENT_USER, L"" REGPATH, L"CrashCounter", REG_DWORD, &crashCounter, sizeof(DWORD));
                    wchar_t times[100];
                    ZeroMemory(times, sizeof(wchar_t) * 100);
                    swprintf_s(times, 100, crashCounterThreshold == 1 ? L"once" : L"%d times", crashCounterThreshold);
                    wchar_t uninstallLink[MAX_PATH];
                    ZeroMemory(uninstallLink, sizeof(wchar_t) * MAX_PATH);
                    uninstallLink[0] = L'\'';
                    SHGetFolderPathW(NULL, SPECIAL_FOLDER, NULL, SHGFP_TYPE_CURRENT, uninstallLink + 1);
                    wcscat_s(uninstallLink, MAX_PATH, APP_RELATIVE_PATH L"\\" SETUP_UTILITY_NAME L"' /uninstall");
                    wchar_t *msg = calloc(sizeof(wchar_t), 10000);
                    swprintf_s(
                        msg, 10000,
                        L"It seems that File Explorer closed unexpectedly %s in less than %d seconds each "
                        L"time when starting up. "
                        L"This might indicate a problem caused by ExplorerPatcher, which might be unaware of "
                        L"recent changes in Windows, for example "
                        L"when running on a new OS build.\n"
                        L"Here are a few recommendations:\n"
                        L"\u2022 If an updated version is available, you can <A "
                        L"HREF=\"eplink://update\">update ExplorerPatcher and restart File Explorer</A>.\n"
                        L"\u2022 On GitHub, you can <A "
                        L"HREF=\"https://github.com/valinet/ExplorerPatcher/releases\">view releases</A>, <A "
                        L"HREF=\"https://github.com/valinet/ExplorerPatcher/discussions/1102\">check the "
                        L"current status</A>, <A "
                        L"HREF=\"https://github.com/valinet/ExplorerPatcher/discussions\">discuss</A> or <A "
                        L"HREF=\"https://github.com/valinet/ExplorerPatcher/issues\">review the latest "
                        L"issues</A>.\n"
                        L"\u2022 If you suspect this is not caused by ExplorerPatcher, please uninstall any "
                        L"recently installed shell extensions or similar utilities.\n"
                        L"\u2022 If no fix is available for the time being, you can <A HREF=\"%s\">uninstall "
                        L"ExplorerPatcher</A>, and then later reinstall it when a fix is published on "
                        L"GitHub. Rest assured, even if you uninstall, your program configuration will be "
                        L"preserved.\n"
                        L"\n"
                        L"I am sorry for the inconvenience this might cause; I am doing my best to try to "
                        L"keep this program updated and working.\n\n"
                        L"ExplorerPatcher is disabled until the next File Explorer restart, in order to "
                        L"allow you to perform maintenance tasks and take the necessary actions.",
                        times, crashThresholdTime / 1000, uninstallLink
                    );
                    SHCreateThread(InformUserAboutCrash, msg, 0, NULL);
                    IncrementDLLReferenceCount(hModule);
                    bInstanced = TRUE;
                    return E_NOINTERFACE;
                }
                crashCounter++;
                RegSetKeyValueW(HKEY_CURRENT_USER, L"" REGPATH, L"CrashCounter", REG_DWORD, &crashCounter, sizeof(DWORD));
                SHCreateThread(ClearCrashCounter, crashThresholdTime, 0, NULL);
            }
        }
#endif
        Inject(!desktopExists);
        IncrementDLLReferenceCount(hModule);
        bInstanced = TRUE;
    } else if (bIsThisStartMEH) {
        InjectStartMenu();
        IncrementDLLReferenceCount(hModule);
        bInstanced = TRUE;
    } else if (bIsThisShellEH) {
        if (IsWindows11Version22H2OrHigher())
            InjectShellExperienceHostFor22H2OrHigher();
        else if (IsWindows11())
            InjectShellExperienceHost();
        IncrementDLLReferenceCount(hModule);
        bInstanced = TRUE;
    } else if (dwMethod == DLL_INJECTION_METHOD_COM) {
        Inject(FALSE);
        IncrementDLLReferenceCount(hModule);
        bInstanced = TRUE;
    }

    return E_NOINTERFACE;
}

#ifdef _WIN64
# pragma comment(linker, "/export:DllGetClassObject=_DllGetClassObject")

// for explorer.exe and ShellExperienceHost.exe
__declspec(dllexport) HRESULT
DXGIDeclareAdapterRemovalSupport(void)
{
    EntryPoint(DLL_INJECTION_METHOD_DXGI);
    return DXGIDeclareAdapterRemovalSupportFunc();
}

// for StartMenuExperienceHost.exe via DXGI
__declspec(dllexport) HRESULT
CreateDXGIFactory1(void *p1, void **p2)
{
    EntryPoint(DLL_INJECTION_METHOD_DXGI);
    return CreateDXGIFactory1Func(p1, p2);
}

// for StartMenuExperienceHost.exe via injection from explorer
HRESULT
InjectStartFromExplorer(void)
{
    EntryPoint(DLL_INJECTION_METHOD_START_INJECTION);
    return HRESULT_FROM_WIN32(GetLastError());
}

#else
# pragma comment(linker, "/export:DllGetClassObject=__DllGetClassObject@12")
#endif

// for everything else
HRESULT WINAPI
_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return EntryPoint(DLL_INJECTION_METHOD_COM);
}


BOOL WINAPI
DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        hModule = hinstDLL;
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
