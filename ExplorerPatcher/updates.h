#pragma once
#ifndef _H_UPDATES_H_
#define _H_UPDATES_H_
#include <Windows.h>
#include <stdio.h>
#include <Wininet.h>
#pragma comment(lib, "Wininet.lib")
#include <shlobj_core.h>
#include "utility.h"

extern HMODULE hModule;

#define UPDATES_VERBOSE_OUTPUT

#define UPDATE_POLICY_AUTO 0
#define UPDATE_POLICY_NOTIFY 1
#define UPDATE_POLICY_MANUAL 2
#define UPDATE_POLICY_DEFAULT UPDATE_POLICY_NOTIFY

#define UPDATES_OP_DEFAULT 0
#define UPDATES_OP_CHECK 1
#define UPDATES_OP_INSTALL 2

#define UPDATES_USER_AGENT "ExplorerPatcher"
#define UPDATES_FORM_HEADERS "Content-Type: text/plain;\r\n"
#define UPDATES_HASH_SIZE 32
#define UPDATES_BUFSIZ 10240
#define UPDATES_DEFAULT_TIMEOUT 600

#define UPDATES_RELEASE_INFO_URL          "https://github.com/valinet/ExplorerPatcher"
#define UPDATES_RELEASE_INFO_URL_STABLE   "https://github.com/valinet/ExplorerPatcher/releases/latest"
#define UPDATES_RELEASE_INFO_URL_STAGING  "https://api.github.com/repos/valinet/ExplorerPatcher/releases?per_page=1"

struct IsUpdateAvailableParameters
{
	HINTERNET hInternet;
	HANDLE hEvent;
};

BOOL IsUpdatePolicy(LPCWSTR wszDataStore, DWORD dwUpdatePolicy);
BOOL ShowUpdateSuccessNotification(
	HMODULE hModule,
	__x_ABI_CWindows_CUI_CNotifications_CIToastNotifier* notifier,
	__x_ABI_CWindows_CUI_CNotifications_CIToastNotificationFactory* notifFactory,
	__x_ABI_CWindows_CUI_CNotifications_CIToastNotification** toast
);
BOOL InstallUpdatesIfAvailable(
	HMODULE hModule,
	__x_ABI_CWindows_CUI_CNotifications_CIToastNotifier* notifier,
	__x_ABI_CWindows_CUI_CNotifications_CIToastNotificationFactory* notifFactory,
	__x_ABI_CWindows_CUI_CNotifications_CIToastNotification** toast,
	DWORD dwOperation, DWORD bAllocConsole, DWORD dwUpdatePolicy
);
#endif