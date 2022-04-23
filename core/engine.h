#pragma once

#include <stddef.h>
#include <math.h>

#include <plat/glad.h>

#include "util.h"
#include "math.h"
#include "input.h"
#include "mesh.h"
#include "camera.h"
#include "light.h"
#include "ring_buffer.h"
#include "audio.h"
#include "list.h"

struct shader {
	GLuint prog;
	GLuint vert;
	GLuint frag;
	GLuint geom;
};
GLint shader_load(struct shader *s, const char *vert, const char *frag, const char *geom);
GLint shader_reload(struct shader *s, const char *vert, const char *frag, const char *geom);
void shader_free(struct shader *s);

vec4 ray_intersect_mesh(vec3 org, vec3 dir, struct mesh *mesh, mat4 *xfrm);

struct texture {
	GLuint id;
	GLenum type;
	size_t width, height;
};
struct texture create_2d_tex(size_t w, size_t h, GLenum format, GLenum type, void *data);
void delete_tex(struct texture *texture);

struct glyph {
	float adv;
	vec2 off; /* offset to the top left glyph corner */
	vec2 ext; /* extent of the glyph, height and width */
	float al, ab, ar, at; /* altas coordinate */
};

struct font_meta {
	size_t glyph_count;
	struct glyph glyph[128];
};

struct font {
	struct texture *atlas;
	struct font_meta *meta;
};

