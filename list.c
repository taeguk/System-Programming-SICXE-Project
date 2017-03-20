#include "list.h"

struct list_node
  {
    struct list_node *prev;
    struct list_node *next;
  };

struct list
  {
    struct list_node head;
    struct list_node tail;
  };
