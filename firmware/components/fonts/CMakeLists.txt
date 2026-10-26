set(srcs
    "greybeard.c"
    "greybeard_11.c"
    "greybeard_14.c"
    "greybeard_16.c"
    "greybeard_18.c"
    "greybeard_22.c"
    "greybeard_26.c"
    "greybeard_30.c"
    "greybeard_34.c"
)

if(CONFIG_KB_LAYOUT_ENABLE_UA)
    list(APPEND srcs
        "greybeard_cyrillic_11.c"
        "greybeard_cyrillic_14.c"
        "greybeard_cyrillic_16.c"
        "greybeard_cyrillic_18.c"
        "greybeard_cyrillic_22.c"
        "greybeard_cyrillic_26.c"
        "greybeard_cyrillic_30.c"
        "greybeard_cyrillic_34.c"
    )
endif()

if(CONFIG_KB_LAYOUT_ENABLE_HE)
    list(APPEND srcs
        "greybeard_hebrew_11.c"
        "greybeard_hebrew_14.c"
        "greybeard_hebrew_16.c"
        "greybeard_hebrew_18.c"
        "greybeard_hebrew_22.c"
        "greybeard_hebrew_26.c"
        "greybeard_hebrew_30.c"
        "greybeard_hebrew_34.c"
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
        list(APPEND srcs "greybeard_latinx_${n}.c")
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

if(CONFIG_DRAFTLING_DISPLAY_HIDPI)
    # High-density boards render 1:1 with the larger Hack font family
    # instead of upscaling Greybeard. The base Hack fonts cover Latin;
    # Cyrillic and Hebrew subsets are gated on the same layout options
    # as the Greybeard subsets. The Hebrew subset is rendered from the
    # Greybeard TTFs because Hack has no Hebrew coverage.
    list(APPEND srcs
        "hack.c"
        "hack_9.c"
        "hack_11.c"
        "hack_14.c"
        "hack_16.c"
        "hack_18.c"
        "hack_22.c"
        "hack_26.c"
        "hack_30.c"
    )
    if(CONFIG_KB_LAYOUT_ENABLE_UA)
        list(APPEND srcs
            "hack_cyrillic_9.c"
            "hack_cyrillic_11.c"
            "hack_cyrillic_14.c"
            "hack_cyrillic_16.c"
            "hack_cyrillic_18.c"
            "hack_cyrillic_22.c"
            "hack_cyrillic_26.c"
            "hack_cyrillic_30.c"
        )
    endif()
    if(CONFIG_KB_LAYOUT_ENABLE_HE)
        list(APPEND srcs
            "hack_hebrew_9.c"
            "hack_hebrew_11.c"
            "hack_hebrew_14.c"
            "hack_hebrew_16.c"
            "hack_hebrew_18.c"
            "hack_hebrew_22.c"
            "hack_hebrew_26.c"
            "hack_hebrew_30.c"
        )
    endif()
    if(latinx_enabled)
        foreach(n ${font_slots_hack})
            list(APPEND srcs "hack_latinx_${n}.c")
        endforeach()
    endif()
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    EMBED_FILES ${embed_files}
    PRIV_REQUIRES lvgl__lvgl
)
