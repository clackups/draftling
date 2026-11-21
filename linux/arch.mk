# Target architecture, included by every makefile here.
#
#   make ARCH=arm64       cross-build for 64-bit Arm (Debian names:
#                         amd64, arm64, armhf, riscv64)
#
# Without ARCH, or with this host's own architecture, the build is
# native and its output goes to build/ as before. A cross build uses
# the <triplet>- toolchain (gcc-<triplet>, g++-<triplet>, binutils),
# the target's libraries through pkg-config's multiarch folder
# (/usr/lib/<triplet>/pkgconfig: the Debian / Ubuntu "<pkg>:<arch>"
# packages) and its own output folder, build/<arch>/. The fetched
# sources in build/ are shared. CROSS_COMPILE=<prefix> overrides the
# toolchain prefix.

HOST_ARCH := $(shell dpkg --print-architecture 2>/dev/null || uname -m)
ARCH ?= $(HOST_ARCH)

triplet_amd64   := x86_64-linux-gnu
triplet_arm64   := aarch64-linux-gnu
triplet_armhf   := arm-linux-gnueabihf
triplet_riscv64 := riscv64-linux-gnu
# meson's cpu_family / cpu for the cross file of libwayland
cpu_amd64   := x86_64
cpu_arm64   := aarch64
cpu_armhf   := arm
cpu_riscv64 := riscv64

ifeq ($(ARCH),$(HOST_ARCH))
CROSS :=
OUT   := build
ifeq ($(origin CC),default)
CC := cc
endif
ifeq ($(origin CXX),default)
CXX := c++
endif
STRIP   ?= strip
OBJDUMP ?= objdump
else
CROSS   := $(ARCH)
TRIPLET := $(triplet_$(ARCH))
ifeq ($(TRIPLET),)
$(error Unknown ARCH '$(ARCH)' (known: amd64 arm64 armhf riscv64))
endif
MESON_CPU := $(cpu_$(ARCH))
CROSS_COMPILE ?= $(TRIPLET)-
CC      := $(CROSS_COMPILE)gcc
CXX     := $(CROSS_COMPILE)g++
AR      := $(CROSS_COMPILE)ar
STRIP   := $(CROSS_COMPILE)strip
OBJDUMP := $(CROSS_COMPILE)objdump
OUT     := build/$(ARCH)
# pkg-config finds the target's .pc files only.
export PKG_CONFIG_LIBDIR := /usr/lib/$(TRIPLET)/pkgconfig:/usr/share/pkgconfig
export PKG_CONFIG_PATH :=
endif

# Large files (64-bit off_t, so readdir() works on file systems with
# 64-bit inode numbers) and 64-bit time_t, as the distributions' 32-bit
# libraries are built; no change on 64-bit targets.
ABI_FLAGS := -D_FILE_OFFSET_BITS=64 -D_TIME_BITS=64

export CC CXX AR
