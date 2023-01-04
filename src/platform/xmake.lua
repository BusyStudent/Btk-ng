win32_plat     = is_host("windows") and not is_plat("cross")
native_painter = true

if is_plat("linux") then
    add_requires("pango", "pangocairo", "cairo", "libsdl", "libx11")
    add_packages("pango", "pangocairo", "cairo", "libsdl", "libx11")

    add_links("pthread")
end

-- Platform specific option
option("sdl2_driver")
    set_default(false)
    set_showmenu(true)
    set_description("Force to use sdl graphics driver")
    set_category("Drivers")
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

-- Driver check
if has_config("sdl2_driver") then 
    add_requires("libsdl")
end

target("btk_platform")
    set_kind("static")

    -- Generic 
    if win32_plat then 
        -- Win32 deps
        add_links("user32", "shlwapi", "shell32", "imm32")
        add_links("windowscodecs", "gdi32", "ole32")
    end


    -- Backends
    if win32_plat and not has_config("sdl2_driver") then
        -- Make default driver
        add_defines("BTK_DRIVER=Win32DriverInfo")
        add_files("backend/win32.cpp")

        -- Add windows specific libraries
        add_links("d2d1", "dwrite", "uuid", "dxguid")
    else
        -- Make default driver
        add_defines("BTK_DRIVER=SDLDriverInfo")
        add_files("backend/sdl2.cpp")

        -- Add sdl requires
        add_packages("libsdl")
    end

    -- Painter
    if     win32_plat and native_painter then
        -- Add Direct2D
        add_files("painter/d2d_painter.cpp")

        -- Add direct2d libs
        add_links("d2d1", "dwrite", "uuid", "dxguid")
    elseif is_plat("linux") and native_painter then
        -- Add cairo
        add_files("painter/cairo_painter.cpp")

        -- Add cairo deps
        add_packages("pango", "pangocairo", "cairo")
    else
        -- Add nanovg
        add_files("painter/nvg_painter.cpp")
    end 

    -- Add extra sources
    add_files("backend/backend.cpp")