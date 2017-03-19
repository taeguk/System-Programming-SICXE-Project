#ifndef __HISTORY_H__
#define __HISTORY_H__

#include "list.h"

#define COMMAND_MAX_LEN 50

struct history
  {
    int32_t no;
    char command_str[COMMAND_MAX_LEN+1];
  };

struct history_manager
  {
    struct list history_list;
  };

void history_manager_init (struct history_manager *manager);

const struct history *history_insert (const char *command_str);

void history_print (struct history_manager *manager);

///////////////////////

static struct history_node
  {
    struct history history;
    struct list_node list_node;
  };

#endif
