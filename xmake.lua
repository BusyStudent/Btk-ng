add_rules("mode.debug","mode.release")
native_painter = true
lib_kind       = "static"

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

option("nanovg_painter")
    set_default(false)
    set_showmenu(true)
    set_description("Use platfrom indepent painter")
    set_category("Painter")
option_end()


-- Painter check
if has_config("nanovg_painter") then
    add_requires("freetype")
end 


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

    -- Option check
    if has_config("nanovg_painter") then 
        add_packages("freetype")
        add_files("src/painter/nvg_painter.cpp")
        native_painter = false
    end

    -- Backends
    if is_host("windows") and not is_plat("cross") then
        -- Add native painer if
        if native_painter then 
            add_files("src/painter/d2d_painter.cpp")
        end
        -- Make default driver
        add_defines("BTK_DRIVER=Win32DriverInfo")
        add_files("src/backend/win32.cpp")

        -- Add windows specific libraries
        add_links("user32", "shlwapi", "shell32", "imm32", "gdi32", "ole32")
        add_links("windowscodecs", "d2d1", "dwrite", "uuid", "dxguid")
    else
        -- Add native painer if
        if native_painter then 
            add_files("src/painter/cairo_painter.cpp")
        end
        -- Make default driver
        add_defines("BTK_DRIVER=SDLDriverInfo")
        add_files("src/backend/sdl2.cpp")

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

    target("pixproc")
        set_kind("binary")
        add_deps("btk")

        add_includedirs("include")
        add_files("tests/pixproc.cpp")
    target_end()
end
