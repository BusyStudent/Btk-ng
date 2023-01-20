target("btk_core")
    set_kind("static")
    add_files("*.cpp")

    if is_plat("windows") then 
        add_files("font/dwrite.cpp")
    end