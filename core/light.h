#pragma once

struct light {
	vec3 pos;
	quaternion rot;
	vec3 dir;
	vec4 col;
};

void light_init(struct light *c);

void light_set(struct light *c, vec3 pos, quaternion rotation);
void light_set_pos(struct light* c, vec3 pos);
void light_set_rot(struct light *c, quaternion q);
vec3 light_get_dir(struct light *c);
void light_move(struct light *c, vec3 off);
void light_look_at(struct light *c, vec3 look_at, vec3 up);

