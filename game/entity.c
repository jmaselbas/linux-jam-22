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
map_at(struct game_state *game_state, vec2 p)
{
	struct lvl *lvl = game_state->current_lvl;
	int x = p.x;
	int y = p.y;

	if (p.x < 0 || p.y < 0)
		return 0;
	if (x > ARRAY_LEN(lvl->map) || y > ARRAY_LEN(lvl->map[0]))
		return 0;

	return lvl->map[x][y];
}

static int
walkable_at(struct game_state *game_state, vec2 p, size_t b_count, entity_id *b_ids)
{
	struct lvl *lvl = game_state->current_lvl;
	size_t i, id;
	int x = p.x;
	int y = p.y;

	if (p.x < 0 || p.y < 0)
		return 0;
	if (x > ARRAY_LEN(lvl->map) || y > ARRAY_LEN(lvl->map[0]))
		return 0;

	if (lvl->map[x][y] == 1)
		return 0;

	for (i = 0; i < b_count; i++) {
		struct entity *e = &game_state->entity[b_ids[i]];
		vec2 bp = {(int)e->position.x, (int)e->position.z};
		if (bp.x <= p.x && p.x <= (bp.x + 1.01) &&
		    bp.y <= p.y && p.y <= (bp.y + 1.01))
		    return 0;
	}

	return 1;
}

static vec3
walk_entity(struct game_state *game_state, vec3 cur, vec3 pos, size_t b_count, entity_id *b_ids)
{
	struct system *dbg_render = &game_state->dbg_render;
	const int N = 7;
	const float S = 0.2;
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
			walk[x][y] = walkable_at(game_state, grid[x][y], b_count, b_ids);
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
move_entity(struct game_state *game_state, size_t count, entity_id *id,
	size_t b_count, entity_id *b_ids)
{
	float dt = game_state->dt;
	size_t i;

	for (i = 0; i < count; i++) {
		struct entity *e = &game_state->entity[id[i]];
		vec3 pos = e->position;
		vec3 dir = e->direction;
		float speed = e->move_speed * dt;

		pos = walk_entity(game_state, pos, vec3_add(pos, vec3_mult(speed, dir)), b_count, b_ids);
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

int box_at_target_count;

static void
game_action_box(struct game_state *game_state, struct entity *e)
{
	struct system *dbg_render = &game_state->dbg_render;
	struct system *sys_render = &game_state->sys_render;
	struct system *sys_text = &game_state->sys_text;
	struct input *input = game_state->input;
	struct lvl *lvl = game_state->current_lvl;
	vec3 forw = camera_get_dir(&game_state->cam);
	vec3 player;
	int x = e->position.x;
	int y = e->position.z;
	vec3 pos = {x + 0.5, 0, y + 0.5};
	float act_dist = 0.6;
	float ang_x, ang_y;
	float dst_n, dst_s, dst_e, dst_w;
	vec3 pos_n, pos_s, pos_e, pos_w;
	int can_n, can_s, can_e, can_w;
	vec3 dbg_c = C_RED;

	if (map_at(game_state, (vec2){x, y}) & 2) {
		e->color = C_CYAN;
		box_at_target_count++;
	} else {
		e->color = C_RED;
	}
	player = game_state->entity[0].position;
	e->box_state.hilight = 0;
	if (e->box_state.moving) {
		float dt = game_state->dt;
		vec3 pos = e->position;
		vec3 dir = e->direction;
		float speed = e->move_speed * dt;
		vec3 new = vec3_add(pos, vec3_mult(speed, dir));
		e->move_dist -= vec3_dist(pos, new);
		if (e->move_dist <= 0) {
			new = vec3_add(new, vec3_mult(e->move_dist, dir));
			e->box_state.moving = 0;
		}
		e->position = new;
		return;
	}

	if (vec3_dist(player, pos) > 2)
		return; /* player too far */

	pos_n = (vec3){pos.x - 0.5, pos.y, pos.z};
	pos_e = (vec3){pos.x      , pos.y, pos.z - 0.5};
	pos_s = (vec3){pos.x + 0.5, pos.y, pos.z};
	pos_w = (vec3){pos.x      , pos.y, pos.z + 0.5};

	dst_n = vec3_dist(player, pos_n);
	dst_e = vec3_dist(player, pos_e);
	dst_s = vec3_dist(player, pos_s);
	dst_w = vec3_dist(player, pos_w);

	ang_x = vec3_dot(forw, VEC3_AXIS_X);
	ang_y = vec3_dot(forw, VEC3_AXIS_Z);

	can_n = ang_x > +0.5 && dst_n < act_dist && walkable_at(game_state, (vec2){x + 1, y}, 0, 0);
	can_e = ang_y > +0.5 && dst_e < act_dist && walkable_at(game_state, (vec2){x,     y + 1}, 0,0);
	can_s = ang_x < -0.5 && dst_s < act_dist && walkable_at(game_state, (vec2){x - 1, y}, 0,0);
	can_w = ang_y < -0.5 && dst_w < act_dist && walkable_at(game_state, (vec2){x,     y - 1}, 0,0);

	if (can_n || can_e || can_s || can_w) {
		sys_render_push(sys_render, &(struct render_entry){
				.shader = SHADER_HILIGHT,
				.mesh = DEBUG_MESH_CUBE,
				.scale = {0.5, 0.5, 0.5},
				.position = vec3_add(e->position, (vec3){0,0.5,0}),
				.rotation = QUATERNION_IDENTITY,
				.color = C_GREEN,
				.cull = 1,
			});
		if (!e->box_state.moving && key_pressed(input, KEY_SPACE)) {
			e->box_state.moving = 1;
			e->move_dist = 1.0;
			if (can_n)
				e->direction = VEC3_AXIS_X;
			else if (can_e)
				e->direction = VEC3_AXIS_Z;
			else if (can_s)
				e->direction = (vec3){-1.0, 0.0, 0.0};
			else if (can_w)
				e->direction = (vec3){0.0, 0.0, -1.0};
		}
//		sys_text_printf(sys_text, (vec2){0,0}, (vec3){0.8,0.8,0.0}, "PUSH");
	}

	sys_render_push(dbg_render, &(struct render_entry){
			.shader = SHADER_SOLID,
			.mesh = DEBUG_MESH_CUBE,
			.scale = {0.1, 0.1, 0.1},
			.position = pos_n,
			.rotation = QUATERNION_IDENTITY,
			.color = (can_n) ? C_GREEN : C_RED,
			.cull = 1,
		});
	sys_render_push(dbg_render, &(struct render_entry){
			.shader = SHADER_SOLID,
			.mesh = DEBUG_MESH_CUBE,
			.scale = {0.1, 0.1, 0.1},
			.position = pos_e,
			.rotation = QUATERNION_IDENTITY,
			.color = (can_e) ? C_GREEN : C_RED,
			.cull = 1,
		});
	sys_render_push(dbg_render, &(struct render_entry){
			.shader = SHADER_SOLID,
			.mesh = DEBUG_MESH_CUBE,
			.scale = {0.1, 0.1, 0.1},
			.position = pos_s,
			.rotation = QUATERNION_IDENTITY,
			.color = (can_s) ? C_GREEN : C_RED,
			.cull = 1,
		});
	sys_render_push(dbg_render, &(struct render_entry){
			.shader = SHADER_SOLID,
			.mesh = DEBUG_MESH_CUBE,
			.scale = {0.1, 0.1, 0.1},
			.position = pos_w,
			.rotation = QUATERNION_IDENTITY,
			.color = (can_w) ? C_GREEN : C_RED,
			.cull = 1,
		});
}

void
entity_step(struct game_state *game_state)
{
	entity_id move_ids[MAX_ENTITY_COUNT];
	entity_id render_ids[MAX_ENTITY_COUNT];
	entity_id sound_ids[MAX_ENTITY_COUNT];
	entity_id box_ids[MAX_ENTITY_COUNT];
	size_t move_count = 0;
	size_t render_count = 0;
	size_t sound_count = 0;
	size_t box_count = 0;
	entity_id id;
	size_t i;

	for (id = 0; id < game_state->entity_count; id++) {
		struct entity *e = &game_state->entity[id];

		if (e->components.has_movement)
			move_ids[move_count++] = id;

		if (e->components.has_render)
			render_ids[render_count++] = id;

		if (e->components.has_sound)
			sound_ids[sound_count++] = id;

		if (e->components.is_box)
			box_ids[box_count++] = id;
	}

	move_entity(game_state, move_count, move_ids, box_count, box_ids);
	render_entity(game_state, render_count, render_ids);
	sound_entity(game_state, sound_count, sound_ids);

	box_at_target_count = 0;
	for (i = 0; i < box_count; i++) {
		id = box_ids[i];
		game_action_box(game_state, &game_state->entity[id]);
	}
	if (box_at_target_count == game_state->current_lvl->count) {
		game_state->win = 1;
	}
	if (game_state->debug) {
		for (id = 0; id < game_state->entity_count; id++) {
			debug_entity(game_state, &game_state->entity[id]);
		}
	}
}
