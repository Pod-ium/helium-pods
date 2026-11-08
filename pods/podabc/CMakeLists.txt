# PodABC - letters and numbers for young children, in Italian (voice and
# screens). Runs on every board (RP2040 and RP2350): ~0.5 MB of flash for
# the voice clips and the letters, little RAM.
picopods_add_pod(podabc
    SOURCES
        src/main.cpp
        src/assets.cpp
        src/sound.cpp
        src/draw.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/assets_data.cpp
    TITLE "PodABC"
    VERSION "1.1.0")
target_include_directories(podabc PRIVATE src)

# The assets (made by tools/make_assets.py, committed) become const arrays:
# in flash on the device.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(_assets ${CMAKE_CURRENT_SOURCE_DIR}/assets)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/assets_data.cpp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed.py
        ${CMAKE_CURRENT_BINARY_DIR}/assets_data.cpp
        abc_voice ${_assets}/voice.bin
        abc_glyphs ${_assets}/glyphs.bin
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed.py ${_assets}/voice.bin ${_assets}/glyphs.bin
    COMMENT "PodABC: embedding the voice clips and the letters")

# Host test of the player, the assets and the drawing (tests/podabc_test.cpp,
# registered in tests/CMakeLists.txt).
if(PD_HOST AND NOT EMSCRIPTEN)
    add_executable(podabc_test
        ${CMAKE_SOURCE_DIR}/tests/podabc_test.cpp
        src/assets.cpp
        src/sound.cpp
        src/draw.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/assets_data.cpp)
    target_include_directories(podabc_test PRIVATE src)
    target_link_libraries(podabc_test PRIVATE pd_app)
    target_compile_options(podabc_test PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
