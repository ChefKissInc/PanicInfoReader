// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <cstdint>
#include <string>
#include <vector>
// platform headers
#include <windows.h>

struct PlatformContext
{
    std::vector<uint8_t> readProp(const char* key)
    {
        const auto size = GetFirmwareEnvironmentVariableA(key, "C436110-AB2A-4BBB-A880-FE41995C9F82", nullptr, 0);
        if (size == 0) {
            const auto err = GetLastError();
            if (err == ERROR_SUCCESS) { return {}; }
            if (err == ERROR_INVALID_FUNCTION) { throw std::runtime_error("NVRAM not supported on this system"); }
            LPVOID lpMsgBuf;
            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                                  | FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL)
                == 0)
            {
                throw std::runtime_error("Failed to get error message string");
            }
            return {};
        }
        std::vector<uint8_t> ret(size, 0);
        if (GetFirmwareEnvironmentVariableA(key, "C436110-AB2A-4BBB-A880-FE41995C9F82", ret.data(), ret.size()) == 0) {
            const auto err = GetLastError();
            if (err == ERROR_SUCCESS) { return {}; }
            if (err == ERROR_INVALID_FUNCTION) { throw std::runtime_error("NVRAM not supported on this system"); }
            LPVOID lpMsgBuf;
            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                                  | FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL)
                == 0)
            {
                throw std::runtime_error("Failed to get error message string");
            }
            throw std::runtime_error(std::string("Failed to get NVRAM data: ") + static_cast<const char*>(lpMsgBuf));
        }
        return ret;
    }
};
