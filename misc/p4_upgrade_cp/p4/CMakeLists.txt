# P4 co-processor upgrade tool: installs the esp-hosted image built by
# ../cp on the board's co-processor (Tab5: ESP32-C6). Build with the
# board's preset: idf.py --preset <board> build. See ../README.md.
cmake_minimum_required(VERSION 3.16)
# The tool's version is the esp-hosted version it installs, pinned in
# ../cp/main/idf_component.yml (the same rule as ../cp/CMakeLists.txt).
file(READ "${CMAKE_CURRENT_LIST_DIR}/../cp/main/idf_component.yml" _manifest)
string(REGEX MATCH "espressif/esp_hosted:[^\n]*\n[ ]*version:[ ]*\"([0-9]+\\.[0-9]+\\.[0-9]+)\"" _m "${_manifest}")
if(NOT CMAKE_MATCH_1)
    message(FATAL_ERROR "No exact espressif/esp_hosted version in ../cp/main/idf_component.yml")
endif()
set(PROJECT_VER "${CMAKE_MATCH_1}")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(p4_upgrade_cp)
