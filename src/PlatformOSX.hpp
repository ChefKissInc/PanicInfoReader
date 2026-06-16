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
        mach_port_t masterPort;
        if (const auto err = IOMasterPort(bootstrap_port, &masterPort); err != KERN_SUCCESS) {
            throw std::runtime_error("Failed to get master port: " + std::to_string(err));
        }

        this->optionsNode = IORegistryEntryFromPath(masterPort, "IODeviceTree:/options");
        if (!this->optionsNode) {
            throw std::runtime_error("NVRAM is not supported on this system");
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
