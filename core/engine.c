#include <stdio.h>
#include <string.h>

#include "engine.h"

static GLint
shader_compile(GLsizei count, const GLchar **string, const GLint *length, GLenum type, GLuint *out)
{
	char logbuf[1024];
	GLsizei logsize;
	GLuint shader = glCreateShader(type);
	GLint ret;

	glShaderSource(shader, count, string, length);
        glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &ret);
	if (ret != GL_TRUE) {
		glGetShaderInfoLog(shader, sizeof(logbuf), &logsize, logbuf);
		glDeleteShader(shader);
		fprintf(stderr, "--- ERROR ---\n%s", logbuf);
		return ret;
	}

	*out = shader;
	return GL_TRUE;
}

GLint
shader_reload(struct shader *s, const char *vert_src, const char *frag_src, const char *geom_src)
{
	char logbuf[1024];
	GLsizei logsize;
	GLuint prog;
	GLuint vert = s->vert;
	GLuint frag = s->frag;
	GLuint geom = s->geom;
	GLint vert_len;
	GLint frag_len;
	GLint ret;

	if (vert_src) {
		vert_len = strlen(vert_src);
		ret = shader_compile(1, &vert_src, &vert_len, GL_VERTEX_SHADER, &vert);
		if (ret != GL_TRUE)
			goto err_vert;
	}

	if (frag_src) {
		frag_len = strlen(frag_src);
		ret = shader_compile(1, &frag_src, &frag_len, GL_FRAGMENT_SHADER, &frag);
		if (ret != GL_TRUE)
			goto err_frag;
	}

#ifdef GL_GEOMETRY_SHADER
	GLint geom_len;
	if (geom_src) {
		geom_len = strlen(geom_src);
		ret = shader_compile(1, &geom_src, &geom_len, GL_GEOMETRY_SHADER, &geom);
		if (ret != GL_TRUE)
			goto err_geom;
	}
#endif

	/* Create a new program */
	prog = glCreateProgram();
	if (!prog)
		goto err_prog;
	if (vert)
		glAttachShader(prog, vert);
	if (frag)
		glAttachShader(prog, frag);
	if (geom)
		glAttachShader(prog, geom);

	glLinkProgram(prog);
	glGetProgramiv(prog, GL_LINK_STATUS, &ret);
	if (ret != GL_TRUE) {
		glGetProgramInfoLog(prog, sizeof(logbuf), &logsize, logbuf);
		fprintf(stderr, "--- ERROR ---\n%s", logbuf);
		goto err_link;
	}

	if (s->prog) {
		glDetachShader(s->prog, s->vert);
		glDetachShader(s->prog, s->frag);
		glDetachShader(s->prog, s->geom);
		glDeleteProgram(s->prog);
		if (s->vert != vert)
			glDeleteShader(s->vert);
		if (s->vert != frag)
			glDeleteShader(s->frag);
		if (s->geom != geom)
			glDeleteShader(s->geom);
	}
	s->prog = prog;
	s->vert = vert;
	s->frag = frag;
	s->frag = geom;
	return 0;

err_link:
	if (vert)
		glDetachShader(prog, vert);
	if (frag)
		glDetachShader(prog, frag);
	if (geom)
		glDetachShader(prog, geom);
	glDeleteProgram(prog);
err_prog:
	if (geom_src)
		glDeleteShader(geom);
#ifdef GL_GEOMETRY_SHADER
err_geom:
	if (frag_src)
		glDeleteShader(frag);
#endif
err_frag:
	if (vert_src)
		glDeleteShader(vert);
err_vert:

	return ret;
}

GLint
shader_load(struct shader *s, const char *vert_src, const char *frag_src, const char *geom_src)
{
	return shader_reload(s, vert_src, frag_src, geom_src);
}

void
shader_free(struct shader *s)
{
	glDetachShader(s->prog, s->vert);
	glDetachShader(s->prog, s->frag);
	glDeleteShader(s->vert);
	glDeleteShader(s->frag);
	glDeleteProgram(s->prog);
	s->vert = 0;
	s->frag = 0;
	s->prog = 0;
}

vec4
ray_intersect_mesh(vec3 org, vec3 dir, struct mesh *mesh, mat4 *xfrm)
{
	vec4 q = { 0 };
	unsigned int i;
	float dist = 10000.0; /* TODO: find a sane max value */
	float *pos = mesh->positions;

	if (!pos)
		return q;
	if (mesh->primitive != GL_TRIANGLES)
		return q; /* not triangulated, need indexes as well */

	for (i = 0; i < mesh->vertex_count; i += 3) {
		size_t idx = i * 3;
		vec3 t1 = mat4_mult_vec3(xfrm, (vec3){ pos[idx + 0], pos[idx + 1], pos[idx + 2] });
		vec3 t2 = mat4_mult_vec3(xfrm, (vec3){ pos[idx + 3], pos[idx + 4], pos[idx + 5] });
		vec3 t3 = mat4_mult_vec3(xfrm, (vec3){ pos[idx + 6], pos[idx + 7], pos[idx + 8] });
		vec3 n = vec3_normalize(vec3_cross(vec3_sub(t2, t1), vec3_sub(t3, t1)));
		vec4 plane = { n.x, n.y, n.z, vec3_dot(t1, n)};
		float d = ray_distance_to_plane(org, dir, plane);
		if (d >= 0 && d < dist) {
			vec3 p = vec3_add(vec3_mult(d, dir), org);
			if (point_in_triangle(p, t1, t2, t3)) {
				dist = d;
				q = (vec4) { p.x, p.y, p.z, d };
			}
		}
	}
	return q;
}

struct texture
create_2d_tex(size_t w, size_t h, GLenum format, GLenum type, void *data)
{
	struct texture tex;

	tex.type = GL_TEXTURE_2D;
	tex.width = w;
	tex.height = h;

	glGenTextures(1, &tex.id);
	glBindTexture(tex.type, tex.id);

	glTexParameteri(tex.type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(tex.type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(tex.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(tex.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	/* for now input format is the same as the texture format */
	glTexImage2D(tex.type, 0, format, w, h, 0, format, type, data);

	return tex;
}

void
delete_tex(struct texture *texture)
{
	glDeleteTextures(1, &texture->id);
}
