add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_policy("build.progress_style", "multirow")
set_policy("check.target_package_licenses", false)
set_warnings("allextra", "error")

if is_plat("windows") then
	add_requires("libplist", { configs = { toolchains = { "clang-cl" } } })
else
	add_requires("libplist")
end

target("PanicInfoReader")
    set_kind("binary")
    add_files("src/main.cpp")
    set_languages("cxx17")
    add_packages("libplist")
    set_policy("build.optimization.lto", not is_mode("debug"))
    if is_plat("macosx") then
        add_frameworks("IOKit", "CoreFoundation")
    elseif is_plat("windows") then
        add_syslinks("advapi32")
    end
