add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_policy("check.target_package_licenses", false)

if is_plat("windows") then
	add_requires("libplist", { configs = { toolchains = { "clang-cl" } } })
else
	add_requires("libplist")
end

target("PanicInfoReader")
    if not is_mode("debug") then
        set_policy("build.optimization.lto", true)
    end
    set_kind("binary")
    add_files("src/main.cpp")
    set_languages("cxx17")
    add_packages("libplist")
    add_cxflags(
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-Wsign-conversion",
        "-Wold-style-cast",
        "-Wshadow",
        "-Wconversion",
        "-Werror"
    )
    if is_plat("macosx") then
        add_frameworks("IOKit", "CoreFoundation")
    elseif is_plat("windows") then
        add_syslinks("advapi32")
    end
