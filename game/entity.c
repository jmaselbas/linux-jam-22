#include "game.h"
#include "asset.h"
#include "render.h"
#include "sound.h"
#include "entity.h"

static void
render_entity(struct game_state *game_state, size_t count, entity_id *id)
{
	struct system *sys_render = &game_state->sys_render;
	struct texture *depth = &game_state->depth;
	size_t i;

	for (i = 0; i < count; i++) {
		struct entity *e = &game_state->entity[id[i]];

		sys_render_push(sys_render, &(struct render_entry){
				.shader = e->shader,
				.mesh = e->mesh,
				.scale = {1, 1, 1},
				.position = e->position,
				.rotation = QUATERNION_IDENTITY,
				.color = e->color,
				.texture = {
					{.name = "t_line", .res_id = TEXTURE_LINE },
					{.name = "depth", .res_id = INTERNAL_TEXTURE, .tex = depth }
				},
				.cull = 1,
			});
	}
}

static void
sound_entity(struct game_state *game_state, size_t count, entity_id *id)
{
	struct system *sys_sound = &game_state->sys_sound;
	struct wav *wav;
	size_t i;

	for (i = 0; i < count; i++) {
		struct entity *e = &game_state->entity[id[i]];
		wav = game_get_wav(game_state->game_asset, e->wav);

		if (e->sound_state == RESET) {
			e->sound_state = CONTINUE;
			sampler_init(&e->sampler, wav);
		}

		sys_sound_push(sys_sound, &(struct sound_entry){
				.sampler = &e->sampler,
				.wav = wav,
				.entity_pos = e->position,
			});
	}
}

static int
walkable_at(struct game_state *game_state, vec2 p)
{
	struct lvl *lvl = game_state->current_lvl;
	int x = p.x;
	int y = p.y;

	if (p.x < 0 || p.y < 0)
		return 0;
	if (x > ARRAY_LEN(lvl->map) || y > ARRAY_LEN(lvl->map[0]))
		return 0;

	return lvl->map[x][y] == 0;
}

static vec3
walk_entity(struct game_state *game_state, vec3 cur, vec3 pos)
{
	struct system *dbg_render = &game_state->dbg_render;
	const int N = 7;
	const float S = 0.2;
	int min_x, max_x;
	int min_y, max_y;
	int x, y;
	vec3 p = {0};
	vec3 s  = {0.01,0.01,0.01};
	vec3 s2 = {0.2,0.2,0.2};
	float m = 10000, d;

	sys_render_push_vec(dbg_render, pos, vec3_normalize(vec3_sub(pos, cur)), (vec3){0,1,0.5});
	sys_render_push_cross(dbg_render, cur, s2, (vec3){0,0.5,0.5});
	sys_render_push_cross(dbg_render, pos, s2, (vec3){0.5,0,0.5});

	vec2 grid[N][N];
	for (x = 0; x < N; x++) {
		for (y = 0; y < N; y++) {
			grid[x][y].x = ((int)cur.x) + (x-1) * S;
			grid[x][y].y = ((int)cur.z) + (y-1) * S;
		}
	}
	char walk[N][N];
	for (x = 0; x < N; x++) {
		for (y = 0; y < N; y++) {
			walk[x][y] = walkable_at(game_state, grid[x][y]);
		}
	}
	for (x = 0; x < N; x++) {
		if (!walk[x][0])
			walk[x][1] = 0;
		if (!walk[x][N-1])
			walk[x][N-2] = 0;
	}
	for (y = 0; y < N; y++) {
		if (!walk[0][y])
			walk[1][y] = 0;
		if (!walk[N-1][y])
			walk[N-2][y] = 0;
	}

	for (x = 0; x < N; x++) {
		for (y = 0; y < N; y++) {
			float px = grid[x][y].x;
			float py = grid[x][y].y;
			vec3 pp = {grid[x][y].x, 0.01, grid[x][y].y};
			if (walk[x][y])
				sys_render_push_cross(dbg_render, pp, s, C_GREEN);
			else
				sys_render_push_cross(dbg_render, pp, s, C_RED);

			if (!walk[x][y])
				continue;
			if (pos.x < px)
				p.x = px;
			else if (pos.x > (px + S))
				p.x = px + S;
			else
				p.x = pos.x;
			if (pos.z < py)
				p.z = py;
			else if (pos.z > (py + S))
				p.z = py + S;
			else
				p.z = pos.z;

			sys_render_push_cross(dbg_render, (vec3){x,0,y}, s, C_CYAN);
			sys_render_push_cross(dbg_render, p, s, C_CYAN);
			d = vec3_dist(pos, p);
			if (d < m) {
				m = d;
				cur = p;
			}
		}
	}

	return cur;
}

static void
move_entity(struct game_state *game_state, size_t count, entity_id *id)
{
	struct system *dbg_render = &game_state->dbg_render;
	float dt = game_state->dt;
	size_t i;

	for (i = 0; i < count; i++) {
		struct entity *e = &game_state->entity[id[i]];
		vec3 pos = e->position;
		vec3 dir = e->direction;
		float speed = e->move_speed * dt;

		pos = walk_entity(game_state, pos, vec3_add(pos, vec3_mult(speed, dir)));
		e->position = pos;
	}
}

static void
debug_entity(struct game_state *game_state, struct entity *e)
{
	struct system *dbg_render = &game_state->dbg_render;

	if (e->components.has_debug) {
		sys_render_push(dbg_render, &(struct render_entry){
				.shader = SHADER_SOLID,
				.mesh = e->debug_mesh,
				.scale = e->debug_scale,
				.position = e->position,
				.rotation = QUATERNION_IDENTITY,
				.color = { 0, 0.8, 0.2},
			});
	}

	sys_render_push_cross(dbg_render, e->position, (vec3){0.2,0.2,0.2}, (vec3){0,0.5,0.5});
}

void
entity_step(struct game_state *game_state)
{
	entity_id move_ids[MAX_ENTITY_COUNT];
	entity_id render_ids[MAX_ENTITY_COUNT];
	entity_id sound_ids[MAX_ENTITY_COUNT];
	size_t move_count = 0;
	size_t render_count = 0;
	size_t sound_count = 0;
	entity_id id;

	for (id = 0; id < game_state->entity_count; id++) {
		struct entity *e = &game_state->entity[id];

		if (e->components.has_movement)
			move_ids[move_count++] = id;

		if (e->components.has_render)
			render_ids[render_count++] = id;

		if (e->components.has_sound)
			sound_ids[sound_count++] = id;
	}

	move_entity(game_state, move_count, move_ids);
	render_entity(game_state, render_count, render_ids);
	sound_entity(game_state, sound_count, sound_ids);

	if (game_state->debug) {
		for (id = 0; id < game_state->entity_count; id++) {
			debug_entity(game_state, &game_state->entity[id]);
		}
	}
}
