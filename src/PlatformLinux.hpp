// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
// platform headers
#include <sys/fcntl.h>
#include <unistd.h>

class PlatformContext
{
public:
    PlatformContext() { }

    std::vector<uint8_t> readProp(const char* key)
    {
        std::string path = std::string("/sys/firmware/efi/efivars/") + key + "-7c436110-ab2a-4bbb-a880-fe41995c9f82";

        int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) {
            if (errno == EACCES || errno == EPERM) {
                throw std::runtime_error("Insufficient permissions or kernel lockdown prevents reading EFI variables");
            }
            if (errno == ENOENT) { return {}; }
            throw std::runtime_error("Failed to open NVRAM file. Error code: " + std::to_string(errno));
        }

        off_t size = lseek(fd, 0, SEEK_END);
        if (size == (off_t)-1) {
            close(fd);
            throw std::runtime_error("Failed to seek NVRAM file. Error code: " + std::to_string(errno));
        }

        if (size < 4) {
            close(fd);
            return {};
        }

        size_t               dataSize = static_cast<size_t>(size) - 4;
        std::vector<uint8_t> ret(dataSize, 0);

        if (lseek(fd, 4, SEEK_SET) == (off_t)-1) {
            close(fd);
            throw std::runtime_error("Failed to seek past NVRAM attributes.");
        }

        ssize_t bytesRead;
        do {
            bytesRead = read(fd, ret.data(), dataSize);
        }
        while (bytesRead < 0 && errno == EINTR);

        close(fd);

        if (bytesRead < 0) {
            throw std::runtime_error("Failed to read NVRAM data. Error code: " + std::to_string(errno));
        }
        if (static_cast<size_t>(bytesRead) != dataSize) { throw std::runtime_error("Partial read on NVRAM data."); }

        return ret;
    }
};
