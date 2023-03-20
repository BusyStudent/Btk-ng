
option("multimedia")
    set_default(false)
    set_showmenu(true)
    set_description("Enable multimedia support")
    set_category("Plugins")
option_end()

option("webview")
    set_default(false)
    set_showmenu(true)
    set_description("Enable webview support")
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
        add_deps("btk_core")

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

-- Add XmlBuilder
if has_config("multimedia") then
    add_requires("tinyxml2")

    target("btk_builder")
        set_kind("static")
        add_deps("btk_core")
        add_packages("tinyxml2")

        add_files("builder.cpp")
    target_end()
end

-- Add Webview
if has_config("webview") then

    target("btk_webview")
        set_kind("static")
        add_deps("btk_core")

        if is_plat("windows") then 
            add_files("webview/webview2.cpp")
        end
    target_end()
end