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

	sys_render_push(sys_render, &(struct render_entry){
			.shader = SHADER_SKY,
			.mesh = MESH_QUAD,
			.scale = {1, 1, 1},
			.position = {0, 0, -1},
			.rotation = QUATERNION_IDENTITY,
			.cull = 0,
		});

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
					{.name = "tex", .res_id = TEXTURE_CONCRETE },
					{.name = "nmap", .res_id = TEXTURE_CONCRETE_NORMAL },
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

static void
move_entity(struct game_state *game_state, size_t count, entity_id *id)
{
	float dt = game_state->dt;
	size_t i;

	for (i = 0; i < count; i++) {
		struct entity *e = &game_state->entity[id[i]];
		vec3 pos = e->position;
		vec3 dir = e->direction;
		float speed = e->move_speed * dt;

		e->position = vec3_add(pos, vec3_mult(speed, dir));
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
