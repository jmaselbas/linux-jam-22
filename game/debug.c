#include "debug.h"
#include "asset.h"
#include "render.h"

void
debug_origin_mark(struct system *sys_render)
{
	vec3 o = {0, 0, 0};
	vec3 r = {1, 0, 0};
	vec3 g = {0, 1, 0};
	vec3 b = {0, 0, 1};
	sys_render_push_cross(sys_render, o, (vec3){1,0,0}, r);
	sys_render_push_cross(sys_render, o, (vec3){0,1,0}, g);
	sys_render_push_cross(sys_render, o, (vec3){0,0,1}, b);
}

void
debug_light_mark(struct light *l, struct system *sys_render)
{
	vec3 r = {1, 0, 0};
	vec3 g = {0, 1, 0};
	sys_render_push_cross(sys_render, l->pos, (vec3){0.1,0.1,0.1}, g);
	sys_render_push_vec(sys_render, l->pos, l->dir, r);
}

void
debug_texture(struct system *dbg, vec2 size, struct texture *tex)
{
	float w = size.x / (float)dbg->game_state->width;
	float h = size.y / (float)dbg->game_state->height;
	vec2 at = {-1, 1}; /* top - left */

	sys_render_push(dbg, &(struct render_entry){
			.shader = DEBUG_SHADER_TEXTURE,
			.mesh = DEBUG_MESH_CUBE,
			.scale = {w, h, 0},
			.position = {at.x + w, at.y - h, -1},
			.rotation = QUATERNION_IDENTITY,
			.texture = {
				{ "tex", INTERNAL_TEXTURE, .tex = tex }
			},
		});
}
