# The Last Bell - a Final-Fantasy-style RPG on the PicoPods libraries.
# 160x160 indexed framebuffer scaled 2x by IndexedBlitter (~330 KB), so like
# PodCraft it needs an RP2350 (Pico 2 / Pico 2 W).
if(NOT PD_HOST AND PICO_PLATFORM MATCHES "rp2040")
    message(STATUS "The Last Bell skipped: it needs an RP2350 board (pico2, pico2_w)")
    return()
endif()
picopods_add_pod(lastbell
    SOURCES
        src/main.cpp
        src/art.cpp
        src/data.cpp
        src/maps.cpp
        src/game.cpp
        src/render.cpp
        src/battle.cpp
        src/storage.cpp
        src/keys.cpp
    LIBS pd_diag
    TITLE "The Last Bell"
    VERSION "0.1.0")
target_include_directories(lastbell PRIVATE src)
target_compile_options(lastbell PRIVATE -O2)
if(NOT PD_HOST)
    # Full 320x320 frames go out in ~49 ms at 50 MHz (as PodCraft runs).
    target_compile_definitions(lastbell PRIVATE PD_PICOCALC_LCD_HZ=50000000)
endif()
