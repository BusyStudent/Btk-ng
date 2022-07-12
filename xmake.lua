add_rules("mode.debug","mode.release")
lib_kind = "static"

if is_plat("windows") then
    add_cxxflags("/utf-8")
end

set_languages("c++17")

target("btk")
    set_kind(lib_kind)
    add_files("src/*.cpp")
    add_includedirs("src")
    add_includedirs("include")

    -- Backends
    if is_plat("windows") then
        add_files("src/backend/win32.cpp")
        add_files("src/painter/d2d_painter.cpp")

        -- Add windows specific libraries
        add_links("user32", "shlwapi", "shell32")
        add_links("windowscodecs", "d2d1", "dwrite")
    else
        add_files("src/backend/sdl2.cpp")
    end

target("win")
    set_kind("binary")
    add_deps("btk")
    add_includedirs("include")
    add_files("tests/win.cpp")