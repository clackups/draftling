# HarfBuzz text shaper, vendored (see README.md). Shapes the Devanagari
# text of the Indic language pack on the device. Built only with
# CONFIG_DRAFTLING_HAS_DEVANAGARI; otherwise an empty component, and
# the fonts component requires it through idf_component_optional_requires().
if(NOT CONFIG_DRAFTLING_HAS_DEVANAGARI)
    idf_component_register()
    return()
endif()

idf_component_register(
    SRCS "src/harfbuzz.cc"
    INCLUDE_DIRS "src"
    PRIV_INCLUDE_DIRS "config"
)

# HB_TINY: OpenType layout and shaping only (no variable / color /
# bitmap fonts, AAT, drawing, subsetting), optimised for size; no
# threads. config/hb_draftling_config.h adjusts it for ESP-IDF. Upstream code: silence the warnings ESP-IDF turns on.
target_compile_definitions(${COMPONENT_LIB} PUBLIC HB_TINY HB_NO_MT)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    HB_CONFIG_OVERRIDE_LAST_H="hb_draftling_config.h")
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Os -fno-exceptions -fno-rtti
    -Wno-error -Wno-unused-function -Wno-unused-variable
    -Wno-missing-field-initializers -Wno-cast-function-type
    -Wno-sign-compare -Wno-unused-parameter)
