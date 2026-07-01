// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
// platform headers
#include <windows.h>

class PlatformContext
{
    struct HandleCloser
    {
        void operator()(HANDLE h) const noexcept
        {
            if (h) { CloseHandle(h); }
        }
    };
    using UniqueHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;

public:
    PlatformContext()
    {
        HANDLE rawToken = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &rawToken)) {
            throw std::runtime_error("Failed to open process token; Error code: " + std::to_string(GetLastError()));
        }
        const UniqueHandle hToken(rawToken);

        TOKEN_PRIVILEGES tp{};
        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!LookupPrivilegeValueA(NULL, "SeSystemEnvironmentPrivilege", &tp.Privileges[0].Luid)) {
            throw std::runtime_error("Failed to look up SeSystemEnvironmentPrivilege; Error code: "
                                     + std::to_string(GetLastError()));
        }

        const auto adjusted = AdjustTokenPrivileges(hToken.get(), FALSE, &tp, 0, NULL, NULL);
        const auto err      = GetLastError();

        if (!adjusted) {
            throw std::runtime_error("Failed to adjust token privileges; Error code: " + std::to_string(err));
        }

        if (err == ERROR_NOT_ALL_ASSIGNED) {
            throw std::runtime_error("Could not assign SeSystemEnvironmentPrivilege; process likely not elevated");
        }
    }

    std::vector<uint8_t> readProp(const char* key)
    {
        static constexpr size_t MAX_BUFFER_SIZE = 1U << 20;

        std::vector<uint8_t> buf(4096, 0);

        DWORD err = ERROR_SUCCESS;
        while (true) {
            SetLastError(ERROR_SUCCESS);

            const auto size = GetFirmwareEnvironmentVariableA(key, "{7C436110-AB2A-4BBB-A880-FE41995C9F82}", buf.data(),
                                                              static_cast<DWORD>(buf.size()));

            err = GetLastError();

            if (size > 0 || (size == 0 && err == ERROR_SUCCESS)) {
                buf.resize(size);
                return buf;
            }

            if (err == ERROR_ENVVAR_NOT_FOUND) { return {}; }
            else if (err == ERROR_INVALID_FUNCTION) {
                throw std::runtime_error("NVRAM is not supported on this system");
            }

            if (buf.size() >= MAX_BUFFER_SIZE) { break; }
            buf.resize(buf.size() * 2);
        }

        LPSTR      lpMsgBuf = nullptr;
        const auto msgLen   = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&lpMsgBuf), 0, NULL);

        if (msgLen > 0 && lpMsgBuf) {
            std::string errMsg(lpMsgBuf, msgLen);
            LocalFree(lpMsgBuf);
            while (!errMsg.empty() && (errMsg.back() == '\r' || errMsg.back() == '\n')) { errMsg.pop_back(); }
            throw std::runtime_error("Failed to get NVRAM data: " + errMsg);
        }
        else {
            throw std::runtime_error("Failed to get NVRAM data. Error code: " + std::to_string(err));
        }
    }
};
