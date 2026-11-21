# One Draftling program: one language pack. Run by the Makefile:
#
#   make -f draftling.mk PACK=<language pack> IO="wayland x11 fbdev"
#
# linux/sdkconfig.defaults (the DRAFTLING_MODEL_LINUX "board") plus,
# for a pack other than "default", sdkconfig.defaults.lang_<pack> drive
# the firmware's Kconfig, like `idf.py --preset <board> --lang-pack
# <pack>`. The resulting CONFIG_* values are read back into make
# (config.mk) and select the sources, font data and input methods
# exactly as the firmware's component CMakeLists.txt files do.
# Output: build/bin/draftling (default pack) or draftling-<pack>.

ifndef PACK
$(error PACK is not set)
endif

FIRMWARE   := ../firmware
COMPONENTS := $(FIRMWARE)/components
BUILD      := build
LVGL_DIR   := $(BUILD)/lvgl
LVGL_LIB   := $(BUILD)/liblvgl.a
KCONFIGLIB := $(BUILD)/kconfiglib/kconfiglib.py
MINIZ      := $(BUILD)/miniz/miniz.c

# Objects depend on the backends compiled in (-DLINUX_IO_*): one
# folder per pack and backend set.
empty :=
space := $(empty) $(empty)
PDIR := $(BUILD)/p/$(PACK)-$(subst $(space),_,$(strip $(IO)))
PROG := $(BUILD)/bin/draftling$(if $(filter default,$(PACK)),,-$(PACK))

SDKCONFIG := $(PDIR)/sdkconfig.h
DEFAULTS  := sdkconfig.defaults
ifneq ($(PACK),default)
DEFAULTS  += $(FIRMWARE)/sdkconfig.defaults.lang_$(PACK)
endif

VERSION := $(shell sed -n 's/^set(PROJECT_VER "\(.*\)")/\1/p' $(FIRMWARE)/CMakeLists.txt)

OPT ?= -O2 -g

all: $(PROG)

# ---- configuration --------------------------------------------------------

KCONFIG_INPUTS := tools/Kconfig.linux tools/gen_sdkconfig.py $(DEFAULTS) \
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
ifndef CONFIG_DRAFTLING_MODEL_LINUX
$(error The Kconfig did not select DRAFTLING_MODEL_LINUX -- check sdkconfig.defaults)
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

# Fonts, as components/fonts/CMakeLists.txt selects them. The Linux
# "board" is high-density: only the hi-res (Hack) set.
F := $(COMPONENTS)/fonts
LATINX := $(if $(CONFIG_KB_LAYOUT_ENABLE_VI)$(CONFIG_DRAFTLING_HAS_CJK),y)
ifneq ($(HAVE_CONFIG),)
ifndef CONFIG_DRAFTLING_DISPLAY_HIDPI
$(error The Linux build expects CONFIG_DRAFTLING_DISPLAY_HIDPI)
endif
endif
SET   := hires
SLOTS := 9 11 14 16 18 22 26 30
SUBSETS := $(if $(CONFIG_KB_LAYOUT_ENABLE_UA),cyrillic) \
	$(if $(CONFIG_KB_LAYOUT_ENABLE_HE),hebrew) \
	$(if $(LATINX),latinx) \
	$(if $(CONFIG_DRAFTLING_HAS_LATIN_EXT_A),latina)
FW_SRCS += $(F)/$(SET).c $(foreach n,$(SLOTS),$(F)/$(SET)_$(n).c) \
	$(foreach s,$(SUBSETS),$(foreach n,$(SLOTS),$(F)/$(SET)_$(s)_$(n).c))

ifdef CONFIG_DRAFTLING_HAS_CJK
CJK_SET := $(subst ",,$(CONFIG_DRAFTLING_CJK_FONT_SET))
FW_SRCS += $(F)/cjk_font.c
ifeq ($(CJK_SET),all)
# The "all" pack: the ja, zh and ko sets merged at build time.
CJK_DIR := $(PDIR)/cjk-all
else
CJK_DIR := $(F)/cjk/$(CJK_SET)
endif
EMBED   += $(foreach n,10 12 16,$(CJK_DIR)/cjk$(n).bin)
endif

ifdef CONFIG_DRAFTLING_HAS_TAMIL
FW_SRCS += $(F)/tamil_font.c
EMBED   += $(foreach n,$(SLOTS),$(F)/tamil/tamil_$(SET)_$(n).bin)
endif

INDIC_SCRIPTS := $(if $(CONFIG_DRAFTLING_HAS_DEVANAGARI),deva) $(if $(CONFIG_DRAFTLING_HAS_BENGALI),beng)
ifdef CONFIG_DRAFTLING_HAS_INDIC_SHAPING
FW_SRCS += $(F)/indic_font.c
EMBED   += $(foreach s,$(INDIC_SCRIPTS),$(F)/$(s)/$(s)_layout.ttf \
	$(foreach n,$(SLOTS),$(F)/$(s)/$(s)_$(SET)_$(n).bin))
HB_SRCS := $(COMPONENTS)/harfbuzz/src/harfbuzz.cc
endif

# Input methods, as components/ime/CMakeLists.txt selects them.
I := $(COMPONENTS)/ime
TAMIL_IME := $(CONFIG_KB_LAYOUT_ENABLE_TA99)$(CONFIG_KB_LAYOUT_ENABLE_TA_TW)$(CONFIG_KB_LAYOUT_ENABLE_TA_TV)
HAS_IME := $(CONFIG_KB_LAYOUT_ENABLE_VI)$(CONFIG_KB_LAYOUT_ENABLE_KO)$(CONFIG_KB_LAYOUT_ENABLE_JA)$(CONFIG_KB_LAYOUT_ENABLE_ZH)$(TAMIL_IME)$(CONFIG_KB_LAYOUT_ENABLE_BN_NJ)
ifneq ($(HAS_IME),)
FW_SRCS += $(foreach s,ime ime_builtin ime_telex ime_hangul ime_kana ime_pinyin,$(I)/$(s).c)
ifneq ($(TAMIL_IME),)
FW_SRCS += $(foreach s,ime_tamil ime_tamil99 ime_tamil_tw ime_taravinyas,$(I)/$(s).c)
endif
ifdef CONFIG_KB_LAYOUT_ENABLE_BN_NJ
FW_SRCS += $(I)/ime_bengali.c
endif
EMBED += $(if $(CONFIG_KB_LAYOUT_ENABLE_JA),$(I)/dict/ja.bin) \
	$(if $(CONFIG_KB_LAYOUT_ENABLE_ZH),$(I)/dict/zh.bin)
endif

# ---- the Linux stand-ins and the I/O backends -----------------------------

LINUX_SRCS := $(filter-out src/io_%.c,$(wildcard src/*.c) $(wildcard src/*.cpp))
LINUX_SRCS += $(foreach b,$(IO),src/io_$(b).c)

IO_DEFS := $(foreach b,$(IO),-DLINUX_IO_$(shell echo $(b) | tr a-z A-Z))

# Static archives of the system's libraries: link them with -Bstatic,
# leaving the C library's own (pthread, dl, m, rt) shared.
SYSLIBS := -lpthread -pthread -ldl -lm -lrt -lc
static_of = $(filter-out $(SYSLIBS),$(1))

PKG_INC :=
PKG_STATIC :=

# OpenSSL (TLS for Git sync and Draftbox, SHA-1).
OPENSSL_LIBS := $(shell pkg-config --static --libs libssl libcrypto 2>/dev/null)
ifeq ($(OPENSSL_LIBS),)
$(error OpenSSL development files not found (pkg-config libssl libcrypto) -- see README.md)
endif
PKG_INC    += $(shell pkg-config --cflags libssl libcrypto)
PKG_STATIC += $(call static_of,$(OPENSSL_LIBS))

# Wayland: libwayland-client built by wayland.mk, protocol code from
# wayland-protocols, generated with wayland-scanner.
ifneq ($(filter wayland,$(IO)),)
WL_INST  := $(BUILD)/wayland-inst
WL_PROTO := $(BUILD)/wayland-protocols
WL_GEN   := $(BUILD)/wayland-gen
WL_XML_xdg-shell := $(WL_PROTO)/stable/xdg-shell/xdg-shell.xml
WL_XML_xdg-decoration := $(WL_PROTO)/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
WL_PROTOS := xdg-shell xdg-decoration
WL_HDRS := $(foreach p,$(WL_PROTOS),$(WL_GEN)/$(p)-client-protocol.h)
WL_SRCS := $(foreach p,$(WL_PROTOS),$(WL_GEN)/$(p)-protocol.c)
WAYLAND_SCANNER ?= wayland-scanner
PKG_INC += -I$(WL_INST)/include -I$(WL_GEN)
FFI_LIBS := $(shell pkg-config --static --libs libffi 2>/dev/null || echo -lffi)
PKG_STATIC += $(WL_INST)/lib/libwayland-client.a $(call static_of,$(FFI_LIBS))
endif

# X11 through XCB (+ the XKB extension for detectable auto-repeat).
ifneq ($(filter x11,$(IO)),)
XCB_LIBS := $(shell pkg-config --static --libs xcb xcb-xkb 2>/dev/null)
ifeq ($(XCB_LIBS),)
$(error XCB development files not found (pkg-config xcb xcb-xkb) -- see README.md)
endif
PKG_INC    += $(shell pkg-config --cflags xcb xcb-xkb)
PKG_STATIC += $(call static_of,$(XCB_LIBS))
endif

EMBED_S := $(PDIR)/embed.S

# $(PDIR)/obj/<path>.o; "../" becomes "up/" so objects stay inside build/.
obj = $(patsubst %,$(PDIR)/obj/%.o,$(subst ../,up/,$(1)))

OBJS := $(call obj,$(FW_SRCS) $(LINUX_SRCS) $(HB_SRCS)) \
	$(PDIR)/obj/miniz.c.o $(EMBED_S:.S=.o) \
	$(patsubst $(WL_GEN)/%.c,$(PDIR)/obj/wl/%.c.o,$(WL_SRCS))

# ---- flags ------------------------------------------------------------------

INCLUDES := \
	-I$(PDIR) -Ishim/include -Ishim -Isrc \
	-I$(LVGL_DIR) -I$(LVGL_DIR)/src \
	-I$(FIRMWARE)/main \
	$(foreach c,editor display sd_card ble_keyboard kb_layout wifi_manager \
		git_sync standby battery fonts usb_msc kermit ime,-I$(COMPONENTS)/$(c)/include) \
	-I$(COMPONENTS)/editor/private_include -I$(F) -I$(G) \
	-I$(COMPONENTS)/harfbuzz/src -I$(BUILD)/miniz \
	$(PKG_INC)

# CLIPBOARD_FILE: the data folder is the current directory (main_linux.cpp).
DEFINES := -DLV_CONF_INCLUDE_SIMPLE -D_GNU_SOURCE \
	-DDRAFTLING_VERSION='"$(VERSION)"' -DDRAFTLING_LANG_PACK='"$(PACK)"' \
	-DCLIPBOARD_FILE='"./.clipboard"' \
	-DHB_TINY -DHB_NO_MT $(IO_DEFS)

WARN := -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable \
	-Wno-deprecated-declarations -Wno-missing-field-initializers -Wno-cpp \
	-Wno-format-truncation -Wno-stringop-truncation -Wno-sign-compare

CFLAGS   := $(OPT) -MMD -MP -pthread $(DEFINES) $(INCLUDES) $(WARN)
CXXFLAGS := $(CFLAGS) -std=gnu++17
HB_FLAGS := -Os -MMD -MP -DHB_TINY -DHB_NO_MT -fno-exceptions -fno-rtti \
	-DHB_CONFIG_OVERRIDE_LAST_H='"hb_draftling_config.h"' \
	-I$(COMPONENTS)/harfbuzz/config -I$(COMPONENTS)/harfbuzz/src -w

LDFLAGS := $(OPT) -pthread -static-libgcc -static-libstdc++ \
	-Wl,--wrap=lv_text_get_line_width -Wl,--as-needed

LDLIBS := $(LVGL_LIB) -Wl,-Bstatic $(PKG_STATIC) -Wl,-Bdynamic -ldl -lm -lrt

# ---- rules ------------------------------------------------------------------

$(PROG): $(OBJS) $(LVGL_LIB)
	@mkdir -p $(dir $@)
	@echo "LD  $@"
	@$(CXX) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

# Every object waits for the generated headers.
GEN_HDRS := $(SDKCONFIG) $(WL_HDRS)

$(PDIR)/cjk-all/cjk%.bin: tools/merge_cjk_fonts.py $(wildcard $(F)/cjk/*/cjk*.bin)
	python3 tools/merge_cjk_fonts.py $(F)/cjk $(dir $@)

$(EMBED_S): $(EMBED) tools/gen_embed.py $(PDIR)/config.mk
	python3 tools/gen_embed.py $@ $(EMBED)

$(EMBED_S:.S=.o): $(EMBED_S)
	@$(CC) -c $< -o $@

$(PDIR)/obj/miniz.c.o: $(MINIZ)
	@mkdir -p $(dir $@)
	@echo "CC  $(PACK) miniz.c"
	@$(CC) $(OPT) -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES -w -c $< -o $@

$(WL_GEN)/%-client-protocol.h:
	@mkdir -p $(dir $@)
	$(WAYLAND_SCANNER) client-header $(WL_XML_$*) $@

$(WL_GEN)/%-protocol.c:
	@mkdir -p $(dir $@)
	$(WAYLAND_SCANNER) private-code $(WL_XML_$*) $@

$(PDIR)/obj/wl/%.c.o: $(WL_GEN)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(OPT) -I$(WL_INST)/include -c $< -o $@

$(PDIR)/obj/up/%.cc.o: ../%.cc $(GEN_HDRS)
	@mkdir -p $(dir $@)
	@echo "CXX $(PACK) $(notdir $<)"
	@$(CXX) $(HB_FLAGS) -c $< -o $@

$(PDIR)/obj/up/%.c.o: ../%.c $(GEN_HDRS)
	@mkdir -p $(dir $@)
	@echo "CC  $(PACK) $(notdir $<)"
	@$(CC) $(CFLAGS) -std=gnu11 -c $< -o $@

$(PDIR)/obj/up/%.cpp.o: ../%.cpp $(GEN_HDRS)
	@mkdir -p $(dir $@)
	@echo "CXX $(PACK) $(notdir $<)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(PDIR)/obj/src/%.c.o: src/%.c $(GEN_HDRS)
	@mkdir -p $(dir $@)
	@echo "CC  $(PACK) $(notdir $<)"
	@$(CC) $(CFLAGS) -std=gnu11 -c $< -o $@

$(PDIR)/obj/src/%.cpp.o: src/%.cpp $(GEN_HDRS)
	@mkdir -p $(dir $@)
	@echo "CXX $(PACK) $(notdir $<)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(shell find $(PDIR)/obj -name '*.d' 2>/dev/null)
