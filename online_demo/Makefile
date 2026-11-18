# Draftling online demo: the editor firmware compiled to WebAssembly.
#
#   make            fetch LVGL + kconfiglib, build, assemble dist/
#   make serve      build, then serve dist/ on http://localhost:8000/
#   make clean      remove build/ and dist/ (keeps the fetched sources)
#   make distclean  remove everything that is not in git
#
# Needs the Emscripten SDK (emcc / em++ on PATH, see README.md), git,
# curl, python3 and GNU make. The output in dist/ is static files only;
# copy them to the web server.

FIRMWARE   := ../firmware
COMPONENTS := $(FIRMWARE)/components
BUILD      := build
DIST       := dist

# The board the demo simulates: its sdkconfig defaults drive the
# firmware's Kconfig, exactly like `idf.py --preset freenove_fnk0104s`.
BOARD_DEFAULTS := $(FIRMWARE)/sdkconfig.defaults.freenove_fnk0104s

# Pinned dependencies. LVGL matches what the ESP-IDF component manager
# resolves "lvgl/lvgl ^9.2" (main/idf_component.yml) to.
LVGL_VERSION       := v9.6.0
KCONFIGLIB_VERSION := v14.1.0

LVGL_DIR   := $(BUILD)/lvgl
LVGL_STAMP := $(LVGL_DIR)/.fetched-$(LVGL_VERSION)
KCONFIGLIB := $(BUILD)/kconfiglib/kconfiglib.py
SDKCONFIG  := $(BUILD)/sdkconfig.h

VERSION := $(shell sed -n 's/^set(PROJECT_VER "\(.*\)")/\1/p' $(FIRMWARE)/CMakeLists.txt)

CC  := emcc
CXX := em++

OPT ?= -O2

# ---- sources ------------------------------------------------------------

# The firmware's own code, compiled unchanged.
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

# Fonts of the default language pack on a standard-density board
# (Greybeard "lowres" set with the Cyrillic, Hebrew and Latin
# Extended-A subsets), as components/fonts/CMakeLists.txt selects them.
FONT_SLOTS := 11 14 16 18 22 26 30 34
FW_SRCS += $(COMPONENTS)/fonts/lowres.c \
	$(foreach n,$(FONT_SLOTS),$(COMPONENTS)/fonts/lowres_$(n).c) \
	$(foreach s,cyrillic hebrew latina,$(foreach n,$(FONT_SLOTS),$(COMPONENTS)/fonts/lowres_$(s)_$(n).c))

# Stand-ins for the hardware, the network and the RTOS.
DEMO_SRCS := $(wildcard src/*.c) $(wildcard src/*.cpp)

# Filled in by the second make pass, once LVGL has been fetched.
LVGL_SRCS = $(shell find $(LVGL_DIR)/src -name '*.c')

# build/obj/<path>.o; "../" becomes "up/" so firmware objects stay inside build/.
obj = $(patsubst %,$(BUILD)/obj/%.o,$(subst ../,up/,$(1)))

FW_OBJS   := $(call obj,$(FW_SRCS))
DEMO_OBJS := $(call obj,$(DEMO_SRCS))
LVGL_OBJS  = $(call obj,$(LVGL_SRCS))

# ---- flags --------------------------------------------------------------

INCLUDES := \
	-Ishim/include -Ishim -Isrc -I$(BUILD) \
	-I$(LVGL_DIR) -I$(LVGL_DIR)/src \
	-I$(FIRMWARE)/main \
	$(foreach c,editor display sd_card ble_keyboard kb_layout wifi_manager \
		git_sync standby battery fonts usb_msc kermit,-I$(COMPONENTS)/$(c)/include) \
	-I$(COMPONENTS)/editor/private_include

DEFINES := -DLV_CONF_INCLUDE_SIMPLE -D_GNU_SOURCE \
	-DDRAFTLING_VERSION='"$(VERSION)"'

WARN := -Wno-deprecated-declarations -Wno-string-plus-int -Wno-\#warnings -Wno-unused-command-line-argument

CFLAGS   := $(OPT) -MMD -MP $(DEFINES) $(INCLUDES) $(WARN)
CXXFLAGS := $(CFLAGS) -std=gnu++17
LVGL_CFLAGS := $(OPT) -MMD -MP -DLV_CONF_INCLUDE_SIMPLE -Ishim -I$(LVGL_DIR) -Wno-\#warnings

LDFLAGS := $(OPT) \
	-sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=33554432 -sSTACK_SIZE=1048576 \
	-sENVIRONMENT=web -sFORCE_FILESYSTEM=1 -lidbfs.js \
	-sEXPORTED_FUNCTIONS=_main,_demo_key,_demo_pointer \
	-sEXPORTED_RUNTIME_METHODS=FS,IDBFS,HEAPU16,addRunDependency,removeRunDependency \
	-Wl,--wrap=lv_text_get_line_width \
	--embed-file samples@/seed

# ---- targets ------------------------------------------------------------

.PHONY: all deps wasm serve clean distclean

all: deps
	@$(MAKE) --no-print-directory wasm

deps: $(LVGL_STAMP) $(SDKCONFIG)

$(LVGL_STAMP):
	rm -rf $(LVGL_DIR)
	git -c advice.detachedHead=false clone --quiet --depth 1 \
		--branch $(LVGL_VERSION) https://github.com/lvgl/lvgl.git $(LVGL_DIR)
	touch $@

$(KCONFIGLIB):
	mkdir -p $(dir $@)
	curl -sSfL -o $@.tmp \
		https://raw.githubusercontent.com/ulfalizer/Kconfiglib/$(KCONFIGLIB_VERSION)/kconfiglib.py
	mv $@.tmp $@

# Re-run whenever a Kconfig file or the board defaults change.
KCONFIG_INPUTS := tools/Kconfig.demo tools/gen_sdkconfig.py $(BOARD_DEFAULTS) \
	$(FIRMWARE)/main/Kconfig.projbuild \
	$(COMPONENTS)/kb_layout/Kconfig.projbuild \
	$(COMPONENTS)/editor/Kconfig.projbuild

$(SDKCONFIG): $(KCONFIG_INPUTS) $(KCONFIGLIB)
	mkdir -p $(dir $@)
	PYTHONPATH=$(dir $(KCONFIGLIB)) python3 tools/gen_sdkconfig.py \
		$(FIRMWARE) $(BOARD_DEFAULTS) $@

WEB_FILES := $(wildcard web/*)

wasm: $(DIST)/draftling.js $(patsubst web/%,$(DIST)/%,$(WEB_FILES))

$(DIST)/draftling.js: $(FW_OBJS) $(DEMO_OBJS) $(LVGL_OBJS) $(shell find samples -type f)
	mkdir -p $(DIST)
	$(CXX) $(FW_OBJS) $(DEMO_OBJS) $(LVGL_OBJS) $(LDFLAGS) -o $@

# The page loads draftling.js / .wasm with ?v=<version> so a new
# release is not hidden by a cached copy.
$(DIST)/index.html: web/index.html $(FIRMWARE)/CMakeLists.txt
	mkdir -p $(DIST)
	sed 's/@VERSION@/$(VERSION)/g' $< > $@

$(DIST)/%: web/%
	mkdir -p $(dir $@)
	cp $< $@

$(BUILD)/obj/build/lvgl/%.c.o: $(BUILD)/lvgl/%.c
	@mkdir -p $(dir $@)
	@echo "CC  $<"
	@$(CC) $(LVGL_CFLAGS) -c $< -o $@

$(BUILD)/obj/up/%.c.o: ../%.c $(SDKCONFIG)
	@mkdir -p $(dir $@)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/obj/up/%.cpp.o: ../%.cpp $(SDKCONFIG)
	@mkdir -p $(dir $@)
	@echo "CXX $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/obj/src/%.c.o: src/%.c $(SDKCONFIG)
	@mkdir -p $(dir $@)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/obj/src/%.cpp.o: src/%.cpp $(SDKCONFIG)
	@mkdir -p $(dir $@)
	@echo "CXX $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

serve: all
	cd $(DIST) && python3 -m http.server 8000

clean:
	rm -rf $(BUILD)/obj $(SDKCONFIG) $(DIST)

distclean:
	rm -rf $(BUILD) $(DIST)

-include $(shell find $(BUILD)/obj -name '*.d' 2>/dev/null)
