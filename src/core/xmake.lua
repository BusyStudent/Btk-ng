target("btk_core")
    set_kind("static")
    add_files("*.cpp")

    -- Font System
    if is_plat("windows") then 
        add_files("font/dwrite.cpp")
    else 
        add_files("font/freetype.cpp")
    end