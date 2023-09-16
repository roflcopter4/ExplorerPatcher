#include "ArchiveMenu.h"

#include <Shlwapi.h>
#include <stdio.h>

DWORD ArchiveMenuThread(ArchiveMenuThreadParams const *params)
{
    static ATOM windowRegistrationAtom = 0;

    Sleep(1000);
    wprintf(L"Started \"Archive menu\" thread.\n");

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return 0;

    if (windowRegistrationAtom == 0) {
        WNDCLASS wc = {
            .style         = CS_DBLCLKS,
            .lpfnWndProc   = params->wndProc,
            .hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH),
            .hInstance     = GetModuleHandleW(NULL),
            .lpszClassName = L"ArchiveMenuWindowExplorer",
            .hCursor       = LoadCursorW(NULL, IDC_ARROW),
        };
        ATOM tmp = RegisterClassW(&wc);
        if (tmp != 0)
            windowRegistrationAtom = tmp;
    }

    *params->hWnd = params->CreateWindowInBand(
        0, windowRegistrationAtom,
        L"ArchiveMenuWindowExplorer",
        WS_POPUP, 0, 0, 0, 0,
        NULL, NULL, GetModuleHandleW(NULL),
        NULL, 7
    );
    if (!*params->hWnd)
        return 0;

    ITaskbarList *pTaskList = NULL;
    hr = CoCreateInstance(&__uuidof_TaskbarList, NULL, CLSCTX_ALL,
                          &__uuidof_ITaskbarList, (void **)&pTaskList);
    if (FAILED(hr))
        return 0;
    hr = pTaskList->lpVtbl->HrInit(pTaskList);
    if (FAILED(hr))
        return 0;
    ShowWindow(*(params->hWnd), SW_SHOW);
    hr = pTaskList->lpVtbl->DeleteTab(pTaskList, *(params->hWnd));
    if (FAILED(hr))
        return 0;
    ULONG ulr = pTaskList->lpVtbl->Release(pTaskList);
    if (FAILED(ulr))
        return 0;

    MSG msg = {0};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    wprintf(L"Ended \"Archive menu\" thread.\n");
    return 1;
}

typedef INT64 (*ApplyOwnerDrawToMenuFunc_t)   (HMENU, HMENU, HWND, UINT, LPVOID);
typedef void  (*RemoveOwnerDrawFromMenuFunc_t)(HMENU, HMENU, HWND);

LRESULT CALLBACK ArchiveMenuWndProc(
    _In_ HWND   hWnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    ApplyOwnerDrawToMenuFunc_t    ApplyOwnerDrawToMenuFunc,
    RemoveOwnerDrawFromMenuFunc_t RemoveOwnerDrawFromMenuFunc)
{
    switch (uMsg) {
    case WM_COPYDATA: {
        COPYDATASTRUCT *st = (COPYDATASTRUCT *)lParam;
        POINT pt;
        HWND  prevhWnd = GetForegroundWindow();
        HMENU hMenu    = CreatePopupMenu();
        GetCursorPos(&pt);
        SetForegroundWindow(hWnd);

        WCHAR buffer[MAX_PATH + 100];
        WCHAR filename[MAX_PATH];
        wcscpy_s(filename, ARRAYSIZE(filename), st->lpData);

        PathUnquoteSpacesW(filename);
        PathRemoveExtensionW(filename);
        PathStripPathW(filename);
        swprintf_s(buffer, ARRAYSIZE(buffer), EXTRACT_NAME, filename);

        InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, 1, buffer);
        InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, 2, OPEN_NAME);

        INT64 *unknown_array = calloc(4, sizeof(INT64));
        ApplyOwnerDrawToMenuFunc(hMenu, (HMENU)hWnd, (HWND)&pt, 0xC, unknown_array);
        BOOL res = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x - 15, pt.y - 15, 0, hWnd, NULL);
        RemoveOwnerDrawFromMenuFunc(hMenu, (HMENU)hWnd, (HWND)&pt);
        free(unknown_array);
        SetForegroundWindow(prevhWnd);

        if (res == 1 || res == 2) {
            memset(buffer, 0, sizeof buffer);

            if (res == 1) {
                WCHAR path[MAX_PATH];
                WCHAR path_orig[MAX_PATH];
                wcscpy_s(path, ARRAYSIZE(path), st->lpData);
                wcscpy_s(path_orig, ARRAYSIZE(path_orig), st->lpData);
                PathUnquoteSpacesW(path_orig);
                PathRemoveExtensionW(path_orig);
                swprintf_s(buffer, ARRAYSIZE(buffer), EXTRACT_CMD, path_orig, path);
                //wprintf(L"%s\n%s\n\n", st->lpData, buffer);
            } else if (res == 2) {
                swprintf_s(buffer, ARRAYSIZE(buffer), OPEN_CMD, (wchar_t *)st->lpData);
                //wprintf(L"%s\n%s\n\n", st->lpData, buffer);
            }

            STARTUPINFO si = {.cb = sizeof(si)};
            PROCESS_INFORMATION pi;
            BOOL b = CreateProcessW(NULL, buffer, NULL, NULL, TRUE,
                                    CREATE_UNICODE_ENVIRONMENT,
                                    NULL, NULL, &si, &pi);
            if (b) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            } else {
                wprintf(L"Error starting process \"%ls\", code %lu\n", buffer, GetLastError());
            }
        }

        DestroyMenu(hMenu);
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    }

    case WM_CLOSE:
        return 0;

    default:
        return 1;
    }
}
