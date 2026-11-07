# P4 co-processor upgrade tool: installs the esp-hosted image built by
# ../cp on the board's co-processor (Tab5: ESP32-C6). Build with the
# board's preset: idf.py --preset <board> build. See ../README.md.
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(p4_upgrade_cp)
