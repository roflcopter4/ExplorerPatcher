#pragma once
#ifndef BpYyHnv5XZtD3nxVVCSWbNFL4sUGH5Q8Tbh4RUfz9fhW4TinQLvE1vQLvWOQy0
#define BpYyHnv5XZtD3nxVVCSWbNFL4sUGH5Q8Tbh4RUfz9fhW4TinQLvE1vQLvWOQy0 //NOLINT

#include "Common/Common.h"
#include <winstring.h>

namespace ExplorerPatcher {
/****************************************************************************************/


#define DELETE_COPY_CONSTRUCTORS(type)      \
    type (type const &)           = delete; \
    type &operator=(type const &) = delete
#define DELETE_MOVE_CONSTRUCTORS(type)          \
    type (type  &&) noexcept          = delete; \
    type &operator=(type &&) noexcept = delete
#define DELETE_ALL_CONSTRUCTORS(type) \
    DELETE_COPY_CONSTRUCTORS(type);   \
    DELETE_MOVE_CONSTRUCTORS(type)

#define DEFAULT_COPY_CONSTRUCTORS(type)      \
    type (type const &)           = default; \
    type &operator=(type const &) = default
#define DEFAULT_MOVE_CONSTRUCTORS(type)          \
    type (type  &&) noexcept          = default; \
    type &operator=(type &&) noexcept = default
#define DEFAULT_ALL_CONSTRUCTORS(type) \
    DEFAULT_COPY_CONSTRUCTORS(type);   \
    DEFAULT_MOVE_CONSTRUCTORS(type)

/*--------------------------------------------------------------------------------------*/

class HStringWrapper
{
  public:
    HStringWrapper() = default;

    ~HStringWrapper() noexcept
    {
        (void)WindowsDeleteString(string_);
    }

    HStringWrapper(HStringWrapper &&other) noexcept
    {
        (void)WindowsDeleteString(string_);
        string_  = other.string_;
        header_  = other.header_;
        (void)other.clear();
    }

    HStringWrapper &operator=(HStringWrapper &&other) noexcept
    {
        (void)WindowsDeleteString(string_);
        string_ = other.string_;
        header_ = other.header_;
        (void)other.clear();
        return *this;
    }

    HStringWrapper(HStringWrapper const &)            = delete;
    HStringWrapper &operator=(HStringWrapper const &) = delete;

    //------------------------------------------------------------------------------------

    template <size_t N>
    HRESULT makeRef(wchar_t const (&sourceString)[N])
    {
        if (string_)
            (void)clear();
        return ::WindowsCreateStringReference(
            sourceString, static_cast<UINT32>(std::size(sourceString) - 1),
            &header_, &string_);
    }

    HRESULT clear() noexcept
    {
        HRESULT res = WindowsDeleteString(string_);
        string_     = nullptr;
        header_     = {};
        return res;
    }

    [[nodiscard]] HSTRING const &get() const & { return string_; }

    operator HSTRING() const { return string_; }

  private:
    HSTRING        string_ = nullptr;
    HSTRING_HEADER header_  = {};
};


/****************************************************************************************/
} // namespace ExplorerPatcher
#endif