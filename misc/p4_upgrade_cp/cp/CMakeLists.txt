# ESP-Hosted co-processor firmware: Wi-Fi plus a BLE controller over
# hosted HCI (VHCI), on the SDIO link to an ESP32-P4. Build it with the
# board's preset (idf.py --preset <board> build, see CMakePresets.json);
# the P4 upgrade tool (../p4) embeds build/<board>/cp_hosted.bin and
# installs it over the co-processor's existing firmware.
cmake_minimum_required(VERSION 3.16)
# The app version is the esp-hosted version pinned in
# main/idf_component.yml: the P4 upgrade tool reads it from the image to
# know what the C6 must report after the upgrade.
file(READ "${CMAKE_CURRENT_LIST_DIR}/main/idf_component.yml" _manifest)
string(REGEX MATCH "espressif/esp_hosted:[^\n]*\n[ ]*version:[ ]*\"([0-9]+\\.[0-9]+\\.[0-9]+)\"" _m "${_manifest}")
if(NOT CMAKE_MATCH_1)
    message(FATAL_ERROR "No exact espressif/esp_hosted version in main/idf_component.yml")
endif()
set(PROJECT_VER "${CMAKE_MATCH_1}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cp_hosted)

# The image is installed by OTA over whatever firmware the co-processor
# has now, and OTA cannot change the partition table, so the image must
# fit the smallest app slot of the tables the board's co-processor may
# have. The preset sets CP_MAX_APP_SIZE (Tab5: 1536 KB, the C6 app slot
# of esp-hosted-mcu 1.x).
if(NOT CP_MAX_APP_SIZE)
    message(FATAL_ERROR "CP_MAX_APP_SIZE is not set: build with a preset "
        "(idf.py --preset <board> build).")
endif()
set(app_bin "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin")
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -DBIN=${app_bin} -DMAX=${CP_MAX_APP_SIZE}
            -P ${CMAKE_CURRENT_LIST_DIR}/check_size.cmake
    VERBATIM)
