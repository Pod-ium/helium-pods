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
