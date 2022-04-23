#include <string.h>
#include "engine.h"

void
light_init(struct light *c)
{
	c->pos = (vec3){0.1,2.,4.172231};
	light_look_at(c, (vec3){2.25,0.,-0.891439}, (vec3){0.,1.,0.});
	c->rot = QUATERNION_IDENTITY;
	c->col = (vec4){1,1,1,0.1};
	c->dir = light_get_dir(c);;
}

void
light_set(struct light *c, vec3 pos, quaternion rot)
{
	c->pos = pos;
	c->rot = rot;
	c->dir = light_get_dir(c);;
}

void
light_set_pos(struct light *c, vec3 pos)
{
	light_set(c, pos, c->rot);
	c->dir = light_get_dir(c);;
}


void
light_set_rot(struct light *c, quaternion rot)
{
	light_set(c, c->pos, rot);
	c->dir = light_get_dir(c);;
}

vec3
light_get_dir(struct light *c)
{
	vec3 f;
	mat3 rot;
	quaternion_to_rot3(&rot, c->rot);

	f.x = rot.m[0][2];
	f.y = rot.m[1][2];
	f.z = rot.m[2][2];

	return f;
}

void
light_move(struct light *c, vec3 off)
{
	light_set_pos(c, vec3_add(c->pos, off));
	c->dir = light_get_dir(c);;
}

void
light_look_at(struct light *c, vec3 dir, vec3 up)
{

	vec3 v = vec3_normalize(vec3_sub(dir, c->pos));
	c->rot = quaternion_look_at(v, up);
	c->dir = light_get_dir(c);;
}
