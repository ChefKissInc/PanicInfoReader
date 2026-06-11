add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_policy("build.optimization.lto", true)
set_policy("check.target_package_licenses", false)

add_requires("libplist", {optional = true})

option("enable_libplist")
    set_default(true)
    set_showmenu(true)
    add_defines("CONFIG_ENABLE_LIBPLIST")
option_end()

target("PanicInfoReader")
    set_kind("binary")
    add_files("src/main.cpp")
    set_languages("c++20")
    add_options("enable_libplist")
    if has_config("enable_libplist") then
        add_packages("libplist", {private = true})
    end
    if is_plat("macosx") then
        add_frameworks("IOKit", "CoreFoundation")
    end
target_end()

