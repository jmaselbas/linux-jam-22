
struct sys_part {
	struct system *sys;
};

static void sys_part_init(struct system *sys);

static void sys_part_update(struct game_state *game_state, size_t c, entity_id *id);
static void sys_part_insert(struct game_state *game_state, entity_id id);
static void sys_part_remove(struct game_state *game_state, entity_id id);

void sys_part_init(struct system *sys);


static void
grass_entity(struct game_state *game_state, size_t c, entity_id *id)
{
	struct system *sys_render = &game_state->sys_render;
	struct texture *depth = &game_state->depth;

	if (c == 0)
		return;

	if (p1.count == 0) {
		srand(0x1234);
		p1.count = grass_plant(p1.it_pos, ARRAY_LEN(p1.it_pos), 10, 0.1, 0.75);
		srand(0x1235);
		p2.count = grass_plant(p2.it_pos, ARRAY_LEN(p2.it_pos), 10, 1.0, 1.5);
	}

	struct entity *e = &game_state->entity[id[0]];
	sys_render_push(sys_render, &(struct render_entry){
			.shader = SHADER_GRASS,
			.mesh = MESH_GRASS_TUFF_1,
			.scale = {1, 1, 1},
			.position = e->position,
			.rotation = QUATERNION_IDENTITY,
			.color = {1, 0, 0},
			.texture = {
				{.name = "noise", .res_id = TEXTURE_NOISE },
				{.name = "shadowmap", .res_id = INTERNAL_TEXTURE, .tex = depth }
			},
			.count = p1.count,
			.inst = p1.it_pos,
			.cull = 0,
		});
	sys_render_push(sys_render, &(struct render_entry){
			.shader = SHADER_GRASS,
			.mesh = MESH_GRASS_PATCH_1,
			.scale = {1, 1, 1},
			.position = e->position,
			.rotation = QUATERNION_IDENTITY,
			.color = {1, 0, 0},
			.texture = {
				{.name = "noise", .res_id = TEXTURE_NOISE },
				{.name = "shadowmap", .res_id = INTERNAL_TEXTURE, .tex = depth }
			},
			.count = p2.count,
			.inst = p2.it_pos,
			.cull = 0,
		});
}
