set(srcs
    "lowres.c"
    "lowres_11.c"
    "lowres_14.c"
    "lowres_16.c"
    "lowres_18.c"
    "lowres_22.c"
    "lowres_26.c"
    "lowres_30.c"
    "lowres_34.c"
)

if(CONFIG_KB_LAYOUT_ENABLE_UA)
    list(APPEND srcs
        "lowres_cyrillic_11.c"
        "lowres_cyrillic_14.c"
        "lowres_cyrillic_16.c"
        "lowres_cyrillic_18.c"
        "lowres_cyrillic_22.c"
        "lowres_cyrillic_26.c"
        "lowres_cyrillic_30.c"
        "lowres_cyrillic_34.c"
    )
endif()

if(CONFIG_KB_LAYOUT_ENABLE_HE)
    list(APPEND srcs
        "lowres_hebrew_11.c"
        "lowres_hebrew_14.c"
        "lowres_hebrew_16.c"
        "lowres_hebrew_18.c"
        "lowres_hebrew_22.c"
        "lowres_hebrew_26.c"
        "lowres_hebrew_30.c"
        "lowres_hebrew_34.c"
    )
endif()

# Latin-extension subset (Vietnamese letters, curly quotes, dashes) for
# the Vietnamese, Korean, Japanese and Chinese language packs.
set(latinx_enabled FALSE)
if(CONFIG_KB_LAYOUT_ENABLE_VI OR CONFIG_DRAFTLING_HAS_CJK)
    set(latinx_enabled TRUE)
endif()
set(font_slots_gb 11 14 16 18 22 26 30 34)
set(font_slots_hack 9 11 14 16 18 22 26 30)

if(latinx_enabled)
    foreach(n ${font_slots_gb})
        list(APPEND srcs "lowres_latinx_${n}.c")
    endforeach()
endif()

# Latin Extended-A subset (Polish, Czech, Turkish, ... letters) for the
# Central / Eastern European and Turkish layouts of the default pack.
if(CONFIG_DRAFTLING_HAS_LATIN_EXT_A)
    foreach(n ${font_slots_gb})
        list(APPEND srcs "lowres_latina_${n}.c")
    endforeach()
endif()

# CJK pixel fonts: the runtime driver plus one font set (see
# cjk/LICENSE.txt): the glyphs of the language pack's language, in its
# regional style, at the three native sizes.
set(embed_files)
if(CONFIG_DRAFTLING_HAS_CJK)
    list(APPEND srcs "cjk_font.c")
    set(cjk_set "${CONFIG_DRAFTLING_CJK_FONT_SET}")
    list(APPEND embed_files "cjk/${cjk_set}/cjk10.bin" "cjk/${cjk_set}/cjk12.bin"
                            "cjk/${cjk_set}/cjk16.bin")
endif()

# Tamil cluster glyphs (see tamil/LICENSE.txt): the runtime driver plus
# the blobs of the font family the board renders with.
if(CONFIG_DRAFTLING_HAS_TAMIL)
    list(APPEND srcs "tamil_font.c")
    if(CONFIG_DRAFTLING_DISPLAY_HIDPI)
        foreach(n ${font_slots_hack})
            list(APPEND embed_files "tamil/tamil_hires_${n}.bin")
        endforeach()
    else()
        foreach(n ${font_slots_gb})
            list(APPEND embed_files "tamil/tamil_lowres_${n}.bin")
        endforeach()
    endif()
endif()

# Devanagari (see deva/LICENSE.txt): shaped by HarfBuzz with the layout
# tables in deva_layout.ttf, drawn from the glyph bitmaps of the font
# set the board renders with.
if(CONFIG_DRAFTLING_HAS_DEVANAGARI)
    list(APPEND srcs "deva_font.c")
    list(APPEND embed_files "deva/deva_layout.ttf")
    if(CONFIG_DRAFTLING_DISPLAY_HIDPI)
        foreach(n ${font_slots_hack})
            list(APPEND embed_files "deva/deva_hires_${n}.bin")
        endforeach()
    else()
        foreach(n ${font_slots_gb})
            list(APPEND embed_files "deva/deva_lowres_${n}.bin")
        endforeach()
    endif()
endif()

if(CONFIG_DRAFTLING_DISPLAY_HIDPI)
    # High-density boards render 1:1 with the larger Hack font family
    # instead of upscaling Greybeard. The base Hack fonts cover Latin;
    # Cyrillic and Hebrew subsets are gated on the same layout options
    # as the Greybeard subsets. The Hebrew subset is rendered from the
    # Greybeard TTFs because Hack has no Hebrew coverage.
    list(APPEND srcs
        "hires.c"
        "hires_9.c"
        "hires_11.c"
        "hires_14.c"
        "hires_16.c"
        "hires_18.c"
        "hires_22.c"
        "hires_26.c"
        "hires_30.c"
    )
    if(CONFIG_KB_LAYOUT_ENABLE_UA)
        list(APPEND srcs
            "hires_cyrillic_9.c"
            "hires_cyrillic_11.c"
            "hires_cyrillic_14.c"
            "hires_cyrillic_16.c"
            "hires_cyrillic_18.c"
            "hires_cyrillic_22.c"
            "hires_cyrillic_26.c"
            "hires_cyrillic_30.c"
        )
    endif()
    if(CONFIG_KB_LAYOUT_ENABLE_HE)
        list(APPEND srcs
            "hires_hebrew_9.c"
            "hires_hebrew_11.c"
            "hires_hebrew_14.c"
            "hires_hebrew_16.c"
            "hires_hebrew_18.c"
            "hires_hebrew_22.c"
            "hires_hebrew_26.c"
            "hires_hebrew_30.c"
        )
    endif()
    if(latinx_enabled)
        foreach(n ${font_slots_hack})
            list(APPEND srcs "hires_latinx_${n}.c")
        endforeach()
    endif()
    if(CONFIG_DRAFTLING_HAS_LATIN_EXT_A)
        foreach(n ${font_slots_hack})
            list(APPEND srcs "hires_latina_${n}.c")
        endforeach()
    endif()
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    EMBED_FILES ${embed_files}
    PRIV_REQUIRES lvgl__lvgl
)

# Kconfig-conditional requirement: HarfBuzz is an empty component
# unless Devanagari is enabled.
if(CONFIG_DRAFTLING_HAS_DEVANAGARI)
    idf_component_optional_requires(PRIVATE harfbuzz)
endif()
