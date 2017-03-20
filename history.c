#include "history.h"
#include "command.h"
#include <string.h>

typedef int32_t history_no_t;

struct history_manager
  {
    history_no_t history_cnt;
    struct list history_list;
  };

static struct history
  {
    history_no_t no;
    char command_str[COMMAND_INPUT_MAX_LEN+1];
  };

static struct history_node
  {
    struct history history;
    struct list_node list_node;
  };

struct history_manager *history_manager_construct ()
{
  struct history_manager *manager = malloc (sizeof(*manager));

  list_init (&manager->history_list);
  manager->history_cnt = 0;

  return manager;
}

void history_manager_destroy (struct history_manager *manager)
{
  struct list_node *node;
  while (node = list_pop_front (manager->history_list))
    {
      free (list_entry (node, struct opcode_node, list_node));
    }
  free (manager);
}

void history_insert (struct history_manager *manager, const char *command_str)
{
  struct history_node *node = malloc (sizeof(*node));

  node->history.no = manager->history_cnt++;
  strncpy (node->history.command_str, command_str, COMMAND_INPUT_MAX_LEN);

  list_push_back (&manager->history_list, (struct list_node *) node);
}

void history_print (struct history_manager *manager)
{
  struct list_node *node;
  /***************** will be implemented ******/
}

