# Host tests of the games (cmake -DPD_TERMINAL=host): scripted sessions
# that drive each pod through its screens. Run with ctest.
set(_tmp ${CMAKE_CURRENT_BINARY_DIR}/tmp)
file(MAKE_DIRECTORY ${_tmp})

# PodCraft: title, play (break and place a block), help, debug info, menu:
# save to slot 1, then load it back; the save must have the expected size
# (64-byte header + 256 KB world).
set(_csd ${_tmp}/podcraft_sd)
set(_ckeys "wait:1800,enter,wait:200,j,wait:100,3,k,wait:100,f1,wait:100,space,f3,wait:100,shot")
set(_ckeys "${_ckeys},esc,wait:100,down,enter,wait:300,esc,wait:100,down,down,enter,wait:300,shot")
add_test(NAME podcraft_script COMMAND ${CMAKE_COMMAND} -E env
    PD_SDCARD=${_csd} PD_FRAMES=${_tmp}/podcraft_frames PD_KEYS=${_ckeys}
    sh -c "rm -rf ${_csd} ${_tmp}/podcraft_frames && mkdir -p ${_csd} ${_tmp}/podcraft_frames && $<TARGET_FILE:podcraft> && test $(wc -c < ${_csd}/podcraft/world1.pcw) -eq 262208")
set_tests_properties(podcraft_script PROPERTIES TIMEOUT 60)

# PodABC: assets, clip player (sample by sample) and drawing.
add_test(NAME podabc_unit COMMAND podabc_test)
set_tests_properties(podabc_unit PROPERTIES ENVIRONMENT "PD_KEYS=;PD_SDCARD=${_tmp}/podabc_unit_sd" TIMEOUT 30)

# PodABC: menu, game 2 (three keys), back to the menu with Esc + Up, game 1
# (a wrong key, then every letter: one of them is right), settings (symbols
# for game 1 -> letters only, saved on the card).
set(_asd ${_tmp}/podabc_sd)
set(_akeys "wait:1200,down,wait:300,enter,wait:400,a,wait:300,7,wait:300,W,wait:300,esc+up,wait:300")
set(_akeys "${_akeys},up,enter,wait:500,1,wait:2000,text:abcdefghijklmnopqrstuvwxyz,wait:3000,esc+up,wait:300")
set(_akeys "${_akeys},down,down,enter,wait:200,right,wait:200,esc,wait:300,shot")
add_test(NAME podabc_script COMMAND ${CMAKE_COMMAND} -E env
    PD_WINDOW=0 PD_SDCARD=${_asd} PD_FRAMES=${_tmp}/podabc_frames PD_KEYS=${_akeys}
    sh -c "rm -rf ${_asd} ${_tmp}/podabc_frames && mkdir -p ${_asd} ${_tmp}/podabc_frames && $<TARGET_FILE:podabc> && grep -qx symbols=1 ${_asd}/podabc/settings.txt")
set_tests_properties(podabc_script PROPERTIES TIMEOUT 60)
