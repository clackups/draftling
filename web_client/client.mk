# The web client program: draftling.js + draftling.wasm. Run by the
# Makefile:
#
#   make -f client.mk
#
# web_client/sdkconfig.defaults (the DRAFTLING_MODEL_WEB "board") plus
# firmware/sdkconfig.defaults.lang_all (every language) drive the
# firmware's Kconfig, like `idf.py --preset <board> --lang-pack all`.
# The resulting CONFIG_* values are read back into make (config.mk)
# and select the sources, font data and input methods exactly as the
# firmware's component CMakeLists.txt files do (and as
# linux/draftling.mk does). Output: build/out/.

FIRMWARE   := ../firmware
COMPONENTS := $(FIRMWARE)/components
LINUX      := ../linux
BUILD      := build
LVGL_DIR   := $(BUILD)/lvgl
LVGL_LIB   := $(BUILD)/liblvgl.a
KCONFIGLIB := $(BUILD)/kconfiglib/kconfiglib.py
MINIZ      := $(BUILD)/miniz/miniz.c

PACK := all
PDIR := $(BUILD)/p
OUT  := $(BUILD)/out
PROG := $(OUT)/draftling.js

SDKCONFIG := $(PDIR)/sdkconfig.h
DEFAULTS  := sdkconfig.defaults $(FIRMWARE)/sdkconfig.defaults.lang_$(PACK)

VERSION := $(shell sed -n 's/^set(PROJECT_VER "\(.*\)")/\1/p' $(FIRMWARE)/CMakeLists.txt)

CC  := emcc
CXX := em++
OPT ?= -O2

all: $(PROG)

# ---- configuration --------------------------------------------------------

KCONFIG_INPUTS := tools/Kconfig.web tools/gen_sdkconfig.py $(DEFAULTS) \
	$(FIRMWARE)/main/Kconfig.projbuild \
	$(COMPONENTS)/kb_layout/Kconfig.projbuild \
	$(COMPONENTS)/editor/Kconfig.projbuild

$(SDKCONFIG): $(KCONFIG_INPUTS) $(KCONFIGLIB)
	@mkdir -p $(dir $@)
	PYTHONPATH=$(dir $(KCONFIGLIB)) python3 tools/gen_sdkconfig.py \
		$(FIRMWARE) $@ $(DEFAULTS)

# sdkconfig.h as make variables (CONFIG_FOO := 1); make rebuilds this
# file first and restarts, so everything below can test the options.
$(PDIR)/config.mk: $(SDKCONFIG)
	sed -n 's/^#define \(CONFIG_[A-Za-z0-9_]*\) \(.*\)$$/\1 := \2/p' $< > $@

include $(PDIR)/config.mk

# (On the first pass config.mk does not exist yet: make generates it
# and starts again.)
HAVE_CONFIG := $(wildcard $(PDIR)/config.mk)
ifneq ($(HAVE_CONFIG),)
ifndef CONFIG_DRAFTLING_MODEL_WEB
$(error The Kconfig did not select DRAFTLING_MODEL_WEB -- check sdkconfig.defaults)
endif
ifndef CONFIG_DRAFTLING_DISPLAY_HIDPI
$(error The web client expects CONFIG_DRAFTLING_DISPLAY_HIDPI)
endif
ifndef CONFIG_DRAFTLING_LANG_PACK_ALL
$(error The web client expects the "all" language pack)
endif
endif

# ---- sources ----------------------------------------------------------------

FW_SRCS := \
	$(wildcard $(COMPONENTS)/editor/*.cpp) \
	$(wildcard $(COMPONENTS)/editor/*.c) \
	$(COMPONENTS)/kb_layout/kb_layout.cpp \
	$(COMPONENTS)/sd_card/sd_card.cpp \
	$(COMPONENTS)/display/display_margins.cpp \
	$(COMPONENTS)/display/display_orientation.cpp \
	$(COMPONENTS)/display/display_flip.cpp \
	$(COMPONENTS)/display/display_scale.cpp \
	$(COMPONENTS)/display/lv_rtl_line_width.c

# The real Git client (components/git_sync/CMakeLists.txt).
G := $(COMPONENTS)/git_sync
FW_SRCS += $(G)/git_sync.cpp $(foreach s,git_util git_odb git_pack git_merge git_net draftbox,$(G)/$(s).c)

EMBED :=

# Fonts, as components/fonts/CMakeLists.txt selects them: the hi-res
# (Hack) set only.
F := $(COMPONENTS)/fonts
LATINX := $(if $(CONFIG_KB_LAYOUT_ENABLE_VI)$(CONFIG_DRAFTLING_HAS_CJK),y)
SET   := hires
SLOTS := 9 11 14 16 18 22 26 30
SUBSETS := $(if $(CONFIG_KB_LAYOUT_ENABLE_UA),cyrillic) \
	$(if $(CONFIG_KB_LAYOUT_ENABLE_HE),hebrew) \
	$(if $(LATINX),latinx) \
	$(if $(CONFIG_DRAFTLING_HAS_LATIN_EXT_A),latina)
FW_SRCS += $(F)/$(SET).c $(foreach n,$(SLOTS),$(F)/$(SET)_$(n).c) \
	$(foreach s,$(SUBSETS),$(foreach n,$(SLOTS),$(F)/$(SET)_$(s)_$(n).c))

# The "all" pack's CJK font: the ja, zh and ko sets merged at build
# time by the Linux program's tool.
ifdef CONFIG_DRAFTLING_HAS_CJK
FW_SRCS += $(F)/cjk_font.c
CJK_DIR := $(PDIR)/cjk-all
EMBED   += $(foreach n,10 12 16,$(CJK_DIR)/cjk$(n).bin)
endif

ifdef CONFIG_DRAFTLING_HAS_TAMIL
FW_SRCS += $(F)/tamil_font.c
EMBED   += $(foreach n,$(SLOTS),$(F)/tamil/tamil_$(SET)_$(n).bin)
endif

INDIC_SCRIPTS := $(if $(CONFIG_DRAFTLING_HAS_DEVANAGARI),deva) $(if $(CONFIG_DRAFTLING_HAS_BENGALI),beng) \
	$(if $(CONFIG_DRAFTLING_HAS_TELUGU),telu)
ifdef CONFIG_DRAFTLING_HAS_INDIC_SHAPING
FW_SRCS += $(F)/indic_font.c
EMBED   += $(foreach s,$(INDIC_SCRIPTS),$(F)/$(s)/$(s)_layout.ttf \
	$(foreach n,$(SLOTS),$(F)/$(s)/$(s)_$(SET)_$(n).bin))
HB_SRCS := $(COMPONENTS)/harfbuzz/src/harfbuzz.cc
endif

# Input methods, as components/ime/CMakeLists.txt selects them.
I := $(COMPONENTS)/ime
TELUGU_IME := $(CONFIG_KB_LAYOUT_ENABLE_TE_RTS)$(CONFIG_KB_LAYOUT_ENABLE_TE_PO)$(CONFIG_KB_LAYOUT_ENABLE_TE_AP)
TAMIL_IME := $(CONFIG_KB_LAYOUT_ENABLE_TA99)$(CONFIG_KB_LAYOUT_ENABLE_TA_TW)$(CONFIG_KB_LAYOUT_ENABLE_TA_TV)
HAS_IME := $(CONFIG_KB_LAYOUT_ENABLE_VI)$(CONFIG_KB_LAYOUT_ENABLE_KO)$(CONFIG_KB_LAYOUT_ENABLE_JA)$(CONFIG_KB_LAYOUT_ENABLE_ZH)$(TAMIL_IME)$(CONFIG_KB_LAYOUT_ENABLE_BN_NJ)$(TELUGU_IME)
ifneq ($(HAS_IME),)
FW_SRCS += $(foreach s,ime ime_builtin ime_telex ime_hangul ime_kana ime_pinyin,$(I)/$(s).c)
ifneq ($(TAMIL_IME),)
FW_SRCS += $(foreach s,ime_tamil ime_tamil99 ime_tamil_tw ime_taravinyas,$(I)/$(s).c)
endif
ifdef CONFIG_KB_LAYOUT_ENABLE_BN_NJ
FW_SRCS += $(I)/ime_bengali.c
endif
ifneq ($(TELUGU_IME),)
FW_SRCS += $(I)/ime_telugu.c
endif
EMBED += $(if $(CONFIG_KB_LAYOUT_ENABLE_JA),$(I)/dict/ja.bin) \
	$(if $(CONFIG_KB_LAYOUT_ENABLE_ZH),$(I)/dict/zh.bin)
endif

# ---- the stand-ins ------------------------------------------------------------

# Shared with the Linux program: FreeRTOS on POSIX threads, NVS in a
# file, standby, the clipboard and keyboard layers, the LVGL port and
# its main loop, the display API, the host-managed network and the
# battery (none here).
LINUX_SRCS := $(addprefix $(LINUX)/src/,freertos_linux.c nvs_linux.cpp standby_linux.c \
	clipboard_linux.c keyboard_linux.c lvgl_port_linux.c display_linux.c \
	wifi_host.c battery_linux.c)
# The browser's own: entry point, canvas and input, HTTP, storage,
# system calls, SHA-1.
WEB_SRCS := $(wildcard src/*.c) $(wildcard src/*.cpp)

EMBED_S := $(PDIR)/embed.S

# $(PDIR)/obj/<path>.o; "../" becomes "up/" so objects stay inside build/.
obj = $(patsubst %,$(PDIR)/obj/%.o,$(subst ../,up/,$(1)))

OBJS := $(call obj,$(FW_SRCS) $(LINUX_SRCS) $(WEB_SRCS) $(HB_SRCS)) \
	$(PDIR)/obj/miniz.c.o $(EMBED_S:.S=.o)

# ---- flags ------------------------------------------------------------------

# shim/ first: its headers replace the Linux program's where they
# differ (SHA-1 and base64 without OpenSSL).
INCLUDES := \
	-I$(PDIR) -Ishim/include -Isrc -I$(LINUX)/shim/include -I$(LINUX)/shim -I$(LINUX)/src \
	-I$(LVGL_DIR) -I$(LVGL_DIR)/src \
	-I$(FIRMWARE)/main \
	$(foreach c,editor display sd_card ble_keyboard kb_layout wifi_manager \
		git_sync standby battery fonts usb_msc kermit ime,-I$(COMPONENTS)/$(c)/include) \
	-I$(COMPONENTS)/editor/private_include -I$(F) -I$(G) \
	-I$(COMPONENTS)/harfbuzz/src -I$(BUILD)/miniz

# CLIPBOARD_FILE: the documents folder is the current directory
# (main_web.cpp).
DEFINES := -DLV_CONF_INCLUDE_SIMPLE -D_GNU_SOURCE \
	-DDRAFTLING_VERSION='"$(VERSION)"' -DDRAFTLING_LANG_PACK='"$(PACK)"' \
	-DCLIPBOARD_FILE='"./.clipboard"' \
	-DHB_TINY -DHB_NO_MT -DLINUX_IO_WEB

WARN := -Wno-deprecated-declarations -Wno-string-plus-int -Wno-\#warnings -Wno-c99-designator \
	-Wno-unused-command-line-argument

CFLAGS   := $(OPT) -MMD -MP -pthread $(DEFINES) $(INCLUDES) $(WARN)
CXXFLAGS := $(CFLAGS) -std=gnu++17
HB_FLAGS := -Os -MMD -MP -pthread -DHB_TINY -DHB_NO_MT -fno-exceptions -fno-rtti \
	-DHB_CONFIG_OVERRIDE_LAST_H='"hb_draftling_config.h"' \
	-I$(COMPONENTS)/harfbuzz/config -I$(COMPONENTS)/harfbuzz/src -w

# Threads (the editor's tasks, the Git client) are Web Workers; main()
# runs on one of them too (PROXY_TO_PTHREAD), so the LVGL loop and the
# Git client may block while the page's own thread stays free for the
# DOM. WasmFS gives the program the origin-private file system.
LDFLAGS := $(OPT) -pthread -sPROXY_TO_PTHREAD=1 -sPTHREAD_POOL_SIZE=6 \
	-sWASMFS=1 -sFORCE_FILESYSTEM=1 \
	-sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 -sMAXIMUM_MEMORY=2147483648 \
	-sSTACK_SIZE=1048576 -sDEFAULT_PTHREAD_STACK_SIZE=1048576 \
	-sENVIRONMENT=web,worker -sEXIT_RUNTIME=0 \
	-sEXPORTED_FUNCTIONS=_main,_malloc,_free,_web_input,_web_unsaved,_web_storage,_web_files_request \
	-sEXPORTED_RUNTIME_METHODS=stringToNewUTF8 \
	-Wno-pthreads-mem-growth --post-js src/glue.js \
	-Wl,--wrap=lv_text_get_line_width

# ---- rules ------------------------------------------------------------------

$(PROG): $(OBJS) $(LVGL_LIB) src/glue.js
	@mkdir -p $(dir $@)
	@echo "LD  $@"
	@$(CXX) $(OBJS) $(LVGL_LIB) $(LDFLAGS) -o $@

# The compile flags, kept in a file that changes only when they do:
# every object depends on it, so a new version (DRAFTLING_VERSION, from
# PROJECT_VER), OPT= or another flag change rebuilds the objects.
FLAGS_STAMP := $(PDIR)/compile-flags
FLAGS_NOW   := $(CC) $(CXX) $(CFLAGS) $(CXXFLAGS) $(HB_FLAGS)
ifneq ($(file <$(FLAGS_STAMP)),$(FLAGS_NOW))
$(shell mkdir -p $(PDIR))
$(file >$(FLAGS_STAMP),$(FLAGS_NOW))
endif

GEN_HDRS := $(SDKCONFIG) $(FLAGS_STAMP)

# One run writes all three (a grouped target, GNU make 4.3+).
$(foreach n,10 12 16,$(PDIR)/cjk-all/cjk$(n).bin) &: $(LINUX)/tools/merge_cjk_fonts.py $(wildcard $(F)/cjk/*/cjk*.bin)
	python3 $(LINUX)/tools/merge_cjk_fonts.py $(F)/cjk $(PDIR)/cjk-all

$(EMBED_S): $(EMBED) tools/gen_embed.py $(PDIR)/config.mk
	python3 tools/gen_embed.py $@ $(EMBED)

$(EMBED_S:.S=.o): $(EMBED_S)
	@$(CC) -pthread -c $< -o $@

$(PDIR)/obj/miniz.c.o: $(MINIZ)
	@mkdir -p $(dir $@)
	@echo "CC  miniz.c"
	@$(CC) $(OPT) -pthread -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES -w -c $< -o $@

$(PDIR)/obj/up/%.cc.o: ../%.cc $(GEN_HDRS)
	@mkdir -p $(dir $@)
	@echo "CXX $(notdir $<)"
	@$(CXX) $(HB_FLAGS) -c $< -o $@

$(PDIR)/obj/up/%.c.o: ../%.c $(GEN_HDRS)
	@mkdir -p $(dir $@)
	@echo "CC  $(notdir $<)"
	@$(CC) $(CFLAGS) -std=gnu11 -c $< -o $@

$(PDIR)/obj/up/%.cpp.o: ../%.cpp $(GEN_HDRS)
	@mkdir -p $(dir $@)
	@echo "CXX $(notdir $<)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(PDIR)/obj/src/%.c.o: src/%.c $(GEN_HDRS)
	@mkdir -p $(dir $@)
	@echo "CC  $(notdir $<)"
	@$(CC) $(CFLAGS) -std=gnu11 -c $< -o $@

$(PDIR)/obj/src/%.cpp.o: src/%.cpp $(GEN_HDRS)
	@mkdir -p $(dir $@)
	@echo "CXX $(notdir $<)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(shell find $(PDIR)/obj -name '*.d' 2>/dev/null)
