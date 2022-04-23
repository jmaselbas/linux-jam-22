#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "core/engine.h"
#include "game.h"
#include "asset.h"

struct asset_file {
	const char *name;
	time_t time;
	ssize_t size;
	char *data;
};

struct obj_info {
	unsigned int vert_count;
	unsigned int texc_count;
	unsigned int norm_count;
	unsigned int face_count;
};

enum res_type {
	UNKNOWN = 0,
	SHADER,
	MESH_INTERNAL,
	MESH_OBJ,
	SOUND_INTERNAL,
	SOUND_WAV,
	SOUND_OGG,
	TEXTURE_PNG,
	FONT_CSV,
};

struct res_entry {
	enum res_type type;
	union {
		struct {
			const char *vert;
			const char *frag;
			const char *geom;
		};
		struct {
			const char *file;
		};
	};
};

static struct res_entry resfiles[ASSET_KEY_COUNT] = {
	[DEBUG_MESH_CROSS] = { MESH_INTERNAL, {}},
	[DEBUG_MESH_CYLINDER] = { MESH_INTERNAL, {} },
	[DEBUG_MESH_CUBE] = { MESH_INTERNAL, {} },
	[MESH_QUAD] = { MESH_INTERNAL, {} },
	[TEXTURE_TEXT] = { TEXTURE_PNG, .file = "res/DejaVuSansMono.png" },
	[FONT_META] = { FONT_CSV, .file = "res/DejaVuSansMono.csv" },
};

struct mesh empty_mesh = { 0 };
/* default tone for debugging purpose, make it all zeros for a silent sound */
static int16_t tone[200] = {
	/* 0x1000 * sin(480 * M_PI * i / 48000.0)); */
	0x0000, 0x0080, 0x0101, 0x0181, 0x0201, 0x0280, 0x02ff, 0x037d,
	0x03fa, 0x0476, 0x04f1, 0x056b, 0x05e3, 0x065a, 0x06cf, 0x0743,
	0x07b5, 0x0825, 0x0892, 0x08fe, 0x0967, 0x09ce, 0x0a32, 0x0a94,
	0x0af3, 0x0b50, 0x0ba9, 0x0c00, 0x0c54, 0x0ca4, 0x0cf1, 0x0d3b,
	0x0d82, 0x0dc5, 0x0e05, 0x0e41, 0x0e7a, 0x0eaf, 0x0ee0, 0x0f0d,
	0x0f37, 0x0f5d, 0x0f7f, 0x0f9d, 0x0fb7, 0x0fcd, 0x0fdf, 0x0fed,
	0x0ff7, 0x0ffd, 0x1000, 0x0ffd, 0x0ff7, 0x0fed, 0x0fdf, 0x0fcd,
	0x0fb7, 0x0f9d, 0x0f7f, 0x0f5d, 0x0f37, 0x0f0d, 0x0ee0, 0x0eaf,
	0x0e7a, 0x0e41, 0x0e05, 0x0dc5, 0x0d82, 0x0d3b, 0x0cf1, 0x0ca4,
	0x0c54, 0x0c00, 0x0ba9, 0x0b50, 0x0af3, 0x0a94, 0x0a32, 0x09ce,
	0x0967, 0x08fe, 0x0892, 0x0825, 0x07b5, 0x0743, 0x06cf, 0x065a,
	0x05e3, 0x056b, 0x04f1, 0x0476, 0x03fa, 0x037d, 0x02ff, 0x0280,
	0x0201, 0x0181, 0x0101, 0x0080, 0x0000, 0xff80, 0xfeff, 0xfe7f,
	0xfdff, 0xfd80, 0xfd01, 0xfc83, 0xfc06, 0xfb8a, 0xfb0f, 0xfa95,
	0xfa1d, 0xf9a6, 0xf931, 0xf8bd, 0xf84b, 0xf7db, 0xf76e, 0xf702,
	0xf699, 0xf632, 0xf5ce, 0xf56c, 0xf50d, 0xf4b0, 0xf457, 0xf400,
	0xf3ac, 0xf35c, 0xf30f, 0xf2c5, 0xf27e, 0xf23b, 0xf1fb, 0xf1bf,
	0xf186, 0xf151, 0xf120, 0xf0f3, 0xf0c9, 0xf0a3, 0xf081, 0xf063,
	0xf049, 0xf033, 0xf021, 0xf013, 0xf009, 0xf003, 0xf000, 0xf003,
	0xf009, 0xf013, 0xf021, 0xf033, 0xf049, 0xf063, 0xf081, 0xf0a3,
	0xf0c9, 0xf0f3, 0xf120, 0xf151, 0xf186, 0xf1bf, 0xf1fb, 0xf23b,
	0xf27e, 0xf2c5, 0xf30f, 0xf35c, 0xf3ac, 0xf400, 0xf457, 0xf4b0,
	0xf50d, 0xf56c, 0xf5ce, 0xf632, 0xf699, 0xf702, 0xf76e, 0xf7db,
	0xf84b, 0xf8bd, 0xf931, 0xf9a6, 0xfa1d, 0xfa95, 0xfb0f, 0xfb8a,
	0xfc06, 0xfc83, 0xfd01, 0xfd80, 0xfdff, 0xfe7f, 0xfeff, 0xff80,
};

static struct wav silent_wav = {
	.header = { .channels = 1 },
	.extras = {
		.samplesize = sizeof(tone[0]),
		.nb_frames = ARRAY_LEN(tone),
		.nb_samples = ARRAY_LEN(tone),
	},
	.audio_data = tone,
};

static int res_file_changed(struct game_asset *game_asset, struct res_entry *res, time_t since);
static void res_reload_shader(struct game_asset *game_asset, enum asset_key key);
static void res_reload_mesh_obj(struct game_asset *game_asset, enum asset_key key);
static void res_reload_wav(struct game_asset *game_asset, enum asset_key key);
static void res_reload_ogg(struct game_asset *game_asset, enum asset_key key);
static void res_reload_png(struct game_asset *game_asset, enum asset_key key);
static void res_reload_font_meta(struct game_asset *game_asset, enum asset_key key);
static void init_wav(struct wav *wav, char *obj);
static struct obj_info read_obj_info(struct asset_file *file);
static void load_obj(struct memory_zone *zone, struct asset_file *file, struct obj_info info,
	 size_t count, float *out_vert, float *out_norm, float *out_texc);

static struct res_data
asset_push_res_data(struct game_asset *game_asset, size_t size)
{
	struct res_data res = { };

	res.size = size;
	res.base = mempush(game_asset->memzone, res.size);

	return res;
}

static void *
asset_push(struct game_asset *game_asset, enum asset_key key, size_t size)
{
	game_asset->assets[key] = asset_push_res_data(game_asset, size);
	return game_asset->assets[key].base;
}

static void
asset_state(struct game_asset *game_asset, enum asset_key key, enum asset_state state)
{
	game_asset->assets[key].state = state;
}

static void
asset_since(struct game_asset *game_asset, enum asset_key key, time_t new)
{
	time_t old = game_asset->assets[key].since;
	game_asset->assets[key].since = MAX(old, new);
}

static void
asset_reload(struct game_asset *game_asset, enum asset_key key)
{
	struct res_data *res = &game_asset->assets[key];
	enum res_type type = resfiles[key].type;

	switch (type) {
	case SHADER:
		res_reload_shader(game_asset, key);
		break;
	case MESH_OBJ:
		res_reload_mesh_obj(game_asset, key);
		break;
	case SOUND_OGG:
		res_reload_ogg(game_asset, key);
		break;
	case SOUND_WAV:
		res_reload_wav(game_asset, key);
		break;
	case TEXTURE_PNG:
		res_reload_png(game_asset, key);
		break;
	case FONT_CSV:
		res_reload_font_meta(game_asset, key);
		break;
	case UNKNOWN:
	case MESH_INTERNAL:
		switch (key) {
		case DEBUG_MESH_CYLINDER:
			*res = asset_push_res_data(game_asset, sizeof(struct mesh));
			mesh_load_cylinder(res->base, 2, 1, 16);
			asset_state(game_asset, key, STATE_LOADED);
			break;
		case DEBUG_MESH_SPHERE:
			*res = asset_push_res_data(game_asset, sizeof(struct mesh));
			mesh_load_bounding_sphere(res->base, 1.0);
			asset_state(game_asset, key, STATE_LOADED);
			break;
		case DEBUG_MESH_CROSS:
			*res = asset_push_res_data(game_asset, sizeof(struct mesh));
			mesh_load_cross(res->base, 1.0);
			asset_state(game_asset, key, STATE_LOADED);
			break;
		case DEBUG_MESH_CUBE:
			*res = asset_push_res_data(game_asset, sizeof(struct mesh));
			mesh_load_box(res->base, 1, 1, 1);
			asset_state(game_asset, key, STATE_LOADED);
			break;
		case MESH_QUAD:
			*res = asset_push_res_data(game_asset, sizeof(struct mesh));
			mesh_load_quad(res->base, 1, 1);
			asset_state(game_asset, key, STATE_LOADED);
			break;
		default:
			break;
		}
		break;
	default:
		/* do nothing */
		fprintf(stderr, "%s: asset key '%d' not handled\n", __func__, key);
		break;
	}
}

static struct asset_file
res_load_file(struct game_asset *game_asset, struct memory_zone *zone, const char *filename)
{
	struct asset_file f = { 0 };

	f.name = filename;
	f.time = game_asset->file_io->time(filename);
	f.size = game_asset->file_io->size(filename);

	if (f.size > 0)
		f.data = mempush(zone, f.size + 1);
	if (f.data) {
		game_asset->file_io->read(filename, f.data, f.size);
		f.data[f.size] = '\0';
	}

	return f;
}

static void
res_reload_shader(struct game_asset *game_asset, enum asset_key key)
{
	struct memory_zone mem_state = game_asset->tmpzone; /* save memory state */
	struct res_entry *res = &resfiles[key];
	struct shader *shader;
	struct asset_file vert = {}, frag = {}, geom = {};
	time_t time;
	int ret;

	if (res->vert)
		vert = res_load_file(game_asset, &game_asset->tmpzone, res->vert);
	if (res->frag)
		frag = res_load_file(game_asset, &game_asset->tmpzone, res->frag);
	if (res->geom)
		geom = res_load_file(game_asset, &game_asset->tmpzone, res->geom);

	/* not a valid shader */
	if (!vert.data || !frag.data)
		goto out;

	time = MAX(vert.time, frag.time);
	time = MAX(time, geom.time);

	shader = asset_push(game_asset, key, sizeof(struct shader));

	ret = shader_reload(shader, vert.data, frag.data, geom.data);
	if (ret) {
		printf("failed to reload shader: %s %s\n", vert.data, frag.data);
		asset_state(game_asset, key, STATE_UNLOAD);
	} else {
		asset_since(game_asset, key, time);
		asset_state(game_asset, key, STATE_LOADED);
	}
out:
	/* restore memory zone */
	game_asset->tmpzone = mem_state;
}

static void
res_reload_mesh_obj(struct game_asset *game_asset, enum asset_key key)
{
	struct memory_zone mem_state = game_asset->tmpzone; /* save memory state */
	struct res_entry *res = &resfiles[key];
	struct mesh *mesh;
	struct asset_file file;
	struct obj_info info;
	float *positions;
	float *normals;
	float *texcoords;
	size_t fcount;

	file = res_load_file(game_asset, &game_asset->tmpzone, res->file);
	if (file.data) {
		info = read_obj_info(&file);

		fcount = info.face_count;
		/* keep positions outside of scrap/tmp zone */
		positions = mempush(&game_asset->tmpzone, fcount * 3 * 3 * sizeof(float));
		normals   = mempush(&game_asset->tmpzone, fcount * 3 * 3 * sizeof(float));
		texcoords = NULL;
		if (info.texc_count > 0)
			texcoords = mempush(&game_asset->tmpzone, fcount * 3 * 2 * sizeof(float));

		load_obj(&game_asset->tmpzone, &file, info, fcount, positions, normals, texcoords);

		mesh = asset_push(game_asset, key, sizeof(struct mesh));
		/* for now mesh are triangulates: no index list */
		mesh_load(mesh, fcount * 3, GL_TRIANGLES, positions, normals, texcoords);
		mesh->positions = positions;

		asset_since(game_asset, key, file.time);
		asset_state(game_asset, key, STATE_LOADED);
	}

	/* restore memory zone */
	game_asset->tmpzone = mem_state;
}

static void
res_reload_font_meta(struct game_asset *game_asset, enum asset_key key)
{
	struct memory_zone *mem = &game_asset->tmpzone;
	struct memory_zone mem_state = *mem; /* save memory state */
	struct font_meta *meta;
	struct res_entry *res;
	struct asset_file file;
	char *line;
	size_t glyph_count;
	ssize_t i, len;
	size_t n;
	size_t id;
	float adv,gl,gb,gr,gt,al,ab,ar,at;
	res = &resfiles[key];
	file = res_load_file(game_asset, mem, res->file);
	glyph_count = 0;
	if (file.data) {
		meta = asset_push(game_asset, key, sizeof(struct font_meta));
		/* count the number of lines/glyphs */
		for (i = 0; i < file.size; i += len) {
			line = &file.data[i];
			for (len = 0; (i + len) < file.size; len++)
				if (line[len] == '\n' || line[len] == '\0')
					break;
			if ((i + len) < file.size) {
				glyph_count++;
				line[len++] = '\0'; /* replace '\n' with '\0' */
			}
			n = sscanf(line, "%ld,%f,%f,%f,%f,%f,%f,%f,%f,%f",
				   &id, &adv, &gl, &gb, &gr, &gt, &al, &ab, &ar, &at);
			if (n == 10) {
				meta->glyph[id].adv = adv;
				meta->glyph[id].off.x = 1 + gl;
				meta->glyph[id].off.y = 1 + gt;
				meta->glyph[id].ext.x = gr - gl;
				meta->glyph[id].ext.y = gb - gt;
				meta->glyph[id].al = al;
				meta->glyph[id].ab = ab;
				meta->glyph[id].ar = ar;
				meta->glyph[id].at = at;
			} else {
				fprintf(stderr, "Reload_font_meta: csv Malformed line, read %ld values: %ld\n", n, id);
			}

		}

		meta->glyph_count = glyph_count;

		asset_since(game_asset, key, file.time);
		asset_state(game_asset, key, STATE_LOADED);
	}
	*mem = mem_state;
}

static void
res_reload_wav(struct game_asset *game_asset, enum asset_key key)
{
	struct wav *wav;
	struct res_entry *res = &resfiles[key];
	struct asset_file file;

	file = res_load_file(game_asset, game_asset->samples, res->file);
	if (file.data) {
		wav = asset_push(game_asset, key, sizeof(struct wav));
		init_wav(wav, file.data);
		asset_since(game_asset, key, file.time);
		asset_state(game_asset, key, STATE_LOADED);
	}
}

#include "stb_vorbis.c"

static void
res_reload_ogg(struct game_asset *game_asset, enum asset_key key)
{
	struct memory_zone mem_state = *game_asset->samples;
	struct wav *wav;
	struct res_entry *res = &resfiles[key];
	struct asset_file file;
	int channels, samplerate, frames;
	int16_t *output;

	file = res_load_file(game_asset, game_asset->samples, res->file);
	if (file.data) {
		unsigned char *data = (unsigned char *)file.data;
		frames = stb_vorbis_decode_memory(data, file.size, &channels, &samplerate, &output);
		wav = asset_push(game_asset, key, sizeof(struct wav));
		wav->extras.samplesize = sizeof(uint16_t);
		wav->extras.nb_frames = frames;
		wav->extras.nb_samples = frames * channels;
		wav->header.channels = channels;
		wav->audio_data = output;

		asset_since(game_asset, key, file.time);
		asset_state(game_asset, key, STATE_LOADED);
	}
	/* restore memory zone */
	*game_asset->samples = mem_state;
}

#include "stb_image.h"

static void
res_reload_png(struct game_asset *game_asset, enum asset_key key)
{
	struct memory_zone mem_state = game_asset->tmpzone;
	struct texture *tex;
	struct res_entry *res = &resfiles[key];
	struct asset_file file;
	int w = 1000, h = 1000, n;

	file = res_load_file(game_asset, &game_asset->tmpzone, res->file);
	stbi_set_flip_vertically_on_load(1);
	if (file.data) {
		GLenum format;
		unsigned char *data = stbi_load_from_memory((stbi_uc*) file.data, file.size, &w, &h, &n, 0);

		if (data == NULL || n == 0)
			goto out;

		if (n == 1)
			format = GL_RED;
		else if (n == 2)
			format = GL_RG;
		else if (n == 3)
			format = GL_RGB;
		else if (n == 4)
			format = GL_RGBA;
		else
			format = GL_RED;

		tex = asset_push(game_asset, key, sizeof(struct texture));
		*tex = create_2d_tex(w, h, format, GL_UNSIGNED_BYTE, data);

		asset_since(game_asset, key, file.time);
		asset_state(game_asset, key, STATE_LOADED);
	}
out:
	/* restore memory zone */
	game_asset->tmpzone = mem_state;
}

static int
res_file_changed(struct game_asset *game_asset, struct res_entry *res, time_t since)
{
	time_t ctime, time = since;

	/* expect res->file to be the same as res->vert */
	if (res->file != res->vert)
		die("fixme: res_file_changed\n");

	ctime = game_asset->file_io->time(res->vert);
	time = MAX(ctime, time);
	ctime = game_asset->file_io->time(res->frag);
	time = MAX(ctime, time);
	ctime = game_asset->file_io->time(res->geom);
	time = MAX(ctime, time);

	return since < time;
}

static void *
game_get_asset(struct game_asset *game_asset, enum asset_key key)
{
	void *asset = NULL;

	if (key < ASSET_KEY_COUNT) {
		if (game_asset->assets[key].state != STATE_LOADED)
			asset_reload(game_asset, key);
		asset = game_asset->assets[key].base;
	}

	return asset;
}

struct shader *
game_get_shader(struct game_asset *game_asset, enum asset_key key)
{
	return game_get_asset(game_asset, key);
}

struct mesh *
game_get_mesh(struct game_asset *game_asset, enum asset_key key)
{
	struct mesh *m;

	m = game_get_asset(game_asset, key);
	if (!m)
		m = &empty_mesh;

	return m;
}

struct wav *
game_get_wav(struct game_asset *game_asset, enum asset_key key)
{
	struct wav *wav = NULL;

	wav = game_get_asset(game_asset, key);
	if (!wav)
		wav = &silent_wav;

	return wav;
}

struct texture null_texture = {
	.id = 0, /* unbind */
	.width = 0,
	.height = 0,
};

struct texture *
game_get_texture(struct game_asset *game_asset, enum asset_key key)
{
	struct texture *tex = NULL;

	tex = game_get_asset(game_asset, key);
	if (!tex)
		tex = &null_texture;

	return tex;
}

struct font_meta *
game_get_font_meta(struct game_asset *game_asset, enum asset_key key)
{
	struct font_meta *meta = NULL;

	meta = game_get_asset(game_asset, key);
	if (!meta)
		meta = NULL;

	return meta;
}

void
game_asset_poll(struct game_asset *game_asset)
{
	enum asset_key key;
	time_t since;

	for (key = 0; key < ASSET_KEY_COUNT; key++) {
		since = game_asset->assets[key].since;
		if (game_asset->assets[key].state == STATE_LOADED)
			if (res_file_changed(game_asset, &resfiles[key], since))
				asset_reload(game_asset, key);
	}
}

void
game_asset_init(struct game_asset *game_asset, struct memory_zone *memzone, struct memory_zone *samples, struct file_io *file_io)
{
	struct memory_zone tmpzone;
	tmpzone.base = mempush(memzone, SZ_4M);
	tmpzone.size = SZ_4M;
	tmpzone.used = 0;

	game_asset->memzone = memzone;
	game_asset->samples = samples;
	game_asset->file_io = file_io;
	game_asset->tmpzone = tmpzone;
}

void
game_asset_fini(struct game_asset *game_asset)
{
	enum asset_key key;

	game_asset->samples->used = 0;
	/* mark all asset as unloaded for now */
	for (key = 0; key < ASSET_KEY_COUNT; key++) {
		asset_since(game_asset, key, STATE_UNLOAD);
	}
}

/* ----------------- wav init ------------------ */

static void
init_wav(struct wav *wav, char *obj)
{
	struct header *header = &wav->header;
	void *seek = obj;
	size_t i;

	for (i = 0; i < 4; i++) {
		header->riff_str[i] = *((char*) seek);
		seek += 1;
	}

	header->filesize = *((uint32_t*) seek);
	seek += 4;

	for (i = 0; i < 4; i++) {
		header->wave_str[i] = *((char*) seek);
		seek += 1;
	}

	for (i = 0; i < 4; i++) {
		header->frmt_str[i] = *((char*) seek);
		seek += 1;
	}

	header->frmtsize = *((uint32_t*) seek);
	seek += 4;

	header->encoding = *((uint16_t*) seek);
	seek += 2;

	header->channels = *((uint16_t*) seek);
	seek += 2;

	header->samplerate = *((uint32_t*) seek);
	seek += 4;

	header->byterate = *((uint32_t*) seek);
	seek += 4;

	header->blockalign = *((uint16_t*) seek);
	seek += 2;

	header->sampledpth = *((uint16_t*) seek);
	seek += 2;

	for (i = 0; i < 4; i++) {
		header->data_str[i] = *((char*) seek);
		seek += 1;
	}

	header->datasize = *((uint32_t*) seek);
	seek += 4;

	wav->audio_data = (void*) seek;

	wav->extras.samplesize = header->sampledpth / 8;
	wav->extras.nb_samples = header->datasize / wav->extras.samplesize;
	wav->extras.frame_size = wav->extras.samplesize * header->channels;
	wav->extras.nb_frames  = wav->extras.nb_samples / header->channels;
}

/* ----------------- obj loader ------------------ */

struct vertex_index {
	unsigned int p;
	unsigned int t;
	unsigned int n;
};

/* We consider only triangulated faces */
struct face {
	struct vertex_index v0;
	struct vertex_index v1;
	struct vertex_index v2;
};

static struct obj_info
read_obj_info(struct asset_file *file)
{
	struct obj_info info;
	size_t size = file->size;
	char *obj = file->data;
	char *line;
	size_t i, len = 0;
	size_t vcount = 0;
	size_t tcount = 0;
	size_t ncount = 0;
	size_t fcount = 0;

	/* count the number of vertices, normals, texcoord and faces */
	for (i = 0; i < size; i += len) {
		line = &obj[i];
		for (len = 0; i + len < size; len++)
			if (line[len] == '\n' || line[len] == '\0')
				break;
		if ((i + len) < size)
			line[len++] = '\0'; /* replace '\n' with '\0' */

		if (strncmp("v ", line, strlen("v ")) == 0) {
			vcount++;
		} else if (strncmp("vt ", line, strlen("vt ")) == 0) {
			tcount++;
		} else if (strncmp("vn ", line, strlen("vn ")) == 0) {
			ncount++;
		} else if (strncmp("f ", line, strlen("f ")) == 0) {
			fcount++;
		} else {
			/* ingore the line */
		}
	}
	info.vert_count = vcount;
	info.texc_count = tcount;
	info.norm_count = ncount;
	info.face_count = fcount;

	return info;
}

static void
load_obj(struct memory_zone *zone, struct asset_file *file, struct obj_info info,
	 size_t count, float *out_vert, float *out_norm, float *out_texc)
{
	struct memory_zone memory_state = *zone;
	size_t size = file->size;
	char *obj = file->data;
	char *line;
	int verbose = 0;
	size_t i, len = 0;
	size_t vcount = 0;
	size_t tcount = 0;
	size_t ncount = 0;
	size_t fcount = 0;
	vec3 *obj_vert;
	vec3 *obj_texc;
	vec3 *obj_norm;
	struct face *obj_face;
	float x, y, z;
	int p0, p1, p2, t0, t1, t2, n0, n1, n2;
	int n;

	/* allocate temporary buffer for parsing the obj file */
	obj_vert = mempush(zone, info.vert_count * sizeof(*obj_vert));
	obj_texc = mempush(zone, info.texc_count * sizeof(*obj_texc));
	obj_norm = mempush(zone, info.norm_count * sizeof(*obj_norm));
	obj_face = mempush(zone, info.face_count * sizeof(*obj_face));

	for (i = 0; i < size; i += len) {
		line = &obj[i];
		for (len = 0; (i + len) < size; len++)
			if (line[len] == '\n' || line[len] == '\0')
				break;
		if ((i + len) < size)
			line[len++] = '\0'; /* replace '\n' with '\0' */

		// TODO: line ending with / are continuated;
		if (strncmp("v ", line, strlen("v ")) == 0) {
			n = sscanf(line, "v %f %f %f", &x, &y, &z);
			if (n == 3) {
				if (verbose)
					printf("v %f %f %f\n", x, y, z);
				obj_vert[vcount++] = (vec3) {x, y, z};
			} else {
				fprintf(stderr, "Malformed line starting with 'v '\n");
			}
		} else if (strncmp("vt ", line, strlen("vt ")) == 0) {
			/*
			  We assume all textures are 2D textures:
			  We look for lines of the form
			  "vt u v \n"
			  with:
			  - u the value for the horizontal direction
			  - v the value for the vertical direction
			  TODO: accept 1D, 2D, 3D textures
			  i.e lines of form:
			  "vt u \n"
			  "vt u v \n"
			  "vt u v w \n"
			 */
			n = sscanf(line, "vt %f %f", &x, &y);
			if (n == 2) {
				if (verbose)
					printf("vt %f %f \n", x, y);
				obj_texc[tcount++] = (vec3) {x, y, 0};
			} else {
				fprintf(stderr, "Malformed line starting with 'vt '\n");
			}
		} else if (strncmp("vn ", line, strlen("vn ")) == 0) {
			n = sscanf(line, "vn %f %f %f", &x, &y, &z);
			if (n == 3) {
				if (verbose)
					printf("vn %f %f %f \n", x, y, z);
				obj_norm[ncount++] = (vec3) {x, y, z};
			} else {
				fprintf(stderr, "Malformed line starting with 'vn '\n");
			}
		} else if (strncmp("f ", line, strlen("f ")) == 0) {
			/* A face line starts with 'f' and is followed by triplets
			   of the form "vertex/texcoord/normal".
			   Example, for a mesh with triangular faces:
			   "f a1/b1/c1 a2/b2/c2 a3/b3/c3 \n"
			   With:
			   - a is the index of a vertex in the obj file vertex list
			   - b is the index of a texture coordinate in the obj texcoords list
			   - c is the index of a normal in the obj file normal list
			   Rem:
			   - in obj files, indices starts from 1
			   arrays indices start from 0 hence the Vertices[a-1] bellow.
			   - For now we only accept mesh with triangular faces.
			   Todo: accept more cellular division by adapting
			   format string fmt bellow.
			   - We assume texture coordinates and normal are present
			   Todo: accept faces lines of the form
			   - "f a1 a2 a3 \n"
			   - "f a1/b1 a2/b2 a3/b3 \n"
			   - "f a1/c1 a2/c2 a3/c3 \n"
			*/
			n = sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d",
				   &p0,&t0,&n0, &p1,&t1,&n1, &p2,&t2,&n2);
			if (n != 9) {
				/* without textcoord */
				t0 = t1 = t2 = 0;
				n = sscanf(line, "f %d//%d %d//%d %d//%d",
					   &p0,&n0, &p1,&n1, &p2,&n2);
			}
			if (n == 9 || n == 6) {
				struct face f = {{p0, t0, n0}, {p1, t1, n1},  {p2, t2, n2}};
				if (verbose)
					printf("f %d/%d/%d %d/%d/%d %d/%d/%d \n",
					       p0,t0,n0, p1,t1,n1, p2,t2,n2);
				obj_face[fcount++] = f;
			} else {
				fprintf(stderr, "Malformed line starting with 'f '\n");
			}
		} else {
			/* ingore the line */
		}
	}

	fcount = MIN(fcount, count);
	if (out_vert) {
		for (i = 0; i < fcount; i++) {
			out_vert[i*9 + 0] = obj_vert[obj_face[i].v0.p - 1].x;
			out_vert[i*9 + 1] = obj_vert[obj_face[i].v0.p - 1].y;
			out_vert[i*9 + 2] = obj_vert[obj_face[i].v0.p - 1].z;
			out_vert[i*9 + 3] = obj_vert[obj_face[i].v1.p - 1].x;
			out_vert[i*9 + 4] = obj_vert[obj_face[i].v1.p - 1].y;
			out_vert[i*9 + 5] = obj_vert[obj_face[i].v1.p - 1].z;
			out_vert[i*9 + 6] = obj_vert[obj_face[i].v2.p - 1].x;
			out_vert[i*9 + 7] = obj_vert[obj_face[i].v2.p - 1].y;
			out_vert[i*9 + 8] = obj_vert[obj_face[i].v2.p - 1].z;
		}
	}
	if (out_norm && ncount > 0) {
		for (i = 0; i < fcount; i++) {
			out_norm[i*9 + 0] = obj_norm[obj_face[i].v0.n - 1].x;
			out_norm[i*9 + 1] = obj_norm[obj_face[i].v0.n - 1].y;
			out_norm[i*9 + 2] = obj_norm[obj_face[i].v0.n - 1].z;
			out_norm[i*9 + 3] = obj_norm[obj_face[i].v1.n - 1].x;
			out_norm[i*9 + 4] = obj_norm[obj_face[i].v1.n - 1].y;
			out_norm[i*9 + 5] = obj_norm[obj_face[i].v1.n - 1].z;
			out_norm[i*9 + 6] = obj_norm[obj_face[i].v2.n - 1].x;
			out_norm[i*9 + 7] = obj_norm[obj_face[i].v2.n - 1].y;
			out_norm[i*9 + 8] = obj_norm[obj_face[i].v2.n - 1].z;
		}
	}
	if (out_texc && tcount > 0) {
		for (i = 0; i < fcount; i++) {
			out_texc[i*6 + 0] = obj_texc[obj_face[i].v0.t - 1].x;
			out_texc[i*6 + 1] = obj_texc[obj_face[i].v0.t - 1].y;
			out_texc[i*6 + 2] = obj_texc[obj_face[i].v1.t - 1].x;
			out_texc[i*6 + 3] = obj_texc[obj_face[i].v1.t - 1].y;
			out_texc[i*6 + 4] = obj_texc[obj_face[i].v2.t - 1].x;
			out_texc[i*6 + 5] = obj_texc[obj_face[i].v2.t - 1].y;
		}
	}

	/* restore memory state */
	*zone = memory_state;
}
