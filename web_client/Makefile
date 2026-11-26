# Draftling web client: the editor firmware compiled to WebAssembly,
# as a web application filling the browser window.
#
#   make -j8        fetch LVGL, kconfiglib and miniz, build the program
#                   (every language), assemble dist/
#   make serve      build, then serve dist/ on http://localhost:8001/
#                   with the headers it needs (tools/serve.py)
#   make clean      remove the build output (keeps the fetched sources)
#   make distclean  remove everything that is not in git
#
# Needs the Emscripten SDK (emcc / em++ on PATH, see README.md), git,
# curl, python3 and GNU make. The output in dist/ is static files only;
# copy them to the web server. The program itself is built by
# client.mk.

FIRMWARE := ../firmware
BUILD    := build
DIST     := dist

# Pinned dependencies, the same as the Linux program's (linux/Makefile).
# LVGL matches what the ESP-IDF component manager resolves
# "lvgl/lvgl ^9.2" (main/idf_component.yml) to; miniz v114 is the zlib
# of the ESP32 ROM the Git client was written against.
LVGL_VERSION       := v9.6.0
KCONFIGLIB_VERSION := v14.1.0
MINIZ_VERSION      := v114

LVGL_DIR   := $(BUILD)/lvgl
LVGL_STAMP := $(LVGL_DIR)/.fetched-$(LVGL_VERSION)
LVGL_LIB   := $(BUILD)/liblvgl.a
KCONFIGLIB := $(BUILD)/kconfiglib/kconfiglib.py
MINIZ      := $(BUILD)/miniz/miniz.c

VERSION := $(shell sed -n 's/^set(PROJECT_VER "\(.*\)")/\1/p' $(FIRMWARE)/CMakeLists.txt)

OPT ?= -O2
export OPT
LVGL_CFLAGS := $(OPT) -pthread -MMD -MP -DLV_CONF_INCLUDE_SIMPLE -I../linux/shim -I$(LVGL_DIR) -Wno-\#warnings

.PHONY: all deps lvgl program web serve clean distclean

all: web

deps: $(LVGL_STAMP) $(KCONFIGLIB) $(MINIZ)

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

$(MINIZ):
	mkdir -p $(dir $@)
	curl -sSfL -o $@.tmp \
		https://raw.githubusercontent.com/richgel999/miniz/$(MINIZ_VERSION)/miniz.c
	mv $@.tmp $@

lvgl: deps
	@$(MAKE) --no-print-directory -f lvgl.mk LVGL_DIR=$(LVGL_DIR) \
		LVGL_LIB=$(LVGL_LIB) LVGL_CFLAGS='$(LVGL_CFLAGS)'

program: lvgl
	@$(MAKE) --no-print-directory -f client.mk

# Every file of the site but index.html and sw.js, which name the build.
WEB_FILES := $(filter-out web/index.html web/sw.js,$(wildcard web/*))

web: program
	@rm -rf $(DIST)
	@mkdir -p $(DIST)
	cp $(WEB_FILES) $(BUILD)/out/draftling.js $(BUILD)/out/draftling.wasm $(DIST)/
	cp ../linux/packaging/draftling.svg $(DIST)/icon.svg
	@# The page loads every other file with ?v=<hash of the site's files>,
	@# and the service worker keeps that build's files for offline use,
	@# so a browser never mixes files of two builds.
	@build=$$(cd $(DIST) && find . -type f | sort | xargs sha256sum | sha256sum | cut -c1-12); \
	 files=$$(cd $(DIST) && ls | sed 's/.*/"&"/' | paste -sd, -); \
	 sed -e 's/@VERSION@/$(VERSION)/g' -e "s/@BUILD@/$$build/g" web/index.html > $(DIST)/index.html; \
	 sed -e "s/@BUILD@/$$build/g" -e "s/@FILES@/$$files/" web/sw.js > $(DIST)/sw.js; \
	 echo "Version $(VERSION), build id $$build"

serve: all
	python3 tools/serve.py $(DIST) 8001

clean:
	rm -rf $(BUILD)/p $(BUILD)/out $(BUILD)/lvgl-obj $(LVGL_LIB) $(DIST)

distclean:
	rm -rf $(BUILD) $(DIST)
