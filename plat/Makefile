plt-src-y += core.c glad.c audio.c
plt-src-$(CONFIG_JACK)  += jack.c
plt-src-$(CONFIG_PULSE) += pulse.c
plt-src-$(CONFIG_MINIAUDIO) += miniaudio.c miniaudio_imp.c
plt-src-$(CONFIG_SDL_AUDIO) += audio_sdl.c
plt-src += $(patsubst %, plat/%, $(plt-src-y))

plt-dynlib-y = libgame_dynamic.c
plt-dynlib-n = libgame_static.c
plt-dynlib-y-src = $(patsubst %, plat/%, $(plt-dynlib-y))
plt-dynlib-n-src = $(patsubst %, plat/%, $(plt-dynlib-n))
