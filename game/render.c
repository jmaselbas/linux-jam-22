#include "game.h"
#include "render.h"
#include "asset.h"

void
render_bind_shader(struct shader *shader)
{
	/* Set the current shader program to shader->prog */
	glUseProgram(shader->prog);
}

void
render_bind_mesh(struct shader *shader, struct mesh *mesh)
{
	GLint position;
	GLint normal;
	GLint texcoord;

	position = glGetAttribLocation(shader->prog, "in_pos");
	normal = glGetAttribLocation(shader->prog, "in_normal");
	texcoord = glGetAttribLocation(shader->prog, "in_texcoord");

	mesh_bind(mesh, position, normal, texcoord);
}

void
render_bind_camera(struct shader *s, struct camera *c)
{
	GLint proj, view;

	proj = glGetUniformLocation(s->prog, "proj");
	view = glGetUniformLocation(s->prog, "view");

	glUniformMatrix4fv(proj, 1, GL_FALSE, (float *)&c->proj.m);
	glUniformMatrix4fv(view, 1, GL_FALSE, (float *)&c->view.m);
}

void
render_mesh(struct mesh *mesh)
{
	if (mesh->index_count > 0)
		glDrawElements(mesh->primitive, mesh->index_count, GL_UNSIGNED_INT, 0);
	else
		glDrawArrays(mesh->primitive, 0, mesh->vertex_count);
}

void
sys_render_push(struct system *sys, struct render_entry *entry)
{
	struct render_entry *e;

	e = list_push(&sys->list, &sys->zone, sizeof(*e));
	*e = *entry;
}

static int
frustum_cull(vec4 frustum[6], struct mesh *mesh, vec3 pos, vec3 scale)
{
	vec3 center = mesh->bounding.off;
	float radius = vec3_max(scale) * mesh->bounding.radius;

	/* move the sphere center to the object position */
	pos = vec3_fma(scale, center, pos);

	return sphere_outside_frustum(frustum, pos, radius);
}

void
sys_render_exec(struct system *sys, struct camera cam, int do_frustum_cull, struct shader *ov)
{
	struct game_state *game_state = sys->game_state;
	struct game_asset *game_asset = sys->game_asset;
	struct camera *sun = &game_state->sun;
	struct light *light = &game_state->light;
	enum asset_key last_shader = ASSET_KEY_COUNT;
	enum asset_key last_mesh = ASSET_KEY_COUNT;
	struct shader *shader = NULL;
	struct mesh *mesh = NULL;
	mat4 vm = mat4_mult_mat4(&cam.proj, &cam.view);
	vec4 frustum[6];
	struct link *link;

	mat4_projection_frustum(&vm, frustum);
	for (link = sys->list.first; link != NULL; link = link->next) {
		struct render_entry e = *(struct render_entry *)link->data;

		if (!mesh || last_mesh != e.mesh)
			mesh = game_get_mesh(game_asset, e.mesh);

		if (0 && do_frustum_cull && e.cull && frustum_cull(frustum, mesh, e.position, e.scale))
			continue;

		if (!shader || last_shader != e.shader) {
			last_shader = e.shader;
			if (ov)
				shader = ov;
			else
				shader = game_get_shader(game_asset, e.shader);
			render_bind_shader(shader);
			render_bind_camera(shader, &cam);
			/* mesh need to be bind again */
			last_mesh = e.mesh;
			render_bind_mesh(shader, mesh);
		} else if (last_mesh != e.mesh) {
			last_mesh = e.mesh;
			render_bind_mesh(shader, mesh);
		}

		mat4 transform = mat4_transform_scale(e.position,
						      e.rotation,
						      e.scale);

		GLint model = glGetUniformLocation(shader->prog, "model");
		if (model >= 0)
			glUniformMatrix4fv(model, 1, GL_FALSE, (float *)&transform.m);

		GLint time = glGetUniformLocation(shader->prog, "time");
		if (time >= 0)
			glUniform1f(time, game_state->last_input.time);

		GLint camp = glGetUniformLocation(shader->prog, "camp");
		if (camp >= 0)
			glUniform3f(camp, cam.position.x, cam.position.y, cam.position.z);

		GLint lightd = glGetUniformLocation(shader->prog, "lightd");
		if (lightd >= 0)
			glUniform3f(lightd,
				    light->dir.x,
				    light->dir.y,
				    light->dir.z);

		GLint lightc = glGetUniformLocation(shader->prog, "lightc");
		if (lightc >= 0)
			glUniform4f(lightc,
				    light->col.x,
				    light->col.y,
				    light->col.z,
				    light->col.w
				);
		GLint lsview = glGetUniformLocation(shader->prog, "lsview");
		if (lsview >= 0)
			glUniformMatrix4fv(lsview, 1, GL_FALSE, (float *) sun->view.m);

		GLint lsproj = glGetUniformLocation(shader->prog, "lsproj");
		if (lsproj >= 0)
			glUniformMatrix4fv(lsproj, 1, GL_FALSE, (float *) sun->proj.m);

		GLint color = glGetUniformLocation(shader->prog, "color");
		if (color >= 0)
			glUniform3f(color, e.color.x, e.color.y, e.color.z);

		GLint v2res = glGetUniformLocation(shader->prog, "v2Resolution");
		if (v2res >= 0)
			glUniform2f(v2res, game_state->width, game_state->height);

		GLint thick = glGetUniformLocation(shader->prog, "thickness");
		if (thick >= 0)
			glUniform1f(v2res, 0.93);

		int unit;
		for (unit = 0; unit < RENDER_MAX_TEXTURE_UNIT; unit++) {
			if (e.texture[unit].name == NULL)
				break;
			const char *name = e.texture[unit].name;
			enum asset_key id = e.texture[unit].res_id;
			GLint tex_loc = glGetUniformLocation(shader->prog, name);
			if (tex_loc >= 0) {
				struct texture *tex_res = e.texture[unit].tex;
				if (id < ASSET_KEY_COUNT)
					tex_res = game_get_texture(game_asset, id);

				glActiveTexture(GL_TEXTURE0 + unit);
				glUniform1i(tex_loc, unit);
				glBindTexture(tex_res->type, tex_res->id);
			}
		}

		render_mesh(mesh);
	}
}

void
sys_render_push_cross(struct system *sys, vec3 at, vec3 scale, vec3 color)
{
	sys_render_push(sys, &(struct render_entry){
			.shader = SHADER_SOLID,
			.mesh = DEBUG_MESH_CROSS,
			.scale = scale,
			.position = at,
			.rotation = QUATERNION_IDENTITY,
			.color = color,
		});
}

void
sys_render_push_vec(struct system *sys, vec3 pos, vec3 dir, vec3 color)
{
	vec3 x = vec3_add(pos, vec3_mult(1, dir));

	sys_render_push(sys, &(struct render_entry){
			.shader = SHADER_SOLID,
			.mesh = DEBUG_MESH_CROSS,
			.scale = {0,0,1},
			.position = x,
			.rotation = quaternion_look_at(dir, (vec3){0,1,0}),
			.color = color,
		});
}

void
sys_render_push_bounding(struct system *sys, vec3 pos, vec3 scale, struct bounding_volume *bvol)
{
	float radius = vec3_max(scale) * bvol->radius;

	sys_render_push(sys, &(struct render_entry){
			.shader = SHADER_SOLID,
			.mesh = DEBUG_MESH_SPHERE,
			.scale = vec3_mult(radius, scale),
			.position = vec3_add(pos, bvol->off),
			.rotation = QUATERNION_IDENTITY,
			.color = {1,1,0},
		});
}
