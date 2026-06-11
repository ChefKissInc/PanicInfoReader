// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

struct PlatformContext
{
    std::vector<uint8_t> readProp(const char* key)
    {
        std::ifstream f(std::string("/sys/firmware/efi/efivars/") + key + "-7c436110-ab2a-4bbb-a880-fe41995c9f82",
                        std::ios::in | std::ios::binary);
        f.seekg(0, std::ios::end);
        std::vector<uint8_t> ret(f.tellg() - std::streampos(4), 0);
        f.seekg(4, std::ios::beg);
        f.read(reinterpret_cast<char*>(ret.data()), ret.size());
        return ret;
    }
};
