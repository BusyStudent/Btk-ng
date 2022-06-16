add_rules("mode.debug","mode.release")
lib_kind = "static"

set_languages("c++17")

target("btk")
    set_kind(lib_kind)
    add_files("src/*.cpp")
    add_includedirs("src")
    add_includedirs("include")

    -- Backends
    add_files("src/backend/sdl2.cpp")