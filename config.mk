# game version
VERSION = ldj-22.0

# Customize below to fit your system
CONFIG_JACK=n
CONFIG_PULSE=n
CONFIG_MINIAUDIO=n
CONFIG_SDL_AUDIO=y

# Install paths
PREFIX := /usr/local
MANPREFIX := $(PREFIX)/share/man

# Target specific configuration
# Grab a sane default value for native target using the selected toolchain.
TARGET?=$(shell $(CC) -dumpmachine)
-include target-$(TARGET).mk

ifneq ($(CROSS_COMPILE),)
CC      = $(CROSS_COMPILE)cc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
READELF = $(CROSS_COMPILE)readelf
OBJSIZE = $(CROSS_COMPILE)size
STRIP   = $(CROSS_COMPILE)strip
endif

PKG_CONFIG_PATH ?= /usr/lib/pkgconfig/
PKG ?= PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config

# Depencies includes and libs
INCS ?= $(shell $(PKG) --cflags sdl2)
LIBS ?= $(shell $(PKG) --libs sdl2)
LIBS += -lm

# Config specific flags
CFLAGS-$(CONFIG_JACK) += -DCONFIG_JACK
CFLAGS-$(CONFIG_PULSE) += -DCONFIG_PULSE
CFLAGS-$(CONFIG_MINIAUDIO) += -DCONFIG_MINIAUDIO
CFLAGS-$(CONFIG_SDL_AUDIO) += -DCONFIG_SDL_AUDIO
LIBS-$(CONFIG_JACK) += -lpthread -ljack
LIBS-$(CONFIG_PULSE) += -lpthread -lpulse
LIBS-$(CONFIG_MINIAUDIO) += -lpthread

LIBS += $(LIBS-y)
CFLAGS += $(CFLAGS-y)

# Flags
CFLAGS += -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200809L
CFLAGS += -O2 -W -fPIC -Wall -Wextra -g
ifneq ($(RELEASE),)
CFLAGS += -s -ffunction-sections
endif
CFLAGS += -DVERSION=\"$(VERSION)\"
CFLAGS += -I. $(INCS)
LDFLAGS += $(LIBS)
