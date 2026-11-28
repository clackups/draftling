cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/version.cmake)

# ESP-IDF 6.1 is the minimum: the Kconfig defaults files use its option
# names, and the managed components (esp_lvgl_port 2.9+) need it.
if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_LESS "6.1")
    message(FATAL_ERROR
        "Draftling needs ESP-IDF v6.1 or later; this is "
        "v${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}.${IDF_VERSION_PATCH}.")
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Draftling release version, shown in the F1 menu and embedded in the
# app image's esp_app_desc_t (readable at runtime via
# esp_app_get_description()->version). Bump this alongside every
# tagged release.
set(PROJECT_VER "1.0.33")

project(draftling)

# Draftling requires PSRAM on every supported board. The editor gap
# buffer (sized dynamically at editor_init() from the free PSRAM, see
# components/editor/editor.cpp), the
# UC8179 was previously supported (Seeed reTerminal E1001 and Waveshare
# E-Paper Driver HAT) but proved too slow for an interactive editor and
# was removed.
# RLCD / epdiy framebuffers, the LVGL display + widget heap
# (CONFIG_LV_USE_CUSTOM_MALLOC routes through PSRAM), the Git-sync
# response buffers and task stack, and Bluedroid's host environment
# (CONFIG_BT_BLE_DYNAMIC_ENV_MEMORY + BT_ALLOCATION_FROM_SPIRAM_FIRST)
# all assume MALLOC_CAP_SPIRAM is available. Without PSRAM the device
# runs out of internal heap during init, so refuse to configure rather
# than produce a binary that crashes at boot.
if(NOT CONFIG_SPIRAM)
    message(FATAL_ERROR
        "Draftling requires PSRAM, but CONFIG_SPIRAM is not enabled. "
        "Run 'idf.py menuconfig' and turn on 'Component config -> ESP "
        "PSRAM -> Support for external, SPI-connected RAM', then rebuild. "
        "If your target chip has no PSRAM support at all (e.g. ESP32-S2, "
        "bare ESP32-C3 modules without PSRAM), it is not supported.")
endif()

# Bluetooth comes from sdkconfig.defaults.bluetooth, which the board's
# preset lists in SDKCONFIG_DEFAULTS. A build directory configured
# before that file existed keeps the old list in its CMake cache, so
# deleting only sdkconfig would give a firmware without the BLE
# keyboard. Stop instead.
if(NOT CONFIG_BT_ENABLED AND NOT CONFIG_DRAFTLING_NO_BLUETOOTH)
    message(FATAL_ERROR
        "Bluetooth is off in ${SDKCONFIG}, but this board uses a BLE "
        "keyboard. The build directory is probably stale (its CMake cache "
        "holds an old SDKCONFIG_DEFAULTS list). Delete the whole build "
        "directory (e.g. rm -rf build/<board>) and build again.")
endif()

# Language pack consistency: idf.py --lang-pack (idf_ext.py) passes
# DRAFTLING_LANG_PACK and puts each pack in its own build directory. If
# an sdkconfig made for another pack ends up here anyway (a hand-picked
# -B, an old build tree), stop instead of building the wrong image.
if(DEFINED DRAFTLING_LANG_PACK AND NOT DRAFTLING_LANG_PACK STREQUAL CONFIG_DRAFTLING_LANG_PACK_NAME)
    message(FATAL_ERROR
        "Language pack mismatch: idf.py asked for '${DRAFTLING_LANG_PACK}' "
        "but ${SDKCONFIG} was made for '${CONFIG_DRAFTLING_LANG_PACK_NAME}'. "
        "Delete that sdkconfig (or the whole build directory) and build again.")
endif()

# The Indic language pack (Devanagari, Bengali, Telugu and Tamil fonts,
# HarfBuzz) does not fit the app partition of a 4 MB flash
# (partitions_4mb.csv, ~3.9 MB): refuse it there before compiling
# anything, instead of failing at the end on the image size check.
if(CONFIG_DRAFTLING_LANG_PACK_INDIC AND
   (CONFIG_ESPTOOLPY_FLASHSIZE_4MB OR CONFIG_ESPTOOLPY_FLASHSIZE_2MB))
    message(FATAL_ERROR
        "The Indic language pack does not fit boards with "
        "${CONFIG_ESPTOOLPY_FLASHSIZE} of flash. Build another language "
        "pack for this board.")
endif()

# Internal-flash document storage (CONFIG_DRAFTLING_STORAGE_FLASH): the
# partition table must hold a data/fat partition with the configured
# label, at least 1 MB large -- smaller volumes leave too little room
# for documents and their Git history. Checked here, at configure time,
# so a wrong table never reaches a device. (ESP-IDF itself checks that
# the table fits the flash and the app fits its slot.)
if(CONFIG_DRAFTLING_STORAGE_FLASH)
    set(_dl_min_storage 1048576)
    set(_dl_label "${CONFIG_DRAFTLING_STORAGE_FLASH_PARTITION}")
    if(NOT CONFIG_PARTITION_TABLE_CUSTOM)
        message(FATAL_ERROR
            "Flash document storage needs a custom partition table with a "
            "'${_dl_label}' data/fat partition (e.g. partitions_storage_16mb.csv; "
            "see sdkconfig.defaults.storage_flash_<size>).")
    endif()
    get_filename_component(_dl_csv "${CONFIG_PARTITION_TABLE_CUSTOM_FILENAME}"
                           ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
    if(NOT EXISTS "${_dl_csv}")
        message(FATAL_ERROR "Partition table ${_dl_csv} not found.")
    endif()
    file(STRINGS "${_dl_csv}" _dl_lines)
    set(_dl_found FALSE)
    foreach(_dl_line IN LISTS _dl_lines)
        string(STRIP "${_dl_line}" _dl_line)
        if(_dl_line STREQUAL "" OR _dl_line MATCHES "^#")
            continue()
        endif()
        string(REPLACE "," ";" _dl_f "${_dl_line}")
        list(LENGTH _dl_f _dl_n)
        if(_dl_n LESS 5)
            continue()
        endif()
        list(GET _dl_f 0 _dl_name)
        string(STRIP "${_dl_name}" _dl_name)
        if(NOT _dl_name STREQUAL _dl_label)
            continue()
        endif()
        set(_dl_found TRUE)
        list(GET _dl_f 1 _dl_type)
        list(GET _dl_f 2 _dl_sub)
        list(GET _dl_f 4 _dl_size)
        string(STRIP "${_dl_type}" _dl_type)
        string(STRIP "${_dl_sub}" _dl_sub)
        string(STRIP "${_dl_size}" _dl_size)
        if(NOT _dl_type STREQUAL "data" OR NOT _dl_sub STREQUAL "fat")
            message(FATAL_ERROR
                "Partition '${_dl_label}' in ${_dl_csv} is ${_dl_type}/${_dl_sub}; "
                "flash document storage needs a data/fat partition.")
        endif()
        # Sizes as gen_esp32part.py accepts them: hex, decimal, K or M.
        string(TOLOWER "${_dl_size}" _dl_size)
        if(_dl_size MATCHES "^0x[0-9a-f]+$")
            math(EXPR _dl_bytes "${_dl_size}")
        elseif(_dl_size MATCHES "^([0-9]+)k$")
            math(EXPR _dl_bytes "${CMAKE_MATCH_1} * 1024")
        elseif(_dl_size MATCHES "^([0-9]+)m$")
            math(EXPR _dl_bytes "${CMAKE_MATCH_1} * 1048576")
        elseif(_dl_size MATCHES "^[0-9]+$")
            set(_dl_bytes "${_dl_size}")
        else()
            message(FATAL_ERROR
                "Cannot read the size '${_dl_size}' of partition '${_dl_label}' "
                "in ${_dl_csv}.")
        endif()
        if(_dl_bytes LESS _dl_min_storage)
            math(EXPR _dl_kb "${_dl_bytes} / 1024")
            message(FATAL_ERROR
                "Flash document storage partition '${_dl_label}' in ${_dl_csv} "
                "is ${_dl_kb} KB; it must be at least 1 MB (1024 KB).")
        endif()
        math(EXPR _dl_kb "${_dl_bytes} / 1024")
        message(STATUS "Draftling flash document storage: '${_dl_label}', ${_dl_kb} KB")
        break()
    endforeach()
    if(NOT _dl_found)
        message(FATAL_ERROR
            "Flash document storage is enabled, but ${_dl_csv} has no "
            "'${_dl_label}' partition.")
    endif()

    # A 4 MB flash with 1 MB of storage leaves a 2.94 MB app slot
    # (partitions_storage_4mb.csv): only the default and Vietnamese
    # packs fit. Refuse the others before compiling anything.
    if(CONFIG_ESPTOOLPY_FLASHSIZE_4MB AND
       NOT (CONFIG_DRAFTLING_LANG_PACK_DEFAULT OR CONFIG_DRAFTLING_LANG_PACK_VIETNAMESE))
        message(FATAL_ERROR
            "The ${CONFIG_DRAFTLING_LANG_PACK_NAME} language pack does not fit a "
            "4 MB flash that also holds the document storage. Build the "
            "default or the Vietnamese pack for this board.")
    endif()
endif()

# Silence warnings emitted from the vroland/epdiy managed component
# (last upstream release 2.0.0, Nov 2023). The component still uses
# APIs that have since been deprecated in ESP-IDF:
#   - gdma_new_channel / gdma_set_transfer_ability  (-Wdeprecated-declarations)
#   - legacy driver/adc.h and esp_adc_cal.h         (raw #warning -> -Wcpp)
# and has a few unused locals (flags, vcomreg, int1reg, -Wunused-variable).
# These warnings are in third-party code we don't maintain and the build
# is still correct, so suppress them only for that component's library
# (and only when it's actually present in the build, i.e. when a LilyGO
# T5 EPD S3 (Pro/Pro Lite) board is selected and the component manager
# pulled epdiy in). This does not affect warnings in any draftling code.
idf_build_get_property(build_components BUILD_COMPONENTS)
if("vroland__epdiy" IN_LIST build_components)
    idf_component_get_property(epdiy_lib vroland__epdiy COMPONENT_LIB)
    if(epdiy_lib)
        target_compile_options(${epdiy_lib} PRIVATE
            -Wno-deprecated-declarations
            -Wno-unused-variable
            -Wno-cpp)
    endif()
endif()
