#include "core.h"
#include "libgame.h"

void libgame_reload(struct libgame *libgame)
{
	UNUSED(libgame);
}

void libgame_init(struct libgame *libgame)
{
	libgame->init = game_init;
	libgame->step = game_step;
	libgame->fini = game_fini;
}

int libgame_changed(struct libgame *libgame)
{
	UNUSED(libgame);
	return 0;
}
