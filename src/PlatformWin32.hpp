// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
// platform headers
#include <windows.h>

class PlatformContext
{
public:
    PlatformContext()
    {
        HANDLE hToken;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            TOKEN_PRIVILEGES tp         = {0};
            tp.PrivilegeCount           = 1;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            if (LookupPrivilegeValueA(NULL, "SeSystemEnvironmentPrivilege", &tp.Privileges[0].Luid)) {
                AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);
            }
            CloseHandle(hToken);
        }
    }

    std::vector<uint8_t> readProp(const char* key)
    {
        std::vector<uint8_t> ret(4096, 0);

        while (true) {
            const auto size = GetFirmwareEnvironmentVariableA(key, "{7C436110-AB2A-4BBB-A880-FE41995C9F82}", ret.data(),
                                                              static_cast<DWORD>(ret.size()));

            if (size > 0) {
                ret.resize(size);
                return ret;
            }

            const auto err = GetLastError();

            if (err == ERROR_ENVVAR_NOT_FOUND) { return {}; }
            if (err == ERROR_INVALID_FUNCTION) {
                throw std::runtime_error("NVRAM not supported on this system (Legacy BIOS)");
            }
            if (err == ERROR_INSUFFICIENT_BUFFER) {
                ret.resize(ret.size() * 2);
                continue;
            }

            LPSTR lpMsgBuf = nullptr;
            DWORD msgLen   = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL);

            if (msgLen > 0 && lpMsgBuf) {
                std::string errMsg = std::string("Failed to get NVRAM data: ") + lpMsgBuf;
                LocalFree(lpMsgBuf);
                throw std::runtime_error(errMsg);
            }
            else {
                throw std::runtime_error("Failed to get NVRAM data. Error code: " + std::to_string(err));
            }
        }
        return ret;
    }
};
