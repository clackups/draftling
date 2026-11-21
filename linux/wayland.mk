# libwayland-client as a static library, built from the pinned release
# with meson (distributions ship only the shared library). Run by the
# Makefile. Needs meson, ninja, wayland-scanner (the build uses the
# host's) and libffi's headers and static library.

WAYLAND_SRC  := $(BUILD)/wayland
WAYLAND_OBJ  := $(BUILD)/wayland-build
WAYLAND_INST := $(abspath $(BUILD)/wayland-inst)

$(WAYLAND_INST)/lib/libwayland-client.a:
	rm -rf $(WAYLAND_SRC) $(WAYLAND_OBJ) $(WAYLAND_INST)
	git -c advice.detachedHead=false clone --quiet --depth 1 \
		--branch $(WAYLAND_VERSION) \
		https://gitlab.freedesktop.org/wayland/wayland.git $(WAYLAND_SRC)
	meson setup $(WAYLAND_OBJ) $(WAYLAND_SRC) \
		--prefix=$(WAYLAND_INST) --libdir=lib --buildtype=release \
		-Ddefault_library=static -Dlibraries=true -Dscanner=false \
		-Dtests=false -Ddocumentation=false -Ddtd_validation=false >/dev/null
	ninja -C $(WAYLAND_OBJ) install >/dev/null
	test -f $@
