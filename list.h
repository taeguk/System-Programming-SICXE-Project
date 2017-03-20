#ifndef __LIST_H__
#define __LIST_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct list_node;
struct list;

#define list_entry (LIST_NODE, ENTRY_STRUCT, NODE_MEMBER) \
  ((ENTRY_STRUCT *) ((int8_t *) &((LIST_NODE)->prev) - offsetof(ENTRY_STRUCT, NODE_MEMBER.prev))

/* List Construct and Destroy */
struct list *list_construct ();
void list_destroy (struct list *list);

/* List Traverse */
struct list_node *list_begin (struct list *list);
struct list_node *list_next (struct list_node *cur_node);
struct list_node *list_end (struct list *list);

/* List Property */
size_t list_size (struct list *list);
bool list_empty (struct list *list);

/* List Insert */
void list_insert (struct list_node *prev_node, struct list_node *node);
void list_push_front (struct list *list, struct list_node *node);
void list_push_back (struct list *list, struct list_node *node);

/* List Remove */
struct list_node *list_remove (struct list_node *node);
struct list_node *list_pop_front (struct list *list);
struct list_node *list_pop_back (struct list *list);

#endif
