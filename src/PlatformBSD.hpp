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
#include <sys/uuid.h>
#include <unistd.h>

class PlatformContext
{
    int    fd;
    uuid_t vendor;

    static std::vector<efi_char> cstrToEfiStr(const char* s)
    {
        const auto            nChars = strlen(s);
        std::vector<efi_char> ret(nChars + 1, 0);
        for (size_t i = 0; i < nChars; ++i) { ret[i] = s[i]; }
        return ret;
    }

public:
    PlatformContext()
    {
        this->fd = open("/dev/efi", O_RDWR);
        if (this->fd < 0) {
            throw std::runtime_error("Insufficient permissions or NVRAM is not supported on this system");
        }
        uint32_t status;
        uuid_from_string("7C436110-AB2A-4BBB-A880-FE41995C9F82", &this->vendor, &status);
        if (status != uuid_s_ok) {
            close(this->fd);
            throw std::runtime_error("internal error; could not parse vendor UUID");
        }
    }

    ~PlatformContext() { close(this->fd); }

    std::vector<uint8_t> readProp(const char* key)
    {
        auto               keyEfi = cstrToEfiStr(key);
        struct efi_var_ioc var    = {
            .vendor   = this->vendor,
            .name     = keyEfi.data(),
            .namesize = keyEfi.size() * sizeof(efi_char),
            .data     = nullptr,
            .datasize = 0,
        };
        if (ioctl(fd, EFIIOC_VAR_GET, &var) < 0) { return {}; }
        if (var.datasize == 0) { return {}; }
        std::vector<uint8_t> ret(var.datasize, 0);
        var.data = ret.data();
        if (ioctl(fd, EFIIOC_VAR_GET, &var) < 0) { return {}; }
        return ret;
    }
};
