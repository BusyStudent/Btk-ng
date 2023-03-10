target("btk_core")
    set_kind("static")
    add_files("*.cpp")

    -- Font System
    if is_plat("windows") then 
        add_files("font/dwrite.cpp")
    elseif is_plat("linux") then
        add_packages("pangocairo")
        add_files("font/pango.cpp")
    else 
        add_files("font/freetype.cpp")
    end