#ifndef __LIST_H__
#define __LIST_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* list 자료구조의 논리구조를 위한 node 구조체 */
struct list_node
  {
    struct list_node *prev;
    struct list_node *next;
  };

/* list는 오직 factory 함수를 통해서만 생성될 수 있다. */
struct list;

/* list_node로 부터 실제 node (entry) 를 얻어오는 매크로 함수 */
#define list_entry(LIST_NODE, ENTRY_STRUCT, NODE_MEMBER) \
  ((ENTRY_STRUCT *) ((uint8_t *) &((LIST_NODE)->prev) - offsetof(ENTRY_STRUCT, NODE_MEMBER.prev)))

/* List를 생성하고 소멸시키는 함수들 */
struct list *list_construct ();
void list_destroy (struct list *list);  // Must be called when list is empty.

/* List를 순회하는 함수들 */
struct list_node *list_begin (struct list *list);
struct list_node *list_next (struct list_node *cur_node);
struct list_node *list_end (struct list *list);

/* List의 property에 대한 함수들 */
bool list_empty (struct list *list);

/* List에 node를 추가하는 함수들 */
void list_push_front (struct list *list, struct list_node *node);
void list_push_back (struct list *list, struct list_node *node);

/* List에서 node를 삭제하는 함수들 */
struct list_node *list_pop_front (struct list *list);
struct list_node *list_pop_back (struct list *list);

#endif
