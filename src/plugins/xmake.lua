
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

    if is_plat("linux") then 
        -- Use ffmpeg from system
        add_requires("libavformat", "libavutil", "libavcodec", "libswresample", "libswscale")
        -- Use SDL by default
        add_requires("libsdl")
    else 
        -- Use ffmpeg from repo
        add_requires("ffmpeg");
        -- Use miniaudio by default
        add_requires("miniaudio")
    end

    target("btk_multimedia")
        set_kind("static")
        add_deps("btk")

        if is_plat("linux") then 
            -- Use ffmpeg from system
            add_packages("libavformat", "libavutil", "libavcodec", "libswresample", "libswscale")
            add_packages("libsdl")
        else 
            add_packages("ffmpeg")
            add_packages("miniaudio")

            -- Tell it
            add_defines("BTK_MINIAUDIO")
        end

        add_files("media/*.cpp")
    target_end()

end