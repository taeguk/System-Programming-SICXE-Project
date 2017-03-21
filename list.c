#include "list.h"
#include <assert.h>
#include <stdlib.h>

struct list
  {
    struct list_node head_node;  // 링크드 리스트의 맨 앞에 위치하는 dummy node
    struct list_node tail_node;  // 링크드 리스트의 맨 뒤에 위치하는 dummy node
  };

/* list를 생성하는 factory 함수 */
struct list *list_construct ()
{
  struct list *list = malloc (sizeof(*list));

  list->head_node.prev = NULL;
  list->head_node.next = &list->tail_node;
  list->tail_node.prev = &list->head_node;
  list->tail_node.next = NULL;

  return list;
}

/* list를 소멸시키는 함수 */
void list_destroy (struct list *list)
{
  assert (list_empty (list));
  free (list);
}

/* list의 시작 node를 반환하는 함수 */
struct list_node *list_begin (struct list *list)
{
  assert (list);
  return list->head_node.next;
}

/* 다음 node를 반환하는 함수 */
struct list_node *list_next (struct list_node *cur_node)
{
  assert (cur_node);
  return cur_node->next;
}

/* list의 끝을 의미하는 node (즉, tail dummy node)를 반환하는 함수 */
struct list_node *list_end (struct list *list)
{
  assert (list);
  return &list->tail_node;
}

/* list가 비었는 지 확인해주는 함수. */
bool list_empty (struct list *list)
{
  assert (list);
  return list->head_node.next == &list->tail_node;
}

/* list의 맨 앞에 node를 추가하는 함수 */
void list_push_front (struct list *list, struct list_node *node)
{
  assert (list && node);

  node->prev = &list->head_node;
  node->next = list->head_node.next;
  list->head_node.next->prev = node;
  list->head_node.next = node;
}

/* list의 맨 뒤에 node를 추가하는 함수 */
void list_push_back (struct list *list, struct list_node *node)
{
  assert (list && node);

  node->prev = list->tail_node.prev;
  node->next = &list->tail_node;
  list->tail_node.prev->next = node;
  list->tail_node.prev = node;
}

/* list의 맨 앞에서 node를 삭제하고 반환하는 함수 */
struct list_node *list_pop_front (struct list *list)
{
  assert (list && list->head_node.next);

  struct list_node *node = list->head_node.next;
  list->head_node.next = node->next;
  node->next->prev = &list->head_node;
  return node;
}

/* list의 맨 뒤에서 node를 삭제하고 반환하는 함수 */
struct list_node *list_pop_back (struct list *list)
{
  assert (list && list->tail_node.prev);

  struct list_node *node = list->tail_node.prev;
  list->tail_node.prev = node->prev;
  node->prev->next = &list->tail_node;
  return node;
}
