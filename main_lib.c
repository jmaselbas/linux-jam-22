#ifndef CONFIG_LIBDIR
#define CONFIG_LIBDIR ""
#endif

#ifdef CONFIG_DYNAMIC_RELOAD
#include <dlfcn.h>
#endif

static void
libgame_reload(void)
{
#ifdef CONFIG_DYNAMIC_RELOAD
	time_t time;
	int ret;

	libgame.init = NULL;
	libgame.step = NULL;
	libgame.fini = NULL;

	if (libgame.handle) {
		ret = dlclose(libgame.handle);
		if (ret)
			fprintf(stderr, "dlclose failed\n");
	}

	time = file_time(libgame.path);
	/* clear previous error message  */
	dlerror();

	libgame.handle = dlopen(libgame.path, RTLD_NOW);

	if (libgame.handle) {
		libgame.init = dlsym(libgame.handle, "game_init");
		libgame.step = dlsym(libgame.handle, "game_step");
		libgame.fini = dlsym(libgame.handle, "game_fini");
		libgame.time = time;
	}
#endif
}

static void
libgame_init(void)
{
#ifdef CONFIG_DYNAMIC_RELOAD
	static const char *path[] = {
		"libgame.so",
		CONFIG_LIBDIR"/libgame.so",
		"bin/libgame.so",
		"bin/"CONFIG_LIBDIR"/libgame.so",
	};
	size_t i;

	for (i = 0; i < ARRAY_LEN(path); i++) {
		libgame.handle = dlopen(path[i], RTLD_LAZY);
		if (libgame.handle) {
			libgame.path = path[i];
			break;
		}
	}
	if (libgame.path == NULL)
		die("failed to found libgame.so\n");

	libgame_reload();
	if (!libgame.handle)
		die("dlopen failed: %s\n", dlerror());
#endif
}

static int
libgame_changed(void)
{
#ifdef CONFIG_DYNAMIC_RELOAD
	time_t new_time;

	new_time = file_time(libgame.path);

	return libgame.time < new_time;
#endif
	return 0;
}
