# libwayland-client as a static library, built from the pinned release
# with meson (distributions ship only the shared library). Run by the
# Makefile. Needs meson, ninja, expat's headers (for wayland-scanner)
# and libffi's headers and static library -- for a cross build the
# target's (libffi-dev:<arch>), through a generated meson cross file
# (arch.mk). The source is shared, the build is per target.
#
# wayland-scanner is built first from the same source, for this host
# (build/wayland-scanner-inst/, shared by every target): the library's
# meson.build asks pkg-config for a wayland-scanner of exactly its own
# version, which a distribution's (Ubuntu 22.04: 1.20, no .pc file)
# need not be. draftling.mk generates the protocol code with it too.

include arch.mk

WAYLAND_SRC   := $(BUILD)/wayland
WAYLAND_STAMP := $(WAYLAND_SRC)/.fetched-$(WAYLAND_VERSION)
WAYLAND_OBJ   := $(OUT)/wayland-build
WAYLAND_INST  := $(abspath $(OUT)/wayland-inst)
CROSS_FILE    := $(OUT)/meson-cross.ini
SCANNER_OBJ   := $(BUILD)/wayland-scanner-build
SCANNER_INST  := $(abspath $(BUILD)/wayland-scanner-inst)
SCANNER       := $(SCANNER_INST)/bin/wayland-scanner
# The scanner runs on this host: its compiler, not the target's.
BUILD_CC      ?= cc

ifneq ($(CROSS),)
MESON_CROSS := --cross-file $(CROSS_FILE)
endif

$(WAYLAND_INST)/lib/libwayland-client.a: $(WAYLAND_STAMP) $(SCANNER) $(if $(CROSS),$(CROSS_FILE))
	rm -rf $(WAYLAND_OBJ) $(WAYLAND_INST)
	PKG_CONFIG_PATH=$(SCANNER_INST)/lib/pkgconfig \
	meson setup $(WAYLAND_OBJ) $(WAYLAND_SRC) $(MESON_CROSS) \
		--prefix=$(WAYLAND_INST) --libdir=lib --buildtype=release \
		-Ddefault_library=static -Dlibraries=true -Dscanner=false \
		-Dtests=false -Ddocumentation=false -Ddtd_validation=false >/dev/null
	ninja -C $(WAYLAND_OBJ) install >/dev/null
	test -f $@

$(SCANNER): $(WAYLAND_STAMP)
	rm -rf $(SCANNER_OBJ) $(SCANNER_INST)
	env -u PKG_CONFIG_LIBDIR -u PKG_CONFIG_PATH CC=$(BUILD_CC) \
	meson setup $(SCANNER_OBJ) $(WAYLAND_SRC) \
		--prefix=$(SCANNER_INST) --libdir=lib --buildtype=release \
		-Dlibraries=false -Dscanner=true \
		-Dtests=false -Ddocumentation=false -Ddtd_validation=false >/dev/null
	ninja -C $(SCANNER_OBJ) install >/dev/null
	test -x $@

$(WAYLAND_STAMP):
	rm -rf $(WAYLAND_SRC)
	git -c advice.detachedHead=false clone --quiet --depth 1 \
		--branch $(WAYLAND_VERSION) \
		https://gitlab.freedesktop.org/wayland/wayland.git $(WAYLAND_SRC)
	touch $@

$(CROSS_FILE): arch.mk wayland.mk
	@mkdir -p $(dir $@)
	printf '%s\n' \
		"[binaries]" \
		"c = '$(CC)'" \
		"cpp = '$(CXX)'" \
		"ar = '$(AR)'" \
		"strip = '$(STRIP)'" \
		"pkg-config = 'pkg-config'" \
		"[properties]" \
		"pkg_config_libdir = '$(PKG_CONFIG_LIBDIR)'" \
		"[host_machine]" \
		"system = 'linux'" \
		"cpu_family = '$(MESON_CPU)'" \
		"cpu = '$(MESON_CPU)'" \
		"endian = 'little'" > $@
