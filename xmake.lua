add_rules("mode.debug","mode.release")
lib_kind = "static"

if is_plat("windows") then
    add_cxxflags("/utf-8")
elseif is_plat("linux") then
    add_requires("pango", "pangocairo", "cairo")
    add_packages("pango", "pangocairo", "cairo")
end

set_languages("c++17")

-- Options
option("tests")
    set_default(false)
    set_showmenu(true)
    set_description("Enable tests")
    set_category("Debugging")
option_end()


-- Main Target

target("btk")
    set_kind(lib_kind)
    add_files("src/*.cpp")
    add_includedirs("src")
    add_includedirs("include")

    -- Package
    add_headerfiles("include/(Btk/plugins/*.hpp)")
    add_headerfiles("include/(Btk/detail/*.hpp)")
    add_headerfiles("include/(Btk/signal/*.hpp)")
    add_headerfiles("include/(Btk/*.hpp)")

    -- Backends
    if is_host("windows") and not is_plat("cross") then
        -- Make default driver
        add_defines("BTK_DRIVER=Win32DriverInfo")

        add_files("src/backend/win32.cpp")
        add_files("src/painter/d2d_painter.cpp")

        -- Add windows specific libraries
        add_links("user32", "shlwapi", "shell32", "imm32", "gdi32")
        add_links("windowscodecs", "d2d1", "dwrite", "uuid", "dxguid")
    else
        -- Make default driver
        add_defines("BTK_DRIVER=SDLDriverInfo")

        add_files("src/backend/sdl2.cpp")
        add_files("src/painter/cairo_painter.cpp")

        -- Add X11 Support
        add_links("X11", "SDL2")
    end
target_end()

-- Enable google test if tests are enabled
if has_config("tests") then
    add_requires("gtest")

    -- Add tests
    target("win")
        set_kind("binary")
        add_deps("btk")

        add_includedirs("include")
        add_files("tests/win.cpp")
    target_end()

    target("anim")
        set_kind("binary")
        add_deps("btk")

        add_includedirs("include")
        add_files("tests/anim.cpp")
    target_end()

    -- Add string test
    target("test")
        set_kind("binary")
        add_packages("gtest")
        add_deps("btk")

        add_includedirs("include")
        add_files("tests/test.cpp")
    target_end()
end
