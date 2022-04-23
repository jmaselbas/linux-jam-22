#pragma once
#include "util.h"

struct link {
	struct link *next;
	void *data;
	size_t size;
};

struct list {
	struct link *first;
	struct link *last;
	size_t count;
};

void list_init(struct list *list);
void *list_push(struct list *list, struct memory_zone *zone, size_t size);
