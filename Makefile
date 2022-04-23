# See LICENSE file for copyright and license details.
.POSIX:

# Makefile usage: make [option] [target]
# This makefile accept some specific options:
# O=<build-dir>        out-of-tree build dir, compilation objects will be there. Default: $PWD/bin
# TARGET=<target>      targeted build system, available: linux-x86_64 linux-x86 w64 wasm
# DESTDIR=<dir>        destination dir, "fake" root directory for distributing an installation
# CROSS_COMPILE=<name> name prefix for an external toolchain, ex: arm-none-eabi-

include config.mk

include core/Makefile
include game/Makefile
include plat/Makefile
-include syn/Makefile_game
-include tests/Makefile

O?=bin

ifneq ($(O),)
$(shell mkdir -p $(O))
OUT = $(basename $(O))/
endif

obj = $(addprefix $(OUT),$(src:.c=.o))
plt-src += main.c
plt-obj = $(addprefix $(OUT),$(plt-src:.c=.o))
plt-dynlib-y-obj = $(addprefix $(OUT),$(plt-dynlib-y-src:.c=.o))
plt-dynlib-n-obj = $(addprefix $(OUT),$(plt-dynlib-n-src:.c=.o))
test-src += test.c
test-obj = $(test-src:.c=.o)
TESTBIN = test
BIN = survivre$(EXT)
LIB = $(LIBDIR)/libgame.so
RES += res/audio/casey.ogg \
 res/audio/fx_bip_01.wav \
 res/audio/fx_crash_01.wav \
 res/audio/fx_crash_02.wav \
 res/audio/fx_crash_03.wav \
 res/audio/fx_crash_04.wav \
 res/audio/fx_wind_loop.ogg \
 res/audio/fx_woosh_01.wav \
 res/audio/fx_woosh_02.wav \
 res/audio/fx_woosh_03.wav \
 res/audio/fx_woosh_04.wav \
 res/audio/LD48_loop_fade.ogg \
 res/cap.obj \
 res/menu_quit.obj \
 res/menu_start.obj \
 res/player.obj \
 res/rock.obj \
 res/room.obj \
 res/screen.obj \
 res/wall.obj \
 res/orth.vert \
 res/proj.vert \
 res/solid.frag \
 res/screen.frag \
 res/wall.frag

# dynlib is the default target for now, not meant for release
all: dynlib static

static: $(OUT)$(BIN);

tests: $(TESTBIN)

# dynlib build enable game code hot reloading
dynlib: LDFLAGS += -ldl -rdynamic -Wl,-rpath,.
dynlib: CFLAGS += -DCONFIG_LIBDIR=\"$(LIBDIR)/\"
dynlib: $(OUT)$(LIB) $(OUT)run;

$(OUT)$(LIB): $(obj)
	@mkdir -p $(dir $@)
	$(CC) -shared $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OUT)run: $(obj) $(plt-obj) $(plt-dynlib-y-obj)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LDLIBS)

$(OUT)$(BIN): $(obj) $(plt-obj) $(plt-dynlib-n-obj)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LDLIBS)

$(TESTBIN): $(test-obj)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LDLIBS)

$(OUT)%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS)
	@$(CC) -MP -MM $< -MT $@ -MF $(call namesubst,%,.%.mk,$@) $(CFLAGS)

install: $(OUT)$(BIN) $(RES)
	@mkdir -p $(DESTDIR)
	install $< $(DESTDIR)
# tar is used here to copy files while preserving the original path of each file
	tar cf - $(RES) | tar xf - -C $(DESTDIR)

clean:
	rm -f $(BIN) $(TESTBIN) main.o $(obj) $(dep) $(plt-obj) $(test-obj)
	@rm -f $(shell find . -name ".*.mk")

echo:
	@echo out: $(OUT),bin: $(BIN) ,lib: $(LIB)

.PHONY: all static dynlib clean echo

include dist.mk

# namesubst perform a patsubst only on the file name, while keeping the path intact
# usage: $(call namesubst,pattern,replacement,text)
# example: $(call namesubst,%.c,.%.d,foo/aa.c bar/bb.c)
# produce: foo/.aa.d bar/.bb.d
define namesubst
	$(foreach i,$3,$(subst $(notdir $i),$(patsubst $1,$2,$(notdir $i)), $i))
endef

-include $(shell find . -name ".*.mk")
