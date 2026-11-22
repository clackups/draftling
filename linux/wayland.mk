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

# The cross file's key for pkg-config: "pkg-config" from meson 1.3 on,
# "pkgconfig" before (Ubuntu 22.04 has 0.61, which ignores the new name
# and then finds no pkg-config for the target: "Pkg-config binary for
# machine 1 not found").
MESON_PKGCONFIG_KEY := $(shell meson --version 2>/dev/null | \
	awk -F. '{ print ($$1 > 1 || ($$1 == 1 && $$2 >= 3)) ? "pkg-config" : "pkgconfig" }')

# meson and ninja print their errors on stdout: keep it in a log and
# show the end of it when a step fails.
#
# In a cross build meson looks for wayland-scanner, a build-machine
# dependency, with PKG_CONFIG_PATH_FOR_BUILD (meson 0.61, Ubuntu 22.04),
# not PKG_CONFIG_PATH: the recipe below sets both.
quiet_log = > $(1) 2>&1 || { tail -n 25 $(1); echo "(full log: $(1))"; exit 1; }

$(WAYLAND_INST)/lib/libwayland-client.a: $(WAYLAND_STAMP) $(SCANNER) $(if $(CROSS),$(CROSS_FILE))
	rm -rf $(WAYLAND_OBJ) $(WAYLAND_INST)
	PKG_CONFIG_PATH=$(SCANNER_INST)/lib/pkgconfig \
	PKG_CONFIG_PATH_FOR_BUILD=$(SCANNER_INST)/lib/pkgconfig \
	meson setup $(WAYLAND_OBJ) $(WAYLAND_SRC) $(MESON_CROSS) \
		--prefix=$(WAYLAND_INST) --libdir=lib --buildtype=release \
		-Ddefault_library=static -Dlibraries=true -Dscanner=false \
		-Dtests=false -Ddocumentation=false -Ddtd_validation=false \
		$(call quiet_log,$(WAYLAND_OBJ)-setup.log)
	ninja -C $(WAYLAND_OBJ) install $(call quiet_log,$(WAYLAND_OBJ)-ninja.log)
	test -f $@

$(SCANNER): $(WAYLAND_STAMP)
	rm -rf $(SCANNER_OBJ) $(SCANNER_INST)
	env -u PKG_CONFIG_LIBDIR -u PKG_CONFIG_PATH CC=$(BUILD_CC) \
	meson setup $(SCANNER_OBJ) $(WAYLAND_SRC) \
		--prefix=$(SCANNER_INST) --libdir=lib --buildtype=release \
		-Dlibraries=false -Dscanner=true \
		-Dtests=false -Ddocumentation=false -Ddtd_validation=false \
		$(call quiet_log,$(SCANNER_OBJ)-setup.log)
	ninja -C $(SCANNER_OBJ) install $(call quiet_log,$(SCANNER_OBJ)-ninja.log)
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
		"$(MESON_PKGCONFIG_KEY) = 'pkg-config'" \
		"[properties]" \
		"pkg_config_libdir = '$(PKG_CONFIG_LIBDIR)'" \
		"[host_machine]" \
		"system = 'linux'" \
		"cpu_family = '$(MESON_CPU)'" \
		"cpu = '$(MESON_CPU)'" \
		"endian = 'little'" > $@
