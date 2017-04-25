#include <stdio.h>
#include <stdlib.h>

#include "debug.h"

#define INITIAL_BP_LIST_MAX_SIZE 10

struct debug_manager
  {
    uint32_t *bp_list;
    uint32_t cnt;
    uint32_t max_size;
  };

struct debug_manager *debug_manager_construct ()
{
  struct debug_manager *manager = malloc (sizeof (*manager));
  manager->max_size = INITIAL_BP_LIST_MAX_SIZE;
  manager->cnt = 0;
  manager->bp_list = malloc (sizeof (*manager->bp_list) * manager->max_size);
  return manager;
}

void debug_manager_destroy (struct debug_manager *manager)
{
  free (manager->bp_list);
  free (manager);
}

bool debug_bp_add (struct debug_manager *manager, uint32_t address)
{
  if (debug_bp_check (manager, address, address + 1, NULL))
      return false;

  if (manager->cnt >= manager->max_size)
    {
      manager->max_size *= 2;
      manager->bp_list = realloc (manager->bp_list, sizeof (*manager->bp_list) * manager->max_size);
    }
  manager->bp_list[manager->cnt++] = address;
  return true;
}

void debug_bp_clear (struct debug_manager *manager)
{
  manager->cnt = 0;
  manager->max_size = INITIAL_BP_LIST_MAX_SIZE;
  free (manager->bp_list);
  manager->bp_list = malloc (sizeof (*manager->bp_list) * manager->max_size);
}

bool debug_bp_check (const struct debug_manager *manager, uint32_t start, uint32_t end, uint32_t *bp)  // [start, end)
{
  for (int i = 0; i < manager->cnt; ++i)
    {
      if (start <= manager->bp_list[i] && manager->bp_list[i] < end)
        {
          if (bp)
            *bp = manager->bp_list[i];
          return true;
        }
    }
  return false;
}

void debug_bp_print_list (const struct debug_manager *manager)
{
  if (manager->cnt == 0)
    {
      printf ("There is no breakpoint.\n");
    }
  else
    {
      printf ("breakpoint\n"
              "----------\n");
      for (int i = 0; i < manager->cnt; ++i)
        printf ("%04X\n", manager->bp_list[i]);
    }
}
