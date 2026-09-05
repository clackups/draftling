set(srcs
    "greybeard.c"
    "greybeard_11.c"
    "greybeard_14.c"
    "greybeard_16.c"
    "greybeard_18.c"
    "greybeard_22.c"
    "greybeard_26.c"
)

if(CONFIG_KB_LAYOUT_ENABLE_UA)
    list(APPEND srcs
        "greybeard_cyrillic_11.c"
        "greybeard_cyrillic_14.c"
        "greybeard_cyrillic_16.c"
        "greybeard_cyrillic_18.c"
        "greybeard_cyrillic_22.c"
        "greybeard_cyrillic_26.c"
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
    )
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
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    PRIV_REQUIRES lvgl__lvgl
)
