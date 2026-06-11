## FastEPD is only used by the original-H752 display backend
## (components/display/display_h752.cpp). Compile its sources only
## when that backend is selected: the library drives the ESP32-S3
## LCD_CAM peripheral directly and does not build on other targets
## (the M5Stack Tab5 is an ESP32-P4). Gating SRCS on a CONFIG_ value
## is safe here -- unlike REQUIRES, the source list is evaluated in
## the second CMake pass, after sdkconfig has been parsed (see the
## NOTE in components/touchscreen/CMakeLists.txt for the REQUIRES
## gotcha). INCLUDE_DIRS stays unconditional so display_h752.cpp's
## #include resolves on every board; its body is preprocessed away
## when the backend is off.
if(CONFIG_DRAFTLING_DISPLAY_H752_EPD)
    set(fastepd_srcs
        "src/FastEPD.cpp"
        "src/FastEPD.inl"
        "src/Group5.cpp"
        "src/bb_ep_gfx.inl"
        "src/g5dec.inl")
else()
    set(fastepd_srcs "")
endif()

idf_component_register(
    SRCS ${fastepd_srcs}
    INCLUDE_DIRS "src"
    REQUIRES driver esp_driver_gpio esp_lcd esp_timer
)

if(CONFIG_DRAFTLING_DISPLAY_H752_EPD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE USE_BIT_BANG_I2C=1)
endif()
