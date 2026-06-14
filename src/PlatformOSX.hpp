// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>
// platform headers
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

class PlatformContext
{
    io_registry_entry_t optionsNode;

public:
    PlatformContext()
    {
        this->optionsNode = IORegistryEntryFromPath(
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 120000
            kIOMainPortDefault,
#else
            kIOMasterPortDefault,
#endif
            "IODeviceTree:/options");
        if (!this->optionsNode) {
            throw std::runtime_error("Insufficient permissions or NVRAM is not supported on this system");
        }
    }

    ~PlatformContext() { IOObjectRelease(this->optionsNode); }

    std::vector<uint8_t> readProp(const char* key)
    {
        const auto keyCF = CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);
        if (!keyCF) { return {}; }
        const auto prop = IORegistryEntryCreateCFProperty(this->optionsNode, keyCF, kCFAllocatorDefault, 0);
        CFRelease(keyCF);
        if (!prop) { return {}; }

        if (CFGetTypeID(prop) == CFDataGetTypeID()) {
            const auto           data    = static_cast<CFDataRef>(prop);
            const auto           dataPtr = CFDataGetBytePtr(data);
            std::vector<uint8_t> ret(dataPtr, dataPtr + CFDataGetLength(data));
            return ret;
        }
        CFRelease(prop);

        return {};
    }
};
