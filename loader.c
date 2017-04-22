#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loader.h"
#include "symbol.h"
#include "list.h"

/*
 * ESTAB -> symbol table 활용
 * load map -> 자체 구조체의 linked list로서 구현.
 * Reference Number -> symbol table을 tricky 하게 이용. (좋지 않은 설계이나 빠른 과제를 위해...)
 */

enum load_map_node_type
  {
    LOAD_MAP_NODE_TYPE_CONTROL_SECTION,
    LOAD_MAP_NODE_TYPE_SYMBOL
  };

#define LOAD_MAP_NODE_NAME_MAX_LEN 10

struct load_map_node
  {
    enum load_map_node_type type;
    char name[LOAD_MAP_NODE_NAME_MAX_LEN + 1];
    uint32_t address;
    uint32_t length;  /* only used when type == LOAD_MAP_NODE_TYPE_CONTROL_SECTION */
    struct list_node list_node;
  };

static int loader_pass_1_to_one_obj (uint32_t *CSADDR, struct symbol_manager *ESTAB, struct list *load_map,
                                     const char *obj_file);
static int loader_pass_1 (uint32_t progaddr, struct symbol_manager *ESTAB, struct list *load_map,
                          const char *obj_file_list[], int count);

static void print_load_map (struct list *load_map);

// TODO: 실패했을 때, memory 원상복구 관련해서 고려하기.
int loader (struct memory_manager *memory_manager, uint32_t progaddr, uint32_t *EXECADDR,
            const char *obj_file_list[], int count)
{
  int ret;
  struct symbol_manager *ESTAB = symbol_manager_construct ();
  struct list *load_map = list_construct ();

  if (loader_pass_1 (progaddr, ESTAB, load_map, obj_file_list, count) != 0)
    {
      fprintf (stderr, "[ERROR] An error occurs in pass 1 of loader.");
      ret = -1;
      goto ERROR;
    }

  print_load_map (load_map);

  ret = 0;
  goto END;

ERROR:
END:
  if (ESTAB)
    symbol_manager_destroy (ESTAB);
  if (load_map)
    {
      while (!list_empty (load_map))
        {
          struct list_node *node = list_pop_front (load_map);
          free (list_entry (node, struct load_map_node, list_node));
        }
      list_destroy (load_map);
    }

  return ret;
}

static void print_load_map (struct list *load_map)
{
  struct list_node *node;
  uint32_t total_length = 0;

  printf ("%-15s %-15s %-15s %-15s\n", "Control", "Symbol", "Address", "Length");
  printf ("%-15s %-15s %-15s %-15s\n", "Section", "Name", "", "");
  printf ("------------------------------------------------------------\n"); // 60 * '-'
  for (node = list_begin (load_map);
       node != list_end (load_map);
       node = list_next (node))
    {
      struct load_map_node *load_map_node = list_entry (node, struct load_map_node, list_node);

      if (load_map_node->type == LOAD_MAP_NODE_TYPE_CONTROL_SECTION)
        {
          printf ("%-15s %-15c %04X%11c %04X%11c\n", load_map_node->name, ' ', load_map_node->address, ' ', load_map_node->length, ' ');
          total_length += load_map_node->length;
        }
      else if (load_map_node->type == LOAD_MAP_NODE_TYPE_SYMBOL)
        {
          printf ("%-15c %-15s %04X%11c %-15c\n", ' ', load_map_node->name, load_map_node->address, ' ', ' ');
        }
    }
  printf ("------------------------------------------------------------\n"); // 60 * '-'
  printf ("%-15c %-15c %-15s %04X%11c\n", ' ', ' ', "Total Length", total_length, ' ');
}

#define MAX_OBJECT_BUF_LEN 1000

static int loader_pass_1_to_one_obj (uint32_t *CSADDR, struct symbol_manager *ESTAB, struct list *load_map,
                                     const char *obj_file)
{
  int ret;
  FILE *obj_fp;
  char obj_buf[MAX_OBJECT_BUF_LEN+1];
  bool header_appear = false;
  uint32_t base_address = *CSADDR;

  obj_fp = fopen (obj_file, "rt");
  if (!obj_fp)
    {
      ret = -1;
      goto ERROR;
    }

  // fprintf (stderr, "[DEBUG] PASS 1 to %s\n", obj_file);

  while (fgets (obj_buf, MAX_OBJECT_BUF_LEN, obj_fp) != NULL)
    {
      if (obj_buf[0] == 'H')
        {
          if (header_appear)
            {
              ret = -1;
              goto ERROR;
            }
          else
            {
              header_appear = true;
            }

          uint32_t dummy;
          struct load_map_node *node = malloc (sizeof(*node));

          node->type = LOAD_MAP_NODE_TYPE_CONTROL_SECTION;
          sscanf (obj_buf, "H%6s%06X%06X", node->name, &dummy, &node->length);
          node->address = base_address;

          list_push_back (load_map, &node->list_node);
          // fprintf (stderr, "[DEBUG] Control Section = %s, %06X %06X\n", node->name, node->address, node->length);

          *CSADDR += node->length;
        }
      else if (obj_buf[0] == 'D')
        {
          int rec_len = strlen(obj_buf);
          rec_len -= 2; /* Exclude 'D' and '\n' */
          if (rec_len % 12 != 0)
            {
              ret = -1;
              goto ERROR;
            }

          for (int i = 0; i < rec_len / 12; ++i)
            {
              uint32_t dummy, offset;
              struct load_map_node *node = malloc (sizeof(*node));

              node->type = LOAD_MAP_NODE_TYPE_SYMBOL;
              sscanf (obj_buf + 1 + i * 12, "%6s%06X", node->name, &offset);
              node->address = base_address + offset;

              list_push_back (load_map, &node->list_node);
              // fprintf (stderr, "[DEBUG] Symbol = %s, %06X\n", node->name, node->address);
            }
        }
      else
        continue;
    }

  if (!header_appear)
    {
      fprintf (stderr, "[ERRPR] Header is not appeared in object file.");
      ret = -1;
      goto ERROR;
    }

  ret = 0;
  goto END;

ERROR:
END:
  if (obj_fp)
    fclose (obj_fp);

  return ret;
}

static int loader_pass_1 (uint32_t progaddr, struct symbol_manager *ESTAB, struct list *load_map,
                          const char *obj_file_list[], int count)
{
  uint32_t CSADDR = progaddr;

  for (int i = 0; i < count; ++i)
    {
      if (loader_pass_1_to_one_obj (&CSADDR, ESTAB, load_map, obj_file_list[i]) != 0)
        {
          fprintf (stderr, "[ERROR] An object file (%s) is invalid.\n", obj_file_list[i]);
          return -1;
        }
    }

  return 0;
}