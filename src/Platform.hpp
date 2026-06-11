// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #include "PlatformWin32.hpp"
#elif defined(__APPLE__)
    #include "PlatformOSX.hpp"
#elif defined(__linux__)
    #include "PlatformLinux.hpp"
#else
    #include <sys/param.h>
    #if defined(BSD) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
        #include "PlatformBSD.hpp"
    #else
        #error "Huh? What are you running, TempleOS?! Impressive :-D"
    #endif
#endif
