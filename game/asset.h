#pragma once

enum asset_key {
	DEBUG_MESH_CROSS,
	DEBUG_MESH_CYLINDER,
	DEBUG_MESH_CUBE,
	DEBUG_MESH_SPHERE,
	DEBUG_SHADER_TEXTURE,
	MESH_QUAD,
	MESH_WALL,
	MESH_FLOOR,
	MESH_BOX,
	MESH_D_PLAT,
	SHADER_SOLID,
	SHADER_WORLD,
	SHADER_TEXT,
	TEXTURE_TEXT,
	TEXTURE_LINE,
	SOUND_BIRDS,
	FONT_META,
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
