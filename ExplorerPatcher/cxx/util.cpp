#include "Common/Common.h"
#include <system_error>

namespace ExplorerPatcher {
/****************************************************************************************/


_Check_return_ extern "C" char *
ExplorerPatcher_GetWin32ErrorMessage(DWORD error)
{
    auto const code = std::error_code{static_cast<int>(error), std::system_category()};
    return ::strdup(code.message().c_str());
}


/****************************************************************************************/
} // namespace ExplorerPatcher
