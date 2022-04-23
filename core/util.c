#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "engine.h"

void
warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	exit(1);
}

#define ALIGN(x, a) ((x) + ((a) - (x)) % (a))

void *
mempush(struct memory_zone *zone, size_t size)
{
	void *addr = NULL;

	if (zone->used + size <= zone->size) {
		addr = zone->base + zone->used;
		zone->used += ALIGN(size, 16);
	} else {
		die("mempush: Not enough memory\n");
	}

	return addr;
}

void
mempull(struct memory_zone *zone, size_t size)
{
	if (size > zone->used)
		die("mempull: Freed too much memory\n");

	zone->used -= size;
}

struct memory_zone
memory_zone_init(void *base, size_t size)
{
	struct memory_zone zone;

	zone.base = base;
	zone.size = size;
	zone.used = 0;

	return zone;
}
