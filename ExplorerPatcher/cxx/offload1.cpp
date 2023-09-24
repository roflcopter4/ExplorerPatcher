#include "Common/Common.h"
#include "cxx_util.hh"

#include <winstring.h>
#include <roapi.h>
#include <inspectable.h>
#include <Windows.ApplicationModel.Contacts.h>

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

extern "C" void
ExplorerPatcher_EnsureXAML(void)
{
    using ABI::Windows::UI::Core::ICoreWindow5;
    using ABI::Windows::System::IDispatcherQueue;

    static std::atomic_flag flg;
    if (flg.test_and_set())
        return;

    HStringWrapper      hstringXamlApplication;
    HStringWrapper      hstringWindowsXamlManager;
    IInspectable       *pXamlApplication          = nullptr;
    IActivationFactory *pUIXamlApplicationFactory = nullptr;
    ICoreWindow5       *pCoreWindow5              = nullptr;
    IDispatcherQueue   *pDispatcherQueue          = nullptr;

    HRESULT res = hstringXamlApplication.makeRef(L"Windows.Internal.Shell.XamlExplorerHost.XamlApplication");
    if (FAILED(res) || !hstringXamlApplication)
    {
        wprintf(L"Error (%lX) in sub_1800135EC on WindowsCreateStringReference.\n", res);
        return;
    }

    res = RoGetActivationFactory(hstringXamlApplication,
                                 uuidof_Windows_Internal_Shell_XamlExplorerHost_IXamlApplicationStatics,
                                 reinterpret_cast<void **>(&pUIXamlApplicationFactory));
    if (FAILED(res) || !pUIXamlApplicationFactory) {
        wprintf(L"Error in sub_1800135EC on RoGetActivationFactory (0x%08lX): \"%hs\".\n",
                res, std::error_code{res, std::system_category()}.message().c_str());
        return;
    }
    pUIXamlApplicationFactory->ActivateInstance(&pXamlApplication); // this is get_Current

    if (!pXamlApplication) {
        wprintf(L"Error in sub_1800135EC on pUIXamlApplicationFactory + 48.\n");
        goto cleanup1;
    }

    res = hstringWindowsXamlManager.makeRef(L"Windows.UI.Xaml.Hosting.WindowsXamlManager");
    if (FAILED(res) || !hstringWindowsXamlManager) {
        wprintf(L"Error in sub_1800135EC on WindowsCreateStringReference 2.\n");
        goto cleanup2;
    }

    res = RoGetActivationFactory(hstringWindowsXamlManager, uuidof_Windows_UI_Core_ICoreWindow5,
                                 reinterpret_cast<void **>(&pCoreWindow5));
    if (FAILED(res) || !pCoreWindow5) {
        wprintf(L"Error (%lX) in sub_1800135EC on RoGetActivationFactory 2.\n", res);
        goto cleanup2;
    }

    pCoreWindow5->get_DispatcherQueue(&pDispatcherQueue);
    if (!pDispatcherQueue) {
        wprintf(L"Error (%lX) in sub_1800135EC on pCoreWindow5 + 48.\n", res);
    }

    // Keep pDispatcherQueue referenced in memory
    pCoreWindow5->Release();
cleanup2:
    pXamlApplication->Release();
cleanup1:
    pUIXamlApplicationFactory->Release();
}


/****************************************************************************************/
} // namespace ExplorerPatcher
