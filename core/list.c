#include <stddef.h>
#include "list.h"

void
list_init(struct list *list)
{
	list->first = NULL;
	list->last = NULL;
	list->count = 0;
}

void *
list_push(struct list *list, struct memory_zone *zone, size_t size)
{
	struct link *link = mempush(zone, sizeof(struct link));
	void *data = mempush(zone, size);

	link->next = NULL;
	link->size = size;
	link->data = data;

	if (list->last != NULL)
		list->last->next = link;
	list->last = link;

	if (list->first == NULL)
		list->first = link;
	list->count++;

	return data;
}
