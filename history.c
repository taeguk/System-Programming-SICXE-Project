#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "history.h"
#include "command_def.h"
#include "list.h"

struct history_manager
  {
    struct list *history_list;  // history들을 관리하는 링크드 리스트
  };

struct history
  {
    char command_str[COMMAND_INPUT_MAX_LEN+1];
  };

/* history manager 내의 history_list에 사용될 node 구조체 */
struct history_node
  {
    struct history history;
    struct list_node list_node;
  };

/* history manager를 생성하는 factory 함수 */
struct history_manager *history_manager_construct ()
{
  struct history_manager *manager = malloc (sizeof(*manager));

  manager->history_list = list_construct ();

  return manager;
}

/* history manager를 소멸시키는 함수 */
void history_manager_destroy (struct history_manager *manager)
{
  struct list_node *node;
  while ((node = list_pop_front (manager->history_list)))
    {
      free (list_entry (node, struct history_node, list_node));
    }
  list_destroy (manager->history_list);
  free (manager);
}

/* history manager에 history를 추가하는 함수 */
void history_insert (struct history_manager *manager, const char *command_str)
{
  struct history_node *node = malloc (sizeof(*node));

  strncpy (node->history.command_str, command_str, COMMAND_INPUT_MAX_LEN);

  list_push_back (manager->history_list, &node->list_node);
}

/* history manager내에 있는 history 들을 출력하는 함수 */
void history_print (struct history_manager *manager, const char *cur_command_str)
{
  struct list_node *node;
  int no = 0;

  for (node = list_begin (manager->history_list);
       node != list_end (manager->history_list);
       node = list_next (node))
    {
      struct history_node *history_node = list_entry (node, struct history_node, list_node);
      printf ("%-4d %s", ++no, history_node->history.command_str);
    }
  printf ("%-4d %s", ++no, cur_command_str);
}
