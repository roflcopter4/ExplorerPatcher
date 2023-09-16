/*
 * Replacement function for calculating a file hash, as a string.
 */

#include "Common/Common.h"

#include <Windows.h>
#include <bcrypt.h>
#include <cstdlib>
#include <cwchar>
#include <filesystem>


[[nodiscard]] static INT64
read_all_bytes(HANDLE handle, BYTE *buf, INT64 nbytes)
{
    INT64 total = 0;
    DWORD n;

    do {
        if (!::ReadFile(handle, buf + total, DWORD(nbytes - total), &n, nullptr))
            break;
    } while (INT64(n) != INT64_C(-1) && (total += n) < nbytes);

    return total;
}

[[nodiscard]] static BYTE *
slurp_file(wchar_t const *filename, INT64 *nread_ptr)
{
    HANDLE handle = ::CreateFileW(filename, GENERIC_READ, 0, nullptr,
                                  OPEN_EXISTING, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        wprintf(L"Failed to open file \"%ls\": %lX\n", filename, err);
        if (nread_ptr)
            *nread_ptr = GetLastError();
        return nullptr;
    }

    auto  size  = std::filesystem::file_size(filename);
    auto *ptr   = new BYTE[size + 1];
    auto  nread = read_all_bytes(handle, ptr, static_cast<INT64>(size));
    ptr[nread]  = '\0';

    if (nread_ptr)
        *nread_ptr = nread;
    if (nread < 0 || static_cast<size_t>(nread) != size) {
        DWORD err = ::GetLastError();
        wprintf(L"Error reading file \"%ls\": Read only %zd bytes of %zd (%lX).\n",
                filename, nread, size, err);
    }
    ::CloseHandle(handle);
    return ptr;
}


extern "C" NTSTATUS
ExplorerPatcher_ComputeFileHash(
    _In_                     LPCWSTR filename,
    _Out_writes_z_(hashSize) LPSTR   hash,
    _In_                     SIZE_T  hashSize)
{
#define FAIL(Status) (status = (static_cast<NTSTATUS>(Status) < 0))

    static constexpr char hexDigits[] = "0123456789abcdef";

    BCRYPT_ALG_HANDLE  hAlg  = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    NTSTATUS status;
    INT64    data_len     = 0;
    DWORD    cbData       = 0;
    DWORD    cbHash       = 0;
    DWORD    cbHashObject = 0;
    PBYTE    pbHashObject = nullptr;
    PBYTE    pbHash       = nullptr;

    hash[0] = '\0';
    BYTE *file_data = slurp_file(filename, &data_len);
    if (file_data == nullptr)
        return static_cast<LSTATUS>(data_len);

    // Open an algorithm handle
    if (FAIL(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_MD5_ALGORITHM, nullptr, 0)))
    {
        wprintf(L"Error creating hash: 0x%lX returned by BCryptOpenAlgorithmProvider\n", status);
        goto Cleanup;
    }

    // Calculate the size of the buffer to hold the hash object
    if (FAIL(BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PBYTE>(&cbHashObject),
                               sizeof(DWORD), &cbData, 0)))
    {
        wprintf(L"Error creating hash: 0x%lX returned by BCryptGetProperty\n", status);
        goto Cleanup;
    }

    pbHashObject = new BYTE[cbHashObject];
    if (pbHashObject == nullptr) {
        wprintf(L"Error creating hash: memory allocation failed\n");
        goto Cleanup;
    }

    // Calculate the length of the hash
    if (FAIL(BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, reinterpret_cast<PBYTE>(&cbHash),
                               sizeof(DWORD), &cbData, 0)))
    {
        wprintf(L"Error creating hash: 0x%lX returned by BCryptGetProperty\n", status);
        goto Cleanup;
    }
    if (hashSize < static_cast<size_t>(cbHash) * 2 + 1) {
        wprintf(L"Error creating hash:: Buffer of size %zu is too small to hold the "
                L"hash. Need size %zu + 1.\n",
                hashSize, static_cast<size_t>(cbHash) * 2);
        status = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    pbHash = new BYTE[cbHash];
    if (pbHash == nullptr) {
        wprintf(L"Error creating hash: memory allocation failed\n");
        goto Cleanup;
    }

    // Create a hash
    if (FAIL(BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, nullptr, 0, 0)))
    {
        wprintf(L"Error creating hash: 0x%lX returned by BCryptCreateHash\n", status);
        goto Cleanup;
    }

    // Hash the data
    if (FAIL(BCryptHashData(hHash, file_data, static_cast<ULONG>(data_len), 0)))
    {
        wprintf(L"Error creating hash: 0x%lX returned by BCryptHashData\n", status);
        goto Cleanup;
    }

    // Close the hash
    if (FAIL(BCryptFinishHash(hHash, pbHash, cbHash, 0)))
    {
        wprintf(L"Error creating hash: 0x%lX returned by BCryptFinishHash\n", status);
        goto Cleanup;
    }

    // Convert to more useful format.
    for (size_t i = 0; i < cbHash; ++i) {
        hash[i * 2]     = hexDigits[pbHash[i] >> 4 & 0xF];
        hash[i * 2 + 1] = hexDigits[pbHash[i] & 0xF];
    }
    status = 0;

Cleanup:
    delete[] file_data;
    delete[] pbHashObject;
    delete[] pbHash;
    if (hAlg)
        BCryptCloseAlgorithmProvider(hAlg, 0);
    if (hHash)
        BCryptDestroyHash(hHash);

    return status;
#undef FAIL
}
