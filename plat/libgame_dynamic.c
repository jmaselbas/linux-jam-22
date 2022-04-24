#include <stdio.h>
#include <dlfcn.h>
#include "libgame.h"
#include "core.h"

#ifndef CONFIG_LIBDIR
#define CONFIG_LIBDIR ""
#endif

void
libgame_reload(struct libgame *libgame)
{
	time_t time;
	int ret;

	libgame->init = NULL;
	libgame->step = NULL;
	libgame->fini = NULL;

	if (libgame->handle) {
		ret = dlclose(libgame->handle);
		if (ret)
			fprintf(stderr, "dlclose failed\n");
	}

	time = file_time(libgame->path);
	/* clear previous error message  */
	dlerror();

	libgame->handle = dlopen(libgame->path, RTLD_NOW);

	if (libgame->handle) {
		libgame->init = dlsym(libgame->handle, "game_init");
		libgame->step = dlsym(libgame->handle, "game_step");
		libgame->fini = dlsym(libgame->handle, "game_fini");
		libgame->time = time;
	}
}

void
libgame_init(struct libgame *libgame)
{
	static const char *path[] = {
		"libgame.so",
		CONFIG_LIBDIR"/libgame.so",
		"bin/libgame.so",
		"bin/"CONFIG_LIBDIR"/libgame.so",
	};
	size_t i;

	for (i = 0; i < ARRAY_LEN(path); i++) {
		libgame->handle = dlopen(path[i], RTLD_LAZY);
		if (libgame->handle) {
			libgame->path = path[i];
			fprintf(stderr, "found %s\n", path[i]);
			break;
		}
	}
	if (libgame->path == NULL)
		die("failed to found libgame.so\n");

	libgame_reload(libgame);
	if (!libgame->handle)
		die("dlopen failed: %s\n", dlerror());
}

int
libgame_changed(struct libgame *libgame)
{
	time_t new_time;

	new_time = file_time(libgame->path);

	return libgame->time < new_time;
}
