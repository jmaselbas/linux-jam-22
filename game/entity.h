#pragma once

enum sound_state {
	RESET,
	CONTINUE,
};

struct components {
	unsigned int is_player:1;
	unsigned int has_debug:1;
	unsigned int has_render:1;
	unsigned int has_sound:1;
	unsigned int has_movement:1;
	unsigned int is_wall:1;
	unsigned int is_box:1;
	unsigned int no_shadow:1;
};

struct box_state {
	unsigned int moving :1;
	unsigned int hilight:1;
};

struct entity {
	struct components components;
	vec3 position;
	vec3 direction;
	quaternion rotation;
	float move_speed;
	float move_dist;

	struct box_state box_state;
	vec2 box_dir;
	vec3 color;
	enum asset_key mesh;
	enum asset_key shader;
	enum asset_key debug_mesh;
	vec3 debug_scale;
	enum sound_state sound_state;
	struct sampler sampler;
	enum asset_key wav;
	size_t track;
};
#define MAX_ENTITY_COUNT 1024
typedef uint16_t entity_id;

void entity_step(struct game_state *game_state);
