// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>
// platform headers
#include <sys/efiio.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct PlatformContext
{
    int         fd;
    struct uuid vendor;

    PlatformContext()
    {
        this->fd = open("/dev/efi", O_RDWR);
        if (this->fd < 0) {
            throw std::runtime_error("insufficient permissions or nvram is not supported on this system");
        }
        if (parse_uuid("C436110-AB2A-4BBB-A880-FE41995C9F82", &this->vendor) != 0) {
            throw std::runtime_error("internal error; could not parse vendor UUID");
        }
    }

    ~PlatformContext() { close(this->fd); }

    std::vector<uint8_t> readProp(const char* key)
    {
        const auto         keyEfi = cstrToEfiStr(key);
        struct efi_var_ioc var    = {
               .vendor   = this->guid,
               .name     = keyEfi.data(),
               .namesize = keyEfi.size() * sizeof(efi_char),
               .data     = nullptr,
               .datasize = 0,
        };
        ioctl(fd, EFIIOC_GET_VARIABLE, &var);
        if (var.datasize == 0) { return {}; }
        std::vector<uint8_t> ret(var.datasize, 0);
        var.data = ret.data();
        ioctl(fd, EFIIOC_GET_VARIABLE, &var);
        return ret;
    }

private:
    static std::vector<efi_char> cstrToEfiStr(const char* s)
    {
        const auto            nChars = strlen(s);
        std::vector<efi_char> ret(nChars + 1, 0);
        for (decltype(nChars) i = 0; i < nChars; ++i) { ret[i] = s[i]; }
        return ret;
    }
};
