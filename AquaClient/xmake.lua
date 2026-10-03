MOD_BUILD_SCRIPT_VERSION = 2

local amethystSrc = os.getenv("AMETHYST_SRC")
includes(path.join(amethystSrc, "AmethystAPI", "mod_build.lua"))

build_mod("AquaClient", 1, 21, 0, false, {
    extra_include_dirs = { "vendor", "vendor/imgui" },
    extra_files = {
        "vendor/imgui/*.cpp",
        "vendor/imgui/backends/imgui_impl_dx11.cpp",
        "vendor/imgui/backends/imgui_impl_win32.cpp"
    },
    extra_links = { "winhttp", "d3d11", "dxgi", "imm32", "ws2_32" }
})
