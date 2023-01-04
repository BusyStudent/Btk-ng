
option("multimedia")
    set_default(false)
    set_showmenu(true)
    set_description("Enable multimedia support")
    set_category("Plugins")
option_end()

option("builder")
    set_default(false)
    set_showmenu(true)
    set_description("Enable builder for build widget tree from xml")
    set_category("Plugins")
option_end()

-- Add ffmpeg coodes for play audio / video
if has_config("multimedia") then
    add_requires("libsdl")

    if is_plat("linux") then 
        -- Use ffmpeg from system
        add_requires("libavformat", "libavutil", "libavcodec", "libswresample", "libswscale")
    else 
        add_requires("ffmpeg");
    end

    target("btk_multimedia")
        set_kind("static")
        add_deps("btk")
        add_packages("libsdl")

        if is_plat("linux") then 
            -- Use ffmpeg from system
            add_packages("libavformat", "libavutil", "libavcodec", "libswresample", "libswscale")
        else 
            add_packages("ffmpeg")
        end

        add_files("media.cpp")
    target_end()

end