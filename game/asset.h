#pragma once

enum asset_key {
	DEBUG_MESH_CROSS,
	DEBUG_MESH_CYLINDER,
	DEBUG_MESH_CUBE,
	DEBUG_MESH_SPHERE,
	MESH_QUAD,
	MESH_TEST,
	MESH_WALL,
	MESH_HALL,
	MESH_ROOM,
	MESH_CABLE_1,
	MESH_CABLE_2,
	MESH_BED_1,
	MESH_BED_2,
	MESH_BEAM,
	MESH_ROCK_FLOOR,
	MESH_ROCK_TEST,
	MESH_ROCK_PILAR_1,
	MESH_GRASS_PATCH_1,
	DEBUG_SHADER_TEXTURE,
	SHADER_SOLID,
	SHADER_TEXT,
	SHADER_TEST,
	SHADER_WALL,
	SHADER_GRAD,
	SHADER_GRASS,
	SHADER_SKY,
	TEXTURE_NOISE,
	TEXTURE_TEXT,
	FONT_META,
	TEXTURE_CONCRETE,
	TEXTURE_CONCRETE_NORMAL,
	OGG_DRONE_BASS_1,
	OGG_DRONE_BELL_1,
	OGG_DRONE_HIGH_1,
	OGG_DRONE_HIGH_2,
	OGG_DRONE_RYTHM_1,
	OGG_DRONE_RYTHM_2,
	OGG_DRONE_RYTHM_3,
	SMF_SONG,
	SYN_PRE_1,
	ASSET_KEY_COUNT,
	/* internal assets id starts here, they are not handled as regular
	 * assets and should not be passed to game_get_*() */
	INTERNAL_TEXTURE,
};

enum asset_state {
	STATE_UNLOAD,
	STATE_LOADED,
};

struct game_asset {
	struct memory_zone *memzone;
	struct memory_zone  tmpzone;
	struct memory_zone *samples;
	struct file_io *file_io;
	struct res_data {
		enum asset_state state;
		time_t since;
		size_t size;
		void *base;
	} assets[ASSET_KEY_COUNT];
};

void game_asset_init(struct game_asset *game_asset, struct memory_zone *memzone, struct memory_zone *samples, struct file_io *file_io);
void game_asset_fini(struct game_asset *game_asset);
void game_asset_poll(struct game_asset *game_asset);

struct shader *game_get_shader(struct game_asset *game_asset, enum asset_key key);
struct mesh *game_get_mesh(struct game_asset *game_asset, enum asset_key key);
struct wav *game_get_wav(struct game_asset *game_asset, enum asset_key key);
struct texture *game_get_texture(struct game_asset *game_asset, enum asset_key key);
struct font_meta *game_get_font_meta(struct game_asset *game_asset, enum asset_key key);
