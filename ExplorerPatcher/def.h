#pragma once
#ifndef Ez5nSHoAripiQBYfkuWxWcXnlWwxuk
#define Ez5nSHoAripiQBYfkuWxWcXnlWwxuk

#define APPID                  "Microsoft.Windows.Explorer"
#define PRODUCT_NAME           "ExplorerPatcher"
#define PRODUCT_PUBLISHER      "VALINET Solutions SRL"
#define APP_RELATIVE_PATH      "\\" PRODUCT_NAME
#define SETUP_UTILITY_NAME     "ep_setup.exe"
#define REGPATH                "SOFTWARE\\ExplorerPatcher"
#define REGPATH_OLD            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ExplorerPatcher"
#define SEH_REGPATH            "Control Panel\\Quick Actions\\Control Center\\QuickActionsStateCapture\\ExplorerPatcher"
#define REGPATH_STARTMENU      REGPATH_OLD
#define SPECIAL_FOLDER         CSIDL_PROGRAM_FILES
#define SPECIAL_FOLDER_LEGACY  CSIDL_APPDATA
#define EP_CLSID               "{D17F1E1A-5919-4427-8F89-A1A8503CA3EB}"
#define EP_CLSID_LITE          "D17F1E1A-5919-4427-8F89-A1A8503CA3EB"
#define EP_DWM_EVENTNAME       "Global\\ep_dwm_2_" EP_CLSID_LITE
#define EP_DWM_SERVICENAME     "ep_dwm_" EP_CLSID_LITE
#define EP_SETUP_HELPER_SWITCH "/CreateExplorerShellUnelevatedAfterServicing"
#define EP_SETUP_EVENTNAME     "Global\\ep_setup_" EP_CLSID_LITE

#define DOSMODE_OFFSET 78
#define TOAST_BUFSIZ   1024

#endif