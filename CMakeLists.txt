# cmake 最低版本要求
cmake_minimum_required (VERSION 3.22)

# 项目名
project(Btk)

# 编译参数
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb -std=c++17")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -std=c++17")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)

aux_source_directory(${PROJECT_SOURCE_DIR}/src SOURCE_LIST)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  # 添加第三方依赖
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(pangocairo REQUIRED IMPORTED_TARGET pangocairo)
  pkg_check_modules(x11 REQUIRED IMPORTED_TARGET x11)
  pkg_check_modules(sdl2 REQUIRED IMPORTED_TARGET sdl2)
  add_definitions(-DBTK_DRIVER=SDLDriverInfo)
  set(SOURCE_BACKEND_LIST ${PROJECT_SOURCE_DIR}/src/backend/sdl2.cpp)
  set(SOURCE_PAINTER_LIST ${PROJECT_SOURCE_DIR}/src/painter/cairo_painter.cpp)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
  add_definitions(-DBTK_DRIVER=Win32DriverInfo)
  set(SOURCE_BACKEND_LIST ${PROJECT_SOURCE_DIR}/src/backend/win32.cpp)
  set(SOURCE_PAINTER_LIST ${PROJECT_SOURCE_DIR}/src/painter/d2d_painter.cpp)
endif()

aux_source_directory(${PROJECT_SOURCE_DIR}/src/common SOURCE_COMMON_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/widgets SOURCE_WIDGETS_LIST)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

# Main Lib
add_library(btk 
            ${SOURCE_LIST}
            ${SOURCE_BACKEND_LIST}
            ${SOURCE_COMMON_LIST}
            ${SOURCE_PAINTER_LIST}
            ${SOURCE_WIDGETS_LIST})

# 链接依赖
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_link_libraries(btk 
    PkgConfig::pangocairo
    PkgConfig::x11
    PkgConfig::sdl2)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
  target_link_libraries(btk 
                        -user32
                        -shlwapi
                        -shell32
                        -imm32
                        -windowscodecs
                        -gdi32
                        -ole32
  )
endif()

# example application
pkg_check_modules(ffmpeg REQUIRED IMPORTED_TARGET libavformat libavutil libavcodec libswresample libswscale)

add_executable(player ./tests/media/player.cpp ./src/plugins/media.cpp)
target_link_libraries(player PRIVATE btk PkgConfig::ffmpeg)

# 启用测试
enable_testing()

# tests
add_executable(win ./tests/win.cpp)
target_link_libraries(win PRIVATE btk)
add_test(win bin/win)


add_executable(anim ./tests/anim.cpp)
target_link_libraries(anim PRIVATE btk)
add_test(anim bin/anim)

add_executable(pixproc ./tests/pixproc.cpp)
target_link_libraries(pixproc PRIVATE btk)
add_test(pixproc bin/pixproc)

# gtest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(test_run ./tests/test.cpp)

target_link_libraries(
  test_run
  GTest::gtest_main
  btk
)

include(GoogleTest)
gtest_discover_tests(test_run)
