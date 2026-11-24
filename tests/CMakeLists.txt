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

# The Last Bell: title -> new game -> intro -> talk to the elder (quest) ->
# leave Quayhaven -> random battle in the tall grass -> victory -> Esc menu
# -> save to slot 1. The save must have the expected size (64-byte header +
# the Game struct), and the session must have won a battle and saved.
set(_lsd ${_tmp}/lastbell_sd)
set(_lkeys "wait:1500,enter,wait:1700,enter,wait:1700,enter,wait:1700,enter,wait:1700,enter,wait:1700,enter,wait:600,up,wait:150,left,wait:200,enter,wait:1700,enter,wait:1700,enter,wait:1700,enter,wait:400,shot,right,wait:140,right,wait:140,right,wait:140,right,wait:140,right,wait:140,right,wait:140,right,wait:140,right,wait:140,right,wait:200,down,wait:140,down,wait:140,down,wait:140,down,wait:140,down,wait:140,down,wait:140,down,wait:140,down,wait:140,down,wait:140,down,wait:140,down,wait:140,down,wait:140,down,wait:140,down,wait:140,down,wait:140,down,wait:140,down,wait:140,wait:300,shot,down,wait:140,down,wait:140,down,wait:140,down,wait:140,right,wait:140,right,wait:140,right,wait:140,down,wait:120,down,wait:120,right,wait:120,right,wait:120,down,wait:120,right,wait:120,right,wait:120,down,wait:120,left,wait:120,left,wait:120,up,wait:120,up,wait:120,left,wait:120,left,wait:120,up,wait:120,up,wait:120,down,wait:120,down,wait:120,right,wait:120,right,wait:120,down,wait:120,right,wait:120,right,wait:120,down,wait:120,left,wait:120,left,wait:120,up,wait:120,up,wait:120,left,wait:120,left,wait:120,up,wait:120,up,wait:120,down,wait:120,down,wait:120,right,wait:120,right,wait:120,down,wait:120,right,wait:120,right,wait:120,down,wait:120,left,wait:120,left,wait:120,up,wait:120,up,wait:120,left,wait:120,left,wait:120,up,wait:120,up,wait:120,down,wait:120,down,wait:120,right,wait:120,right,wait:120,down,wait:120,right,wait:120,right,wait:120,down,wait:120,left,wait:120,left,wait:120,up,wait:120,up,wait:120,left,wait:120,left,wait:120,up,wait:120,up,wait:120,wait:300,shot,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,enter,wait:250,wait:900,enter,wait:400,enter,wait:400,shot,wait:300,shot,esc,wait:300,down,wait:150,down,wait:150,enter,wait:300,enter,wait:500,enter,wait:400,shot")
add_test(NAME lastbell_script COMMAND ${CMAKE_COMMAND} -E env
    PD_SDCARD=${_lsd} PD_FRAMES=${_tmp}/lastbell_frames PD_KEYS=${_lkeys}
    sh -c "rm -rf ${_lsd} ${_tmp}/lastbell_frames && mkdir -p ${_lsd} ${_tmp}/lastbell_frames && $<TARGET_FILE:lastbell> && test $(wc -c < ${_lsd}/lastbell/save1.pqs) -eq 196")
set_tests_properties(lastbell_script PROPERTIES TIMEOUT 120
    PASS_REGULAR_EXPRESSION "quest started.*battle start.*victory.*saved slot 1"
    FAIL_REGULAR_EXPRESSION "game over")
