#pragma once
#ifndef _H_SETTINGSMONITOR_H_
#define _H_SETTINGSMONITOR_H_

#include <Windows.h>
#include <Shlwapi.h>
#include <stdio.h>
#pragma comment(lib, "Shlwapi.lib")

typedef struct Setting {
    HKEY   origin;
    HKEY   hKey;
    HANDLE hEvent;
    void(__stdcall *callback)(void *);
    void   *data;
    wchar_t name[MAX_PATH];
} Setting;

typedef struct SettingsChangeParameters {
    HANDLE   hThread;
    Setting *settings;
    DWORD    size;
} SettingsChangeParameters;

extern DWORD WINAPI MonitorSettings(SettingsChangeParameters*);

#endif
