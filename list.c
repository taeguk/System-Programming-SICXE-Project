#include "list.h"
#include <assert.h>
#include <stdlib.h>

struct list
  {
    struct list_node head_node;
    struct list_node tail_node;
  };

struct list *list_construct ()
{
  struct list *list = malloc (sizeof(*list));

  list->head_node.prev = NULL;
  list->head_node.next = &list->tail_node;
  list->tail_node.prev = &list->head_node;
  list->tail_node.next = NULL;

  return list;
}

void list_destroy (struct list *list)
{
  assert (list_empty (list));
  free (list);
}

struct list_node *list_begin (struct list *list)
{
  assert (list);
  return list->head_node.next;
}

struct list_node *list_next (struct list_node *cur_node)
{
  assert (cur_node);
  return cur_node->next;
}

struct list_node *list_end (struct list *list)
{
  assert (list);
  return &list->tail_node;
}

/* List Property */
bool list_empty (struct list *list)
{
  assert (list);
  return list->head_node.next == &list->tail_node;
}

void list_push_front (struct list *list, struct list_node *node)
{
  assert (list && node);

  node->prev = &list->head_node;
  node->next = list->head_node.next;
  list->head_node.next->prev = node;
  list->head_node.next = node;
}

void list_push_back (struct list *list, struct list_node *node)
{
  assert (list && node);

  node->prev = list->tail_node.prev;
  node->next = &list->tail_node;
  list->tail_node.prev->next = node;
  list->tail_node.prev = node;
}

/* List Remove */
struct list_node *list_pop_front (struct list *list)
{
  assert (list && list->head_node.next);

  struct list_node *node = list->head_node.next;
  list->head_node.next = node->next;
  node->next->prev = &list->head_node;
  return node;
}

struct list_node *list_pop_back (struct list *list)
{
  assert (list && list->tail_node.prev);

  struct list_node *node = list->tail_node.prev;
  list->tail_node.prev = node->prev;
  node->prev->next = &list->tail_node;
  return node;
}
