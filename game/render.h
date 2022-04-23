#pragma once

#include "asset.h"

void render_bind_shader(struct shader *shader);
void render_bind_mesh(struct shader *shader, struct mesh *mesh);
void render_bind_camera(struct shader *s, struct camera *c);
void render_mesh(struct mesh *mesh);

struct render_input {
	const char *name;
	enum asset_key res_id;
	struct texture *tex;
};

#define RENDER_MAX_TEXTURE_UNIT 8
struct render_entry {
	enum asset_key shader;
	enum asset_key mesh;
	struct render_input texture[RENDER_MAX_TEXTURE_UNIT];
	quaternion rotation;
	vec3 position;
	vec3 scale;
	vec3 color;
	int count;
	vec4 *inst;
	int cull:1;
};

void sys_render_init(struct system *sys,
		   struct game_state *game_state,
		   struct game_asset *game_asset,
		   struct memory_zone zone);
void sys_render_push(struct system *, struct render_entry *);
void sys_render_exec(struct system *, struct camera, int do_frustum_cull);

void sys_render_push_cross(struct system *sys, vec3 at, vec3 scale, vec3 color);
void sys_render_push_vec(struct system *sys, vec3 pos, vec3 dir, vec3 color);
void sys_render_push_bounding(struct system *sys, vec3 pos, vec3 scale, struct bounding_volume *bvol);

