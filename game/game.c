#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#include "game.h"
#include "asset.h"
#include "entity.h"
#include "render.h"
#include "text.h"
#include "sound.h"

void
sys_init(struct system *sys,
	 struct game_state *game_state,
	 struct game_asset *game_asset,
	 struct memory_zone zone)
{
	sys->game_state = game_state;
	sys->game_asset = game_asset;
	sys->zone = zone;
	list_init(&sys->list);
}

struct font
game_load_font(struct game_asset *game_asset)
{
	struct font font;
	font.atlas = game_get_texture(game_asset, TEXTURE_TEXT);
	font.meta = game_get_font_meta(game_asset, FONT_META);
	return font;
}

static void
game_load_entities(struct game_state *game_state)
{
	int id = 0;

	/* player must have the id 0 */
	game_state->entity[id++] = (struct entity){
		.components = {
			.is_player = 1,
			.has_movement = 1,
			.has_debug = 1},
		.move_speed = 0.8,
		.position = {0,0,0},
		.direction = {0,0,0},
		.debug_mesh = DEBUG_MESH_CYLINDER,
		.debug_scale = {0.2, 1, 0.2},
	};

	game_state->entity_count = id;
}

void
game_init(struct game_memory *game_memory, struct file_io *file_io, struct window_io *win_io)
{
	struct game_state *game_state;
	struct game_asset *game_asset;

	game_state = mempush(&game_memory->state, sizeof(struct game_state));
	game_asset = mempush(&game_memory->asset, sizeof(struct game_asset));

	game_state->game_asset = game_asset;
	game_asset_init(game_asset, &game_memory->asset, &game_memory->audio, file_io);

	camera_init(&game_state->cam, 1.05, 1);
	camera_set_znear(&game_state->cam, 0.1);

	camera_set(&game_state->cam, (vec3){-1., 4.06, 8.222}, (quaternion){ {0.018959, 0.429833, 0.009028}, -0.902673});
	camera_look_at(&game_state->cam, (vec3){0.0, 0., 0.}, (vec3){0., 1., 0.});
	game_state->flycam_speed = 1;

	game_state->window_io = win_io;
	game_state->state = GAME_INIT;
	game_state->new_state = GAME_MENU;

	/* light */
	light_init(&game_state->light);

	/* text */
	game_state->ascii_font = game_load_font(game_asset);

	/* init game entities */
	game_load_entities(game_state);
}

void
game_fini(struct game_memory *memory)
{
	struct game_asset *game_asset = memory->asset.base;
	game_asset_fini(game_asset);
}

static void
game_set_player_movement(struct input *input, struct entity *e)
{
	int dir_forw = 0, dir_left = 0;

	if (key_pressed(input, 'W'))
		dir_forw += 1;
	if (key_pressed(input, 'S'))
		dir_forw -= 1;
	if (key_pressed(input, 'A'))
		dir_left += 1;
	if (key_pressed(input, 'D'))
		dir_left -= 1;
	if (dir_forw || dir_left) {
		vec3 forw = vec3_mult(dir_forw, (vec3){0,0,1});
		vec3 left = vec3_mult(dir_left, (vec3){1,0,0});
		vec3 dir = vec3_add(forw, left);
		e->direction = vec3_normalize(dir);
	} else {
		e->direction = VEC3_ZERO;
	}
}

static void
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

static void
debug_light_mark(struct light *l, struct system *sys_render)
{
	vec3 r = {1, 0, 0};
	vec3 g = {0, 1, 0};
	sys_render_push_cross(sys_render, l->pos, (vec3){0.1,0.1,0.1}, g);
	sys_render_push_vec(sys_render, l->pos, l->dir, r);
}

static void
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

static void
flycam_move(struct game_state *game_state, struct input *input)
{
	struct input *last_input = &game_state->last_input;
	float dt = game_state->dt;
	vec3 forw = camera_get_dir(&game_state->cam);
	vec3 left = camera_get_left(&game_state->cam);
	vec3 dir = { 0 };
	int fly_forw, fly_left;
	float dx, dy;
	float speed;

	if (key_pressed(input, KEY_LEFT_SHIFT))
		speed = 10 * dt;
	else
		speed = 2 * dt;

	fly_forw = 0;
	if (key_pressed(input, 'W'))
		fly_forw += 1;
	if (key_pressed(input, 'S'))
		fly_forw -= 1;

	fly_left = 0;
	if (key_pressed(input, 'A'))
		fly_left += 1;
	if (key_pressed(input, 'D'))
		fly_left -= 1;

	if (fly_forw || fly_left) {
		forw = vec3_mult(fly_forw, forw);
		left = vec3_mult(fly_left, left);
		dir = vec3_add(forw, left);
		dir = vec3_normalize(dir);
		dir = vec3_mult(speed, dir);
		camera_move(&game_state->cam, dir);
	}

	dx = input->xinc;
	dy = input->yinc;

	if (dx || dy) {
		camera_rotate(&game_state->cam, VEC3_AXIS_Y, -0.001 * dx);
		left = camera_get_left(&game_state->cam);
		left = vec3_normalize(left);
		camera_rotate(&game_state->cam, left, 0.001 * dy);
	}

	/* drop camera config to stdout */
	if (key_pressed(input, KEY_SPACE) && !key_pressed(last_input, KEY_SPACE)) {
		vec3 pos = game_state->cam.position;
		quaternion rot = game_state->cam.rotation;

		printf("camera_set(&game_state->cam, (vec3){%f, %f, %f}, (quaternion){ {%f, %f, %f}, %f});\n", pos.x, pos.y, pos.z, rot.v.x, rot.v.y, rot.v.z, rot.w);
	}
}

void
game_step(struct game_memory *memory, struct input *input, struct audio *audio)
{
	struct game_state *game_state = memory->state.base;
	struct game_asset *game_asset = memory->asset.base;
	struct system *sys_render = &game_state->sys_render;
	struct system *dbg_render = &game_state->dbg_render;
	struct system *sys_text = &game_state->sys_text;
	struct input *last_input = &game_state->last_input;
	struct light *light = &game_state->light;
	game_state->width = input->width;
	game_state->height = input->height;
	game_state->input = input;
	game_state->dt = input->time - last_input->time;

	(void)(audio);

	memory->scrap.used = 0;
	sys_init(sys_render, game_state, game_asset,
		 memory_zone_init(mempush(&memory->scrap, SZ_4M), SZ_4M));
	sys_init(dbg_render, game_state, game_asset,
		 memory_zone_init(mempush(&memory->scrap, SZ_2M), SZ_2M));
	sys_init(sys_text, game_state, game_asset,
		 memory_zone_init(mempush(&memory->scrap, SZ_4M), SZ_4M));

	camera_set_ratio(&game_state->cam, (float)input->width / (float)input->height);

	if (key_pressed(input, KEY_ESCAPE))
		game_state->new_state = GAME_MENU;

	if (key_pressed(input, 'X')  && !key_pressed(last_input, 'X'))
		game_state->debug = !game_state->debug;

	if (key_pressed(input, 'Z') && !key_pressed(last_input, 'Z')) {
		game_state->flycam = !game_state->flycam;
		game_state->window_io->cursor(!game_state->flycam);
	}

	if (game_state->debug) {
		debug_origin_mark(dbg_render);
		debug_light_mark(&game_state->light, dbg_render);
		if (key_pressed(input, 'R') && !key_pressed(last_input, 'R'))
			game_load_entities(game_state);
		if (key_pressed(input, 'R'))
			sys_text_printf(sys_text, (vec2){0,0.8}, (vec3){1,1,1}, "reloaded entities");

		sys_text_printf(sys_text, (vec2){-1,-1}, (vec3){1,1,1}, "+ %dfps pos:%f %f %f",
				(int)(1.0 / game_state->dt),
				game_state->cam.position.x,
				game_state->cam.position.y,
				game_state->cam.position.z);
	}

	if (game_state->flycam)
		flycam_move(game_state, input);
	else
		game_set_player_movement(game_state->input, &game_state->entity[0]);

	entity_step(game_state);

	struct texture *depth = &game_state->depth;
	*depth = create_2d_tex(1024, 1024, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	unsigned int fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth->id, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
		light_set_pos(light, (vec3){-8.113143, 39.749962, -7.408203});
		light_look_at(light, (vec3){0.0, 0., 0.}, (vec3){0., 1., 0.});

		camera_init(&game_state->sun, 1.0, depth->width / depth->height);
		camera_set(&game_state->sun, light->pos, light->rot);

		/* clean depth buffer */
		glClear(GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, depth->width, depth->height);

		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);

		sys_render_exec(sys_render, game_state->sun, 1);

		if (game_state->debug) {
			debug_texture(dbg_render, (vec2){200, 200}, depth);
		}
	} else {
		printf("Error framebuffer incomplete \n");
	}

	/* Rebind the default framebuffer */
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, input->width, input->height);

	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	sys_render_exec(sys_render, game_state->cam, 1);
	if (game_state->debug)
		sys_render_exec(dbg_render, game_state->cam, 0);
	sys_text_exec(sys_text);

	/* Free texture */
	delete_tex(depth);

	/* Free the framebuffer */
	glDeleteFramebuffers(1, &fbo);

	game_state->last_input = *input;
	game_asset_poll(game_asset);
}
