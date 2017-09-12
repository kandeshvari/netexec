#pragma once

#include <stdio.h>
#include <malloc.h>
#include <stdbool.h>



typedef struct list_s {
	struct list_s *next;
	struct list_s *prev;
	void *data;
} list_t;

#define list_for_each(pos, head) \
	for (pos = (head->next); pos != NULL; pos = pos->next)

//#define list_remove_item(item, free_data) \
//	(item)->prev->next = (item)->next; \
//	(item)->next->prev = (item)->prev; \
//	if ((free_data)) \
//		free((item)->data); \
//	free((item));

static inline list_t* list_new() {
	// initialize List HEAD
	list_t *list = malloc(sizeof(list_t));
	// TODO: errors
	list->next = NULL;
	list->prev = NULL;
	list->data = NULL;
	return list;
}

#ifndef DEBUG
#define list_debug(x)
#else
static inline void list_debug(list_t *list) {

	log_debug("LIST = %x", list);
	log_debug("LIST.next = %x", list->next);
	log_debug("LIST.prev = %x", list->prev);
	log_debug("LIST.data = %x", list->data);
}
#endif

static inline void list_free(list_t *list, bool free_data, void (*cb)(void *data)) {
	// Remove List
	list_debug(list);
	list_t *pos = list->next;
	list_t *tmp;
	while (pos != NULL) {
//		log_error("FREE POS: [%s]", pos->data);
		list_debug(pos);
		tmp = pos->next;
		if (free_data) {
			if (!cb)
				free(pos->data);
			else
				cb(pos->data);
		}
		free(pos);
		pos = tmp;
	}
	free(list);
}


//static inline void list_insert(int position, const list_t *list, void *data) {
//	while (pos->next)
//		pos = pos->next;
//
//
//	list_t *new_item = malloc(sizeof(list_t));
//	// TODO: errors
//	new_item->next = list->next;
//	new_item->prev = list;
//	if (list->next)
//		list->next->prev = new_item;
//	list->next = new_item;
//	new_item->data = data;
//}

static inline void list_append(const list_t *list, void *data) {

	list_t *new_item = malloc(sizeof(list_t));
	// TODO: errors
	new_item->data = data;

	list_t *pos = (list_t *)list;
	while (pos->next)
		pos = pos->next;

	new_item->next = NULL;
	new_item->prev = pos;
	pos->next = new_item;
}

static inline void* list_pop(const list_t *list) {
	if (!list->next)
		return NULL;
	list_t *pos ;
	for (pos = (list_t *)list; pos->next != NULL; pos = pos->next) {}

	void *data = pos->data;
	if (pos->prev)
		pos->prev->next = NULL;
	list_debug(pos->prev);
	free(pos);
	return data;
}

static inline void list_remove_item(list_t *item, bool free_data) {
	if (item && item->prev) {
//		log_debug("remove item");
		item->prev->next = item->next;
		if (item->next)
			item->next->prev = item->prev;
		if (free_data)
			free(item->data);
		free(item);
	}
}

static inline int list_len(const list_t *list) {
	int count = 0;
	list_t *pos;
	list_for_each(pos, (list_t *)list)
		count++;
	return count;
}


//#define for_each(list, item, data) do { \


//static inline int list_print(list_t *list) {
//	int count = 0;
//	list_t *runner = list;
//	while (runner) {
//		count++;
//		runner = runner->next;
//	}
//	return count;
//}

