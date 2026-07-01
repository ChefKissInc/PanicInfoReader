// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include "Platform.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <plist/plist.h>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
    #include <fcntl.h>
    #include <io.h>
#endif

namespace
{

    // Adapted from StackOverflow. They seriously only added this in C++20, lol.
    bool stringEndsWith(const std::string& fullString, const std::string& ending)
    {
        if (fullString.length() < ending.length()) { return false; }
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    }

    void printCompressedString(const uint8_t* data, const size_t dataSize)
    {
        std::string s;
        auto        expandKextInfo = false;
        char        prev           = 0;
        uint8_t     low            = 0;
        uint8_t     bit            = 0;
        auto        addChar        = [&](char c)
        {
            if (!expandKextInfo) {
                expandKextInfo = stringEndsWith(s, "loaded kext");
                if (!expandKextInfo) {
                    s.push_back(c);
                    return;
                }
            }

            const char* a;
            switch (c) {
                case '>': a = "com.apple.driver."; break;
                case '|': a = "com.apple.iokit."; break;
                case '$': a = "com.apple.security."; break;
                case '@': a = "com.apple."; break;
                default:
                    if (prev != '!') { goto skip; }
                    switch (c) {
                        case 'A': a = "Apple"; break;
                        case 'a': a = "Action"; break;
                        case 'B': a = "Bluetooth"; break;
                        case 'C': a = "Controller"; break;
                        case 'F': a = "Family"; break;
                        case 'I': a = "Intel"; break;
                        case 'P': a = "Profile"; break;
                        case 'S': a = "Storage"; break;
                        case 'U': a = "AppleUSB"; break;
                        default:
                        skip:
                            prev = c;
                            s.push_back(c);
                            return;
                    }
            };
            if (prev == '!') { s.pop_back(); }
            prev = 0;
            s.append(a);
        };

        for (size_t i = 0; i < dataSize; ++i) {
            const auto v = data[i];
            addChar(static_cast<char>(((v & (0x7FU >> bit)) << bit) | low));
            low = v >> (7 - bit);
            if (bit == 6) {
                addChar(static_cast<char>(low));
                low = 0;
                bit = 0;
                continue;
            }
            bit += 1;
        }

        std::cout << s;
    }

    class PlistParseError : public std::exception
    {
        std::string msg;

    public:
        PlistParseError(plist_err_t err) :
            msg("Could not parse data as a plist: " + std::to_string(err) + "")
        { }

        const char* what() const noexcept override { return msg.c_str(); }
    };

    std::array<char, 19> infoKeyFromIndex(uint8_t i)
    {
        std::array<char, 19> key = {"AAPL,PanicInfo0000"};
        key[17]                  = static_cast<char>(i < 10 ? '0' + i : 'A' + i);
        return key;
    }

    template<typename C>
    void runWithContext(C& context)
    {
        std::vector<uint8_t> fullData;
        for (uint8_t i = 0; i < 16; ++i) {
            const auto key  = infoKeyFromIndex(i);
            const auto data = context.readProp(key.data());
            if (data.empty()) { break; }
            fullData.insert(fullData.end(), data.begin(), data.end());
        }
        if (fullData.empty()) {
            fullData = context.readProp("aapl,panic-info");
            // Work around bug in AppleEFINVRAM.kext
            for (uint8_t i = 10; i < 16; ++i) {
                const auto key  = infoKeyFromIndex(i);
                const auto data = context.readProp(key.data());
                if (data.empty()) { break; }
                fullData.insert(fullData.end(), data.begin(), data.end());
            }
        }
        if (fullData.empty()) { std::cerr << "No panic data could be gathered.\n"; }
        else {
            printCompressedString(fullData.data(), fullData.size());
        }
    }

    class PListContext
    {
        plist_t plist{nullptr};

    public:
        PListContext(const std::vector<char>& data)
        {
            static const char marker[] = "{\"macOSProcessedStackshotData";
            const auto        pos      = std::search(data.begin(), data.end(), marker, marker + (sizeof(marker) - 1));

            if (const auto err =
                    pos == data.end() ?
                        plist_from_memory(data.data(), static_cast<uint32_t>(data.size()), &this->plist, NULL) :
                        plist_from_memory(&*pos,
                                          static_cast<uint32_t>(data.size() - static_cast<size_t>(&*pos - data.data())),
                                          &this->plist, NULL);
                err != PLIST_ERR_SUCCESS)
            {
                throw PlistParseError(err);
            }

            if (plist_get_node_type(this->plist) != PLIST_DICT) {
                plist_free(this->plist);
                throw std::runtime_error("Root node of plist provided is not a dictionary.\n");
            }
        }

        ~PListContext()
        {
            if (this->plist != nullptr) { plist_free(this->plist); }
        };

        std::vector<uint8_t> readProp(const char* key)
        {
            const auto prop = plist_dict_get_item(this->plist, key);
            if (prop == nullptr) { return {}; }
            if (plist_get_node_type(prop) != PLIST_DATA) {
                throw std::runtime_error(std::string(key) + " item in plist is not of data type");
            }
            uint64_t   dataLength = 0;
            const auto dataPtr    = plist_get_data_ptr(prop, &dataLength);
            if (dataPtr == nullptr || dataLength == 0) { return {}; }
            else {
                const auto dataBegin = reinterpret_cast<const uint8_t*>(dataPtr);
                return {dataBegin, dataBegin + dataLength};
            }
        }

        void run()
        {
            if (const auto panicStr = plist_dict_get_item(this->plist, "macOSPanicString")) {
                if (plist_get_node_type(panicStr) != PLIST_STRING) {
                    throw std::runtime_error("macOSPanicString item in plist is not of string type");
                }

                uint64_t   panicStrLength = 0;
                const auto panicStrPtr    = plist_get_string_ptr(panicStr, &panicStrLength);
                if (!panicStrPtr || panicStrLength == 0) { std::cout << "Empty panic string.\n"; }
                else {
                    std::cout << std::string_view(panicStrPtr, panicStrLength);
                }
                return;
            }

            runWithContext(*this);
        }
    };

    void runWithFile(const char* path)
    {
        std::ifstream f(path);
        if (!f || !f.is_open()) { throw std::runtime_error(std::string("Could not open ") + path); }
        f.seekg(0, std::ios::end);
        std::vector<char> data(static_cast<size_t>(f.tellg()), 0);
        f.seekg(0, std::ios::beg);
        f.read(data.data(), static_cast<std::streamsize>(data.size()));

        try {
            PListContext ctx(data);
            ctx.run();
            return;
        }
        catch (const PlistParseError& e) {
            std::cerr << "Warning: " << e.what() << "; interpreting as compressed string.\n";
        }

        printCompressedString(reinterpret_cast<uint8_t*>(&*data.begin()), data.size());
    }

}    // namespace

int main(int argc, char** argv)
{
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [FILE]\n";
        return 1;
    }

    try {
#ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
#endif

        if (argc == 2) {
            runWithFile(argv[1]);
            return 0;
        }

        PlatformContext context;
        runWithContext(context);
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << ".\n";
        return 1;
    }
}
