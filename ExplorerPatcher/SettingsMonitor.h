#pragma once
#ifndef _H_SETTINGSMONITOR_H_
#define _H_SETTINGSMONITOR_H_

#include <Windows.h>
#include <Shlwapi.h>
#include <stdio.h>
#pragma comment(lib, "Shlwapi.lib")

typedef struct Setting {
    void(CALLBACK *callback)(void *);
    void   *data;
    HANDLE  hEvent;
    HKEY    hKey;
    HKEY    origin;
    LPCWSTR name;
} Setting;

typedef struct SettingsChangeParameters {
    HANDLE   hThread;
    Setting *settings;
    DWORD    size;
} SettingsChangeParameters;

extern DWORD WINAPI MonitorSettings(SettingsChangeParameters*);

#endif
