# libwayland-client as a static library, built from the pinned release
# with meson (distributions ship only the shared library). Run by the
# Makefile. Needs meson, ninja, wayland-scanner (the build uses the
# host's) and libffi's headers and static library -- for a cross build
# the target's (libffi-dev:<arch>), through a generated meson cross
# file (arch.mk). The source is shared, the build is per target.

include arch.mk

WAYLAND_SRC   := $(BUILD)/wayland
WAYLAND_STAMP := $(WAYLAND_SRC)/.fetched-$(WAYLAND_VERSION)
WAYLAND_OBJ   := $(OUT)/wayland-build
WAYLAND_INST  := $(abspath $(OUT)/wayland-inst)
CROSS_FILE    := $(OUT)/meson-cross.ini

ifneq ($(CROSS),)
MESON_CROSS := --cross-file $(CROSS_FILE)
endif

$(WAYLAND_INST)/lib/libwayland-client.a: $(WAYLAND_STAMP) $(if $(CROSS),$(CROSS_FILE))
	rm -rf $(WAYLAND_OBJ) $(WAYLAND_INST)
	meson setup $(WAYLAND_OBJ) $(WAYLAND_SRC) $(MESON_CROSS) \
		--prefix=$(WAYLAND_INST) --libdir=lib --buildtype=release \
		-Ddefault_library=static -Dlibraries=true -Dscanner=false \
		-Dtests=false -Ddocumentation=false -Ddtd_validation=false >/dev/null
	ninja -C $(WAYLAND_OBJ) install >/dev/null
	test -f $@

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
