set_project("Btk")

-- Setup the environment
add_rules("mode.debug","mode.release", "mode.asan")

add_includedirs("include")
add_includedirs("src")

set_languages("c++17")
add_cxxflags("cl::/utf-8")

-- Mode check
if is_mode("debug") then 
    if is_plat("linux") then
        add_cxxflags("-rdynamic")
        add_cflags("-rdynamic")
        add_ldflags("-rdynamic")
    end
end

-- Main component
includes("src/core")
includes("src/widgets")
includes("src/platform")

-- Plugins
includes("src/plugins")

-- Tests
includes("tests")

-- Main Target
target("btk")
    set_kind("static")
    add_deps("btk_core", "btk_platform", "btk_widgets")

    -- Package exports
    add_headerfiles("include/(Btk/widgets/*.hpp)")
    add_headerfiles("include/(Btk/plugins/*.hpp)")
    add_headerfiles("include/(Btk/detail/*.hpp)")
    add_headerfiles("include/(Btk/signal/*.hpp)")
    add_headerfiles("include/(Btk/*.hpp)")
target_end()