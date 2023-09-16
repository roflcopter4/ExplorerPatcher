#pragma once
#ifndef _H_ARCHIVEMENU_H_
#define _H_ARCHIVEMENU_H_

#include <initguid.h>
#include <Windows.h>
#include <Shlobj_core.h>

#define OPEN_NAME    L"&Open archive"
#define EXTRACT_NAME L"&Extract to \"%ls\\\""
#define OPEN_CMD     L"\"C:\\Program Files\\7-Zip\\7zFM.exe\" %ls"
#define EXTRACT_CMD  L"\"C:\\Program Files\\7-Zip\\7zG.exe\" x -o\"%ls\" -spe %ls"

DEFINE_GUID(__uuidof_TaskbarList,
    0x56FDF344,
    0xFD6D, 0x11d0, 0x95, 0x8A,
    0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90
);
DEFINE_GUID(__uuidof_ITaskbarList,
    0x56FDF342,
    0xFD6D, 0x11d0, 0x95, 0x8A,
    0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90
);

typedef struct ArchiveMenuThreadParams
{
    HWND* hWnd;
	WNDPROC wndProc;
    HWND(WINAPI* CreateWindowInBand)(
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
}     ArchiveMenuThreadParams;

extern DWORD ArchiveMenuThread(ArchiveMenuThreadParams const* params);

extern LRESULT CALLBACK ArchiveMenuWndProc(
    _In_ HWND   hWnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    INT64(*ApplyOwnerDrawToMenuFunc)(
        HMENU h1,
        HMENU h2,
        HWND a3,
        unsigned int a4,
        void* data
        ),
    void(*RemoveOwnerDrawFromMenuFunc)(
        HMENU _this,
        HMENU hWnd,
        HWND a3
        )
);

#endif
