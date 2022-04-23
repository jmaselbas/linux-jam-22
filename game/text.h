#pragma once

struct text_entry {
	struct font *font;
	const char *text;
	size_t len;
	vec3 color;
	vec3 position;
	quaternion rotation;
	vec3 scale;
	float fx;
};

void sys_text_init(struct system *sys,
		   struct game_state *game_state,
		   struct game_asset *game_asset,
		   struct memory_zone zone);
void sys_text_push(struct system *, struct text_entry *);
void sys_text_exec(struct system *);

void sys_text_puts(struct system *sys, vec2 at, vec3 color, char *s);
void sys_text_printf(struct system *sys, vec2 at, vec3 color, char *fmt, ...);

