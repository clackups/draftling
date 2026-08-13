cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/version.cmake)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
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

# Silence a use-after-free warning emitted from the
# espressif/esp_lcd_st77922 managed component (used by
# display_st77922.cpp on the Freenove FNK0104N, CONFIG_DRAFTLING_DISPLAY_ST77922).
# panel_st77922_del() in esp_lcd_st77922_rgb.c logs the freed pointer
# after calling free() on it:
#   free(st77922);
#   ESP_LOGD(TAG, "del st77922 panel @%p", st77922);
# The pointer value itself is never dereferenced (only printed), so this
# is harmless in practice, but newer GCC toolchains (as shipped with
# recent ESP-IDF) flag it as -Werror=use-after-free and fail the build.
# This bug is still present upstream as of esp-iot-solution's latest
# esp_lcd_st77922_rgb.c, so suppress it for that component's library
# only, without patching third-party vendor code we don't maintain.
if("espressif__esp_lcd_st77922" IN_LIST build_components)
    idf_component_get_property(st77922_lib espressif__esp_lcd_st77922 COMPONENT_LIB)
    if(st77922_lib)
        target_compile_options(${st77922_lib} PRIVATE
            -Wno-error=use-after-free)
    endif()
endif()
