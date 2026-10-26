# Input methods of the Vietnamese, Korean, Japanese and Chinese language
# packs (Vietnamese Telex, Korean Hangul, Japanese kana-kanji, Chinese
# pinyin). Built only when
# one of those layouts is enabled; otherwise an empty component, and the
# editor requires it through idf_component_optional_requires().
if(NOT (CONFIG_KB_LAYOUT_ENABLE_VI OR CONFIG_KB_LAYOUT_ENABLE_KO OR
        CONFIG_KB_LAYOUT_ENABLE_JA OR CONFIG_KB_LAYOUT_ENABLE_ZH))
    # REQUIRES here too: CONFIG_* is empty during the early pass that
    # builds the requirement graph, which takes this branch.
    idf_component_register(REQUIRES kb_layout)
    return()
endif()

set(embed_files)
if(CONFIG_KB_LAYOUT_ENABLE_JA)
    list(APPEND embed_files "dict/ja.bin")
endif()
if(CONFIG_KB_LAYOUT_ENABLE_ZH)
    list(APPEND embed_files "dict/zh.bin")
endif()

idf_component_register(
    SRCS "ime.c" "ime_builtin.c" "ime_telex.c" "ime_hangul.c" "ime_kana.c" "ime_pinyin.c"
    INCLUDE_DIRS "include"
    REQUIRES kb_layout
    EMBED_FILES ${embed_files}
)
