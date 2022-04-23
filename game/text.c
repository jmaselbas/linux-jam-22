#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "game.h"
#include "asset.h"
#include "render.h"
#include "text.h"

void
sys_text_push(struct system *sys, struct text_entry *entry)
{
	struct text_entry *txt;
	size_t total_size = sizeof(*txt) + entry->len * sizeof(char);
	char *t;
	txt = list_push(&sys->list, &sys->zone, total_size);
	*txt = *entry;
	t = (char *)(txt + 1); /* put the text after the end of struct text_entry */

	/* make a local copy of given text */
	memcpy(t, entry->text, entry->len);
	txt->text = t;
}

void
sys_text_puts(struct system *sys, vec2 at, vec3 color, char *s)
{
	float width = sys->game_state->width;
	float height = sys->game_state->height;
	size_t len = strlen(s);
	float size = 64;
	vec3 txt_scale = {size/width, size/height, 1.0};

	sys_text_push(sys, &(struct text_entry) {
			.text = s,
			.len = len,
			.position = {at.x, at.y, 0},
			.rotation = QUATERNION_IDENTITY,
			.scale = txt_scale,
			.color = color,
		});
}

void
sys_text_printf(struct system *sys, vec2 at, vec3 color, char *fmt, ...)
{
	char buf[512];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	sys_text_puts(sys, at, color, buf);
}

static int
text_fill_vbo(struct font *font, const char *text, size_t len, vec3 at, size_t limit, float *pos, float *tex)
{
	float w, h;
	size_t i;

	if (font->atlas == NULL || font->meta == NULL)
		return 0;

	w = font->atlas->width;
	h = font->atlas->height;

	for (i = 0; *text != '\0' && i < len && i < limit; i++, text++) {
		unsigned char c = *text;
		struct glyph g = font->meta->glyph[c];
		float pt = at.y + g.off.y; /* top */
		float pl = at.x + g.off.x; /* left */
		float pb = at.y + g.off.y + g.ext.y; /* bottom */
		float pr = at.x + g.off.x + g.ext.x; /* right */
		float tt = g.at / h;
		float tl = g.al / w;
		float tb = g.ab / h;
		float tr = g.ar / w;

		/*
                    ^
		    |  c    b
                    |  +--+ +
                  at|  | / /|
                  --+--|/ / |-----> baseline
		    |  + +--+
		    |  a    d
		 */

		/* bottom left (a) */
		pos[18 * i + 0] = pl;
		pos[18 * i + 1] = pb;
		pos[18 * i + 2] = 0;
		tex[12 * i + 0] = tl;
		tex[12 * i + 1] = tb;

                /* top right (b) */
		pos[18 * i + 3] = pr;
		pos[18 * i + 4] = pt;
		pos[18 * i + 5] = 0;
		tex[12 * i + 2] = tr;
		tex[12 * i + 3] = tt;

		/* top left (c) */
		pos[18 * i + 6] = pl;
		pos[18 * i + 7] = pt;
		pos[18 * i + 8] = 0;
		tex[12 * i + 4] = tl;
		tex[12 * i + 5] = tt;

		/* triangle 2 */
		/* bottom left (a) */
		pos[18 * i +  9] = pl;
		pos[18 * i + 10] = pb;
		pos[18 * i + 11] = 0;
		tex[12 * i + 6] = tl;
		tex[12 * i + 7] = tb;

		/* bottom right (d) */
		pos[18 * i + 12] = pr;
		pos[18 * i + 13] = pb;
		pos[18 * i + 14] = 0;
		tex[12 * i + 8] = tr;
		tex[12 * i + 9] = tb;

		/* top right (b) */
		pos[18 * i + 15] = pr;
		pos[18 * i + 16] = pt;
		pos[18 * i + 17] = 0;
		tex[12 * i + 10] = tr;
		tex[12 * i + 11] = tt;

		/* increment position */
		at.x += g.adv;
	}

	return i;
}

void
sys_text_exec(struct system *sys)
{
	struct game_state *game_state = sys->game_state;
	struct game_asset *game_asset = sys->game_asset;
	struct shader *shader;
	struct mesh mesh;
	float *positions;
	float *texcoords;
	struct link *link;

	positions = mempush(&sys->zone, 6 * 3 * 512 * sizeof(float));
	texcoords = mempush(&sys->zone, 6 * 2 * 512 * sizeof(float));

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shader = game_get_shader(game_asset, SHADER_TEXT);
	render_bind_shader(shader);
	GLint time = glGetUniformLocation(shader->prog, "time");
	if (time >= 0)
		glUniform1f(time, game_state->last_input.time);

	render_bind_camera(shader, &game_state->cam);

	for (link = sys->list.first; link != NULL; link = link->next) {
		struct text_entry e = *(struct text_entry *)link->data;
		struct font *font = &game_state->ascii_font;
		const char *text = e.text;
		size_t len = e.len;
		int count;
		mat4 transform = mat4_transform_scale(e.position,
						      e.rotation,
						      e.scale);

		GLint model = glGetUniformLocation(shader->prog, "model");
		if (model >= 0)
			glUniformMatrix4fv(model, 1, GL_FALSE, (float *)&transform.m);

		GLint color = glGetUniformLocation(shader->prog, "color");
		if (color >= 0)
			glUniform3f(color, e.color.x, e.color.y, e.color.z);

		GLint fx = glGetUniformLocation(shader->prog, "fx");
		if (fx >= 0)
			glUniform1f(fx, e.fx);

		struct texture *texture = font->atlas;
		int unit = 0;
		GLint tex = glGetUniformLocation(shader->prog, "tex");
		if (tex) {
			glActiveTexture(GL_TEXTURE0 + unit);
			glUniform1i(tex, unit);
			glBindTexture(texture->type, texture->id);
		}

		count = text_fill_vbo(font, text, len, e.position, 512, positions, texcoords);
		mesh_load(&mesh, 6 * count, GL_TRIANGLES, positions, NULL, texcoords);
		render_bind_mesh(shader, &mesh);
		render_mesh(&mesh);

		mesh_free(&mesh);
	}
}
