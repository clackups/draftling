# One variant of the online demo: BOARD x PACK. Run by the Makefile:
#
#   make -f variant.mk BOARD=<board> PACK=<language pack>
#
# The board's sdkconfig.defaults.<board> plus, for a pack other than
# "default", sdkconfig.defaults.lang_<pack> drive the firmware's Kconfig,
# like `idf.py --preset <board> --lang-pack <pack>`. The resulting
# CONFIG_* values are read back into make (config.mk) and select the
# sources, font data and input methods exactly as the firmware's
# component CMakeLists.txt files do. Output: dist/fw/<board>/<pack>/.

ifndef BOARD
$(error BOARD is not set)
endif
ifndef PACK
$(error PACK is not set)
endif

FIRMWARE   := ../firmware
COMPONENTS := $(FIRMWARE)/components
BUILD      := build
LVGL_DIR   := $(BUILD)/lvgl
LVGL_LIB   := $(BUILD)/liblvgl.a
KCONFIGLIB := $(BUILD)/kconfiglib/kconfiglib.py

VDIR := $(BUILD)/v/$(BOARD)/$(PACK)
ODIR := dist/fw/$(BOARD)/$(PACK)

SDKCONFIG := $(VDIR)/sdkconfig.h
DEFAULTS  := $(FIRMWARE)/sdkconfig.defaults.$(BOARD)
ifneq ($(PACK),default)
DEFAULTS  += $(FIRMWARE)/sdkconfig.defaults.lang_$(PACK)
endif

VERSION := $(shell sed -n 's/^set(PROJECT_VER "\(.*\)")/\1/p' $(FIRMWARE)/CMakeLists.txt)

CC  := emcc
CXX := em++
OPT ?= -O2

all: $(ODIR)/draftling.js

# ---- configuration --------------------------------------------------------

KCONFIG_INPUTS := tools/Kconfig.demo tools/gen_sdkconfig.py $(DEFAULTS) \
	$(FIRMWARE)/main/Kconfig.projbuild \
	$(COMPONENTS)/kb_layout/Kconfig.projbuild \
	$(COMPONENTS)/editor/Kconfig.projbuild

$(SDKCONFIG): $(KCONFIG_INPUTS) $(KCONFIGLIB)
	@mkdir -p $(dir $@)
	PYTHONPATH=$(dir $(KCONFIGLIB)) python3 tools/gen_sdkconfig.py \
		$(FIRMWARE) $@ $(DEFAULTS)

# sdkconfig.h as make variables (CONFIG_FOO := 1); make rebuilds this
# file first and restarts, so everything below can test the options.
$(VDIR)/config.mk: $(SDKCONFIG)
	sed -n 's/^#define \(CONFIG_[A-Za-z0-9_]*\) \(.*\)$$/\1 := \2/p' $< > $@

include $(VDIR)/config.mk

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

EMBED :=

# Fonts, as components/fonts/CMakeLists.txt selects them.
F := $(COMPONENTS)/fonts
LATINX := $(if $(CONFIG_KB_LAYOUT_ENABLE_VI)$(CONFIG_DRAFTLING_HAS_CJK),y)
ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
SET   := hires
SLOTS := 9 11 14 16 18 22 26 30
else
SET   := lowres
SLOTS := 11 14 16 18 22 26 30 34
endif
SUBSETS := $(if $(CONFIG_KB_LAYOUT_ENABLE_UA),cyrillic) \
	$(if $(CONFIG_KB_LAYOUT_ENABLE_HE),hebrew) \
	$(if $(LATINX),latinx) \
	$(if $(CONFIG_DRAFTLING_HAS_LATIN_EXT_A),latina)
FW_SRCS += $(F)/$(SET).c $(foreach n,$(SLOTS),$(F)/$(SET)_$(n).c) \
	$(foreach s,$(SUBSETS),$(foreach n,$(SLOTS),$(F)/$(SET)_$(s)_$(n).c))

ifdef CONFIG_DRAFTLING_HAS_CJK
CJK_SET := $(subst ",,$(CONFIG_DRAFTLING_CJK_FONT_SET))
FW_SRCS += $(F)/cjk_font.c
EMBED   += $(foreach n,10 12 16,$(F)/cjk/$(CJK_SET)/cjk$(n).bin)
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

# Stand-ins for the hardware, the network and the RTOS.
DEMO_SRCS := $(wildcard src/*.c) $(wildcard src/*.cpp)

EMBED_S := $(VDIR)/embed.S

# $(VDIR)/obj/<path>.o; "../" becomes "up/" so objects stay inside build/.
obj = $(patsubst %,$(VDIR)/obj/%.o,$(subst ../,up/,$(1)))

OBJS := $(call obj,$(FW_SRCS) $(DEMO_SRCS) $(HB_SRCS)) $(EMBED_S:.S=.o)

# ---- flags ------------------------------------------------------------------

INCLUDES := \
	-I$(VDIR) -Ishim/include -Ishim -Isrc \
	-I$(LVGL_DIR) -I$(LVGL_DIR)/src \
	-I$(FIRMWARE)/main \
	$(foreach c,editor display sd_card ble_keyboard kb_layout wifi_manager \
		git_sync standby battery fonts usb_msc kermit ime,-I$(COMPONENTS)/$(c)/include) \
	-I$(COMPONENTS)/editor/private_include -I$(F) -I$(COMPONENTS)/harfbuzz/src

DEFINES := -DLV_CONF_INCLUDE_SIMPLE -D_GNU_SOURCE \
	-DDRAFTLING_VERSION='"$(VERSION)"' -DDEMO_BOARD='"$(BOARD)"' \
	-DHB_TINY -DHB_NO_MT

WARN := -Wno-deprecated-declarations -Wno-string-plus-int -Wno-\#warnings -Wno-c99-designator \
	-Wno-unused-command-line-argument

CFLAGS   := $(OPT) -MMD -MP $(DEFINES) $(INCLUDES) $(WARN)
CXXFLAGS := $(CFLAGS) -std=gnu++17
HB_FLAGS := -Os -MMD -MP -DHB_TINY -DHB_NO_MT -fno-exceptions -fno-rtti \
	-DHB_CONFIG_OVERRIDE_LAST_H='"hb_draftling_config.h"' \
	-I$(COMPONENTS)/harfbuzz/config -I$(COMPONENTS)/harfbuzz/src -w

LDFLAGS := $(OPT) \
	-sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=33554432 -sSTACK_SIZE=1048576 \
	-sENVIRONMENT=web -sFORCE_FILESYSTEM=1 -lidbfs.js \
	-sEXPORTED_FUNCTIONS=_main,_demo_key,_demo_pointer,_demo_button \
	-sEXPORTED_RUNTIME_METHODS=FS,IDBFS,HEAPU8,HEAPU16,addRunDependency,removeRunDependency \
	-Wl,--wrap=lv_text_get_line_width \
	--embed-file samples@/seed

# ---- rules ------------------------------------------------------------------

$(ODIR)/draftling.js: $(OBJS) $(LVGL_LIB) $(shell find samples -type f)
	@mkdir -p $(dir $@)
	@echo "LD  $@"
	@$(CXX) $(OBJS) $(LVGL_LIB) $(LDFLAGS) -o $@

$(EMBED_S): $(EMBED) tools/gen_embed.py $(VDIR)/config.mk
	python3 tools/gen_embed.py $@ $(EMBED)

$(EMBED_S:.S=.o): $(EMBED_S)
	@$(CC) -c $< -o $@

$(VDIR)/obj/up/%.cc.o: ../%.cc $(SDKCONFIG)
	@mkdir -p $(dir $@)
	@echo "CXX $(BOARD)/$(PACK) $(notdir $<)"
	@$(CXX) $(HB_FLAGS) -c $< -o $@

$(VDIR)/obj/up/%.c.o: ../%.c $(SDKCONFIG)
	@mkdir -p $(dir $@)
	@echo "CC  $(BOARD)/$(PACK) $(notdir $<)"
	@$(CC) $(CFLAGS) -c $< -o $@

$(VDIR)/obj/up/%.cpp.o: ../%.cpp $(SDKCONFIG)
	@mkdir -p $(dir $@)
	@echo "CXX $(BOARD)/$(PACK) $(notdir $<)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(VDIR)/obj/src/%.c.o: src/%.c $(SDKCONFIG)
	@mkdir -p $(dir $@)
	@echo "CC  $(BOARD)/$(PACK) $(notdir $<)"
	@$(CC) $(CFLAGS) -c $< -o $@

$(VDIR)/obj/src/%.cpp.o: src/%.cpp $(SDKCONFIG)
	@mkdir -p $(dir $@)
	@echo "CXX $(BOARD)/$(PACK) $(notdir $<)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(shell find $(VDIR)/obj -name '*.d' 2>/dev/null)
