# The co-processor image built by ../cp with the same preset is embedded
# into this app. sdkconfig values are not known during the component
# manager's early expansion pass, which only collects requirements.
set(embed "")
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    if(NOT CONFIG_P4UP_BOARD_ID)
        message(FATAL_ERROR "No board selected: build with a preset "
            "(idf.py --preset <board> build).")
    endif()
    set(cp_bin "${CMAKE_CURRENT_LIST_DIR}/../../cp/build/${CONFIG_P4UP_BOARD_ID}/cp_hosted.bin")
    if(NOT EXISTS "${cp_bin}")
        message(FATAL_ERROR
            "Co-processor image not found: ${cp_bin}\n"
            "Build it first: cd ../cp && idf.py --preset ${CONFIG_P4UP_BOARD_ID} build "
            "(or run ../build.sh ${CONFIG_P4UP_BOARD_ID}, which builds both).")
    endif()
    set(embed "${cp_bin}")
endif()

idf_component_register(
    SRCS "main.c" "ui.c" "upgrader.c" "cp_sdio.c" "hosted_proto.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_sdmmc sdmmc esp_driver_gpio esp_timer
    EMBED_FILES ${embed}
)
