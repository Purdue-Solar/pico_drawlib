cmake_minimum_required(VERSION 3.14)
project(pico_drawlib_sim C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
add_compile_definitions(SIMULATION)

# ── Find raylib ────────────────────────────────────────────────────────────────
# Tries, in order:
#   1. A locally installed package with CMake config files
#      (common Chocolatey paths are included as hints)
#   2. FetchContent — downloads and builds raylib 5.5 from GitHub
#      (requires internet access on the first configure)
#
# If you have raylib installed somewhere non-standard, pass its root on the
# command line:  cmake -DRAYLIB_ROOT=C:/my/raylib ...

find_package(raylib 4.0 QUIET CONFIG
    HINTS
        "${RAYLIB_ROOT}"
        "C:/raylib"
        "$ENV{RAYLIB_ROOT}"
        "$ENV{USERPROFILE}/raylib"
        "$ENV{ProgramFiles}/raylib"
)

if(NOT raylib_FOUND)
    message(STATUS "raylib CMake config not found — fetching from GitHub (internet required)")
    include(FetchContent)
    set(FETCHCONTENT_QUIET FALSE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.5
        GIT_SHALLOW    TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

# ── Sources ────────────────────────────────────────────────────────────────────
# Explicit list so CMake never accidentally picks up core1Entry.cpp,
# which requires pico multicore headers unavailable in the simulation.

set(SIM_SOURCES
    main.cpp
    picosdk_sim.c
    ili9341_sim.c
)

set(DRAWLIB_C_SOURCES
    ../Src/gfx.c
    ../Src/icons.c
    ../Src/Roboto-Regular20.c
    ../Src/Roboto-Regular20bw.c
    ../Src/Roboto-Regular40.c
    ../Src/Roboto-Regular40bw.c
    ../Src/mcufont_decoder/mf_bwfont.c
    ../Src/mcufont_decoder/mf_encoding.c
    ../Src/mcufont_decoder/mf_font.c
    ../Src/mcufont_decoder/mf_justify.c
    ../Src/mcufont_decoder/mf_kerning.c
    ../Src/mcufont_decoder/mf_rlefont.c
    ../Src/mcufont_decoder/mf_scaledfont.c
    ../Src/mcufont_decoder/mf_wordwrap.c
)

set(DRAWLIB_CXX_SOURCES
    ../Src/pdl.cpp   # C++17; compiled separately from the C sources
)

add_executable(pico_drawlib_sim
    ${SIM_SOURCES}
    ${DRAWLIB_C_SOURCES}
    ${DRAWLIB_CXX_SOURCES}
)

target_include_directories(pico_drawlib_sim PRIVATE
    ../Inc                         # pdl.hpp, display.h, gfx.h, …
    ${CMAKE_CURRENT_SOURCE_DIR}    # Sim/ itself — picosdk_sim.h + pico/stdlib.h stub
    ../Src/mcufont_decoder          # mcufont.h
    ../../pico_canlib/Inc           # artemis_canid.hpp
)

target_link_libraries(pico_drawlib_sim PRIVATE raylib)

# Platform graphics back-ends required by raylib
if(WIN32)
    target_link_libraries(pico_drawlib_sim PRIVATE opengl32 gdi32 winmm)
elseif(APPLE)
    target_link_libraries(pico_drawlib_sim PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreAudio"
        "-framework CoreVideo"
    )
endif()
