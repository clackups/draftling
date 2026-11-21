# Draftling for Linux: the editor firmware as a native program.
#
#   make -j8                    fetch the pinned dependencies, build
#                               build/bin/draftling (default language pack)
#   make -j8 PACKS="default japanese"
#                               one program per language pack:
#                               draftling, draftling-japanese, ...
#   make IO="x11 fbdev"         only some of the I/O backends
#                               (default: wayland x11 fbdev)
#   make run ARGS="-v"          build and run the default pack
#   make install DESTDIR=... PREFIX=/usr
#   make deb                    Debian package in dist/ (packaging/deb/)
#   make clean / distclean
#
# The host needs a C / C++ compiler, GNU make, git, curl, python3, and
# the static libraries and headers of the backends chosen in IO and of
# OpenSSL -- see README.md for the package list. Everything other than
# the C library is linked statically.

FIRMWARE := ../firmware
BUILD    := build
DIST     := dist

PACKS ?= default
IO    ?= wayland x11 fbdev

PREFIX ?= /usr/local
DESTDIR ?=

UNAME := $(shell uname -s)
ifneq ($(UNAME),Linux)
ifneq ($(filter fbdev,$(IO)),)
$(warning The framebuffer backend is Linux-only; leaving it out on $(UNAME))
IO := $(filter-out fbdev,$(IO))
endif
endif

BAD_IO := $(filter-out wayland x11 fbdev,$(IO))
ifneq ($(BAD_IO),)
$(error Unknown I/O backend(s): $(BAD_IO) (known: wayland x11 fbdev))
endif
ifeq ($(strip $(IO)),)
$(error IO is empty: pick at least one of wayland x11 fbdev)
endif

# Pinned dependencies. LVGL matches what the ESP-IDF component manager
# resolves "lvgl/lvgl ^9.2" (main/idf_component.yml) to; miniz v114 is
# the zlib of the ESP32 ROM the Git client was written against.
LVGL_VERSION       := v9.6.0
KCONFIGLIB_VERSION := v14.1.0
MINIZ_VERSION      := v114
WAYLAND_VERSION    := 1.24.0
WAYLAND_PROTOCOLS_VERSION := 1.47

LVGL_DIR   := $(BUILD)/lvgl
LVGL_STAMP := $(LVGL_DIR)/.fetched-$(LVGL_VERSION)
LVGL_LIB   := $(BUILD)/liblvgl.a
KCONFIGLIB := $(BUILD)/kconfiglib/kconfiglib.py
MINIZ      := $(BUILD)/miniz/miniz.c
WAYLAND_LIB := $(BUILD)/wayland-inst/lib/libwayland-client.a
WAYLAND_PROTOCOLS := $(BUILD)/wayland-protocols/.fetched-$(WAYLAND_PROTOCOLS_VERSION)

VERSION := $(shell sed -n 's/^set(PROJECT_VER "\(.*\)")/\1/p' $(FIRMWARE)/CMakeLists.txt)

ifeq ($(origin CC),default)
CC := cc
endif
ifeq ($(origin CXX),default)
CXX := c++
endif
OPT ?= -O2 -g
LVGL_CFLAGS := $(OPT) -MMD -MP -DLV_CONF_INCLUDE_SIMPLE -Ishim -I$(LVGL_DIR) -Wno-cpp

export CC CXX OPT

DEPS := $(LVGL_STAMP) $(KCONFIGLIB) $(MINIZ)
ifneq ($(filter wayland,$(IO)),)
DEPS += $(WAYLAND_LIB) $(WAYLAND_PROTOCOLS)
endif

.PHONY: all deps lvgl programs run install uninstall deb clean distclean check-host

all: programs

deps: $(DEPS)

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

$(WAYLAND_LIB):
	@$(MAKE) --no-print-directory -f wayland.mk BUILD=$(BUILD) \
		WAYLAND_VERSION=$(WAYLAND_VERSION)

$(WAYLAND_PROTOCOLS):
	rm -rf $(dir $@)
	git -c advice.detachedHead=false clone --quiet --depth 1 \
		--branch $(WAYLAND_PROTOCOLS_VERSION) \
		https://gitlab.freedesktop.org/wayland/wayland-protocols.git $(dir $@)
	touch $@

# LVGL does not depend on the language pack: one library for all.
lvgl: deps
	@$(MAKE) --no-print-directory -f lvgl.mk LVGL_DIR=$(LVGL_DIR) \
		LVGL_LIB=$(LVGL_LIB) LVGL_CFLAGS='$(LVGL_CFLAGS)'

programs: $(addprefix program-,$(PACKS))

program-%: lvgl
	@$(MAKE) --no-print-directory -f draftling.mk PACK=$* IO='$(IO)'

prog = $(BUILD)/bin/draftling$(if $(filter default,$(1)),,-$(1))

run: program-default
	$(call prog,default) $(ARGS)

# ---- installation ------------------------------------------------------------

BINDIR   := $(DESTDIR)$(PREFIX)/bin
SHAREDIR := $(DESTDIR)$(PREFIX)/share

install: programs
	install -d $(BINDIR) $(SHAREDIR)/applications $(SHAREDIR)/man/man1 \
		$(SHAREDIR)/doc/draftling $(SHAREDIR)/metainfo
	$(foreach p,$(PACKS),install -m 0755 $(call prog,$(p)) $(BINDIR)/;)
	install -m 0644 packaging/draftling.desktop $(SHAREDIR)/applications/
	install -m 0644 packaging/art.draftling.Draftling.metainfo.xml $(SHAREDIR)/metainfo/
	sed 's/@VERSION@/$(VERSION)/' packaging/draftling.1 | gzip -9n > $(SHAREDIR)/man/man1/draftling.1.gz
	chmod 0644 $(SHAREDIR)/man/man1/draftling.1.gz
	install -m 0644 README.md $(SHAREDIR)/doc/draftling/README.md
	install -m 0644 ../LICENSE $(SHAREDIR)/doc/draftling/LICENSE

uninstall:
	rm -f $(BINDIR)/draftling $(BINDIR)/draftling-* \
		$(SHAREDIR)/applications/draftling.desktop \
		$(SHAREDIR)/metainfo/art.draftling.Draftling.metainfo.xml \
		$(SHAREDIR)/man/man1/draftling.1.gz
	rm -rf $(SHAREDIR)/doc/draftling

deb: programs
	packaging/deb/build-deb.sh "$(VERSION)" "$(PACKS)" "$(DIST)"

# Report which build requirements the host lacks (README.md).
check-host:
	@IO='$(IO)' tools/check-host.sh

clean:
	rm -rf $(BUILD)/p $(BUILD)/lvgl-obj $(LVGL_LIB) $(BUILD)/bin $(DIST)

distclean:
	rm -rf $(BUILD) $(DIST)
