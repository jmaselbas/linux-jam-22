#pragma once
#include "game/game.h"

struct libgame {
	void *handle;
	const char *path;
	time_t time;
	game_init_t *init;
	game_step_t *step;
	game_fini_t *fini;
};

void libgame_reload(struct libgame *libgame);
void libgame_init(struct libgame *libgame);
int libgame_changed(struct libgame *libgame);
