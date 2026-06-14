add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_policy("build.optimization.lto", true)
set_policy("check.target_package_licenses", false)

if is_plat("windows") then
    add_requires("libplist", {system = false, configs = {toolchains = {"clang-cl"}}})
else
    add_requires("libplist", {system = false})
end

target("PanicInfoReader")
    set_kind("binary")
    add_files("src/main.cpp")
    set_languages("c++17")
    add_packages("libplist")
    if is_plat("macosx") then
        add_frameworks("IOKit", "CoreFoundation")
    elseif is_plat("windows") then
        add_syslinks("advapi32")
    end
target_end()

