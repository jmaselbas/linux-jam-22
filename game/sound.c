#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "game.h"
#include "asset.h"
#include "render.h"
#include "sound.h"

struct lrcv{
	float l;
	float r;
	float c;
	float v;
};

void
sys_sound_push(struct system *sys, struct sound_entry *entry)
{
	struct sound_entry *e;

	e = list_push(&sys->list, &sys->zone, sizeof(*e));
	*e = *entry;
}

static struct listener
listener_lerp(struct listener a, struct listener b, float x)
{
	struct listener r;

	r.pos = vec3_lerp(a.pos, b.pos, x);
	r.dir = vec3_lerp(a.dir, b.dir, x);
	r.left = vec3_lerp(a.left, b.left, x);

	return r;
}

static struct lrcv
sys_sound_spatializer(struct sound_entry *e, struct listener *li)
{
	vec3 entity_pos = e->entity_pos;
	vec3 player_pos = li->pos;
	vec3 player_dir = li->dir;
	vec3 player_left = li->left;
	vec3 v = vec3_normalize(vec3_sub(entity_pos, player_pos));
	float sin = vec3_dot(player_left, v);
	float cos = vec3_dot(player_dir, v);
	float l = MAX(0, sin);
	float r = ABS(MIN(0, sin));
	float c = ABS(cos);

	float d = vec3_norm(vec3_sub(player_pos, entity_pos));
	float vol = 1 / (d);
	return (struct lrcv) {
		.l = l,
		.r = r,
		.c = c,
		.v = vol
	};
}

void
sys_sound_set_listener(struct system *sys, vec3 pos, vec3 dir, vec3 left)
{
	struct game_state *game_state = sys->game_state;

	game_state->nxt_listener.pos = pos;
	game_state->nxt_listener.dir = dir;
	game_state->nxt_listener.left = left;
}

void
sys_sound_exec(struct system *sys, struct audio *audio)
{
	struct game_state *game_state = sys->game_state;
	struct listener cur = game_state->cur_listener;
	struct listener nxt = game_state->nxt_listener;
	struct link *link;
	size_t i, j;
	float l, r;

	for (i = 0; i < audio->size; i++) {
		audio->buffer[i].l = 0;
		audio->buffer[i].r = 0;
	}

	/* Note: listener interpolation seems to be working
	 * well with buffers of 1024 frames
	 */
	for (link = sys->list.first; link != NULL; link = link->next) {
		struct sound_entry e = *(struct sound_entry *)link->data;
		for (j = 0; j < audio->size; j++) {
			const float f = j / (float)audio->size;
			struct listener li = listener_lerp(cur, nxt, f);
			struct lrcv lrcv = sys_sound_spatializer(&e, &li);

			e.sampler->vol = lrcv.v;
			l = step_sampler(e.sampler);
			if (e.sampler->wav->header.channels == 1) {
				r = l;
			} else {
				r = step_sampler(e.sampler);
			}
			audio->buffer[j].l += l * lrcv.l + l * lrcv.c;
			audio->buffer[j].r += r * lrcv.r + r * lrcv.c;
		}
	}
	if (audio->size > 0)
		game_state->cur_listener = nxt;
}
