#pragma once

#include <time.h>

#include "core/engine.h"

typedef int64_t (file_size_t)(const char *path);
typedef int64_t (file_read_t)(const char *path, void *buf, size_t size);
typedef time_t (file_time_t)(const char *path);

struct file_io {
	file_size_t *size;
	file_read_t *read;
	file_time_t *time;
};

typedef void (window_close_t)(void);
typedef void (window_cursor_t)(int show);
struct window_io {
	window_close_t *close;  /* request window to be closed */
	window_cursor_t *cursor; /* request cursor to be shown */
};

struct game_memory {
	struct memory_zone state;
	struct memory_zone asset;
	struct memory_zone scrap;
	struct memory_zone audio;
};

/* typedef for function type */
typedef void (game_init_t)(struct game_memory *memory, struct file_io *file_io, struct window_io *win_io);
typedef void (game_step_t)(struct game_memory *memory, struct input *input, struct audio *audio);
typedef void (game_fini_t)(struct game_memory *memory);

/* declare functions signature */
game_init_t game_init;
game_step_t game_step;
game_fini_t game_fini;

struct system {
	struct game_state *game_state;
	struct game_asset *game_asset;
	struct memory_zone zone;
	struct list list;
};

void sys_init(struct system *sys, struct game_state *game_state, struct game_asset *game_asset, struct memory_zone zone);

#include "asset.h"
#include "entity.h"

struct listener {
	vec3 pos;
	vec3 dir;
	vec3 left;
};

struct lvl {
	char map[16][16];
	vec3 start;
};

struct game_state {
	struct game_asset *game_asset;
	struct window_io *window_io;
	struct input *input;
	struct input last_input;
	float dt;
	int width;
	int height;

	enum {
		GAME_INIT,
		GAME_MENU,
		GAME_PLAY,
		GAME_PAUSE,
	} state, new_state;

	struct lvl *current_lvl;
	struct camera cam;
	struct camera sun;
	int flycam;
	int flycam_forward, flycam_left;
	float flycam_speed;

	struct listener cur_listener;
	struct listener nxt_listener;

	struct light light;
	int debug;
	int key_debug;

	struct texture depth;

	struct font ascii_font;

	float fps;
	struct system sys_render;
	struct system dbg_render;
	struct system sys_text;
	struct system sys_sound;

#define MAX_ENTITY_COUNT 512
	size_t entity_count;
	struct entity entity[MAX_ENTITY_COUNT];
};

