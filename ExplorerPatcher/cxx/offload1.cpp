#include "Common/Common.h"

#include <winstring.h>
#include <roapi.h>
#include <inspectable.h>

namespace ExplorerPatcher {
/****************************************************************************************/


static constexpr GUID uuidof_Windows_Internal_Shell_XamlExplorerHost_IXamlApplicationStatics = {
    0x5148D7B1, 0x800E, 0x5C86,
    {0x8F, 0x69, 0x55, 0x81, 0x97, 0x48, 0x31, 0x23}
};

static constexpr GUID uuidof_Windows_UI_Core_ICoreWindow5 = {
    0x28258A12, 0x7D82, 0x505B,
    {0xB2, 0x10, 0x71, 0x2B, 0x04, 0xA5, 0x88, 0x82}
};

template <size_t N>
__forceinline static HRESULT
makeStringRef(wchar_t const (&sourceString)[N], HSTRING_HEADER &header, HSTRING &string)
{
    return ::WindowsCreateStringReference(
        sourceString, static_cast<UINT32>(std::size(sourceString) - 1), &header, &string);
}

extern "C" void ExplorerPatcher_EnsureXAML(void)
{
    using get_func = void (__fastcall *)(void *, void **);

    static std::atomic_flag flg;
    if (flg.test_and_set())
        return;

    ULONGLONG initTime = GetTickCount64();
    ULONGLONG finalTime;
    get_func  func;

    HSTRING_HEADER hstringheaderXamlApplication;
    HSTRING_HEADER hstringheaderWindowsXamlManager;
    HSTRING        hstringXamlApplication    = nullptr;
    HSTRING        hstringWindowsXamlManager = nullptr;
    IInspectable  *pUIXamlApplicationFactory = nullptr;
    IInspectable  *pCoreWindow5              = nullptr;
    IUnknown      *pXamlApplication          = nullptr;
    IUnknown      *pDispatcherQueue          = nullptr;

    HRESULT res = makeStringRef(L"Windows.Internal.Shell.XamlExplorerHost.XamlApplication",
                                hstringheaderXamlApplication, hstringXamlApplication);
    if (FAILED(res) || !hstringXamlApplication)
    {
        wprintf(L"Error in sub_1800135EC on WindowsCreateStringReference.\n");
        return;
    }

    res = RoGetActivationFactory(hstringXamlApplication,
                                 uuidof_Windows_Internal_Shell_XamlExplorerHost_IXamlApplicationStatics,
                                 reinterpret_cast<void **>(&pUIXamlApplicationFactory));
    if (FAILED(res) || !pUIXamlApplicationFactory) {
        char *errmsg = ExplorerPatcher_GetWin32ErrorMessage(res);
        wprintf(L"Error in sub_1800135EC on RoGetActivationFactory (0x%08X): \"%hs\".\n",
                res, errmsg);
        free(errmsg);
        goto cleanup0;
    }

    func = *reinterpret_cast<get_func *>(*reinterpret_cast<UINT_PTR *>(pUIXamlApplicationFactory) + 48);
    func(pUIXamlApplicationFactory, reinterpret_cast<void **>(&pXamlApplication)); // get_Current

    if (!pXamlApplication) {
        wprintf(L"Error in sub_1800135EC on pUIXamlApplicationFactory + 48.\n");
        goto cleanup1;
    }
    pXamlApplication->Release();

    res = makeStringRef(L"Windows.UI.Xaml.Hosting.WindowsXamlManager",
                        hstringheaderWindowsXamlManager, hstringWindowsXamlManager);
    if (FAILED(res) || !hstringWindowsXamlManager) {
        wprintf(L"Error in sub_1800135EC on WindowsCreateStringReference 2.\n");
        goto cleanup1;
    }

    res = RoGetActivationFactory(hstringWindowsXamlManager, uuidof_Windows_UI_Core_ICoreWindow5,
                                 reinterpret_cast<void **>(&pCoreWindow5));
    if (FAILED(res) || !pCoreWindow5) {
        wprintf(L"Error in sub_1800135EC on RoGetActivationFactory 2.\n");
        goto cleanup2;
    }

    func = *reinterpret_cast<get_func *>(*reinterpret_cast<UINT_PTR *>(pCoreWindow5) + 48);
    func(pCoreWindow5, reinterpret_cast<void **>(&pDispatcherQueue)); // get_DispatcherQueue
    if (!pDispatcherQueue) {
        wprintf(L"Error in sub_1800135EC on pCoreWindow5 + 48.\n");
        goto cleanup3;
    }
    // Keep pDispatcherQueue referenced in memory
    //finalTime = GetTickCount64();
    //wprintf(L"EnsureXAML %llu ms.\n", finalTime - initTime);

cleanup3:
    if (pCoreWindow5)
        pCoreWindow5->Release();
cleanup2:
    if (hstringWindowsXamlManager)
        WindowsDeleteString(hstringWindowsXamlManager);
cleanup1:
    if (pUIXamlApplicationFactory)
        pUIXamlApplicationFactory->Release();
cleanup0:
    if (hstringXamlApplication)
        WindowsDeleteString(hstringXamlApplication);
}


/****************************************************************************************/
} // namespace ExplorerPatcher
