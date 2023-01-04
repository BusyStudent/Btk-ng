option("tests")
    set_default(false)
    set_showmenu(true)
    set_description("Enable tests")
    set_category("Debugging")
option_end()

if has_config("tests") then
    add_requires("gtest", {optional = true })

    -- Add tests
    target("win")
        set_kind("binary")
        add_files("win.cpp")
        
        add_deps("btk")
    target_end()

    target("anim")
        set_kind("binary")
        add_files("anim.cpp")

        add_deps("btk")
    target_end()

    target("pixproc")
        set_kind("binary")
        add_files("pixproc.cpp")

        add_deps("btk")
    target_end()

    target("opengl")
        set_kind("binary")
        add_files("opengl.cpp")

        add_deps("btk")
    target_end()

    if has_package("gtest") then 
        -- Add string test
        target("test")
            add_packages("gtest")
            set_kind("binary")
            add_files("test.cpp")

            add_deps("btk")
        target_end()
    end

    -- FFMpeg test
    if has_config("multimedia") and has_config("tests") then 
        target("player")
            set_kind("binary")
            add_deps("btk", "btk_multimedia")

            add_files("media/player.cpp")
            add_includedirs("include")
        target_end()
    end

end