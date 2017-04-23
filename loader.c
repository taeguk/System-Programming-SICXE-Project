#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loader.h"
#include "symbol.h"
#include "list.h"

/*
 * ESTAB -> symbol table 활용
 * load map -> 자체 구조체의 linked list로서 구현.
 * Reference Number -> array table
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

static int loader_pass_2_to_one_obj (uint32_t *CSADDR, const struct symbol_manager *ESTAB,
                                     struct memory_manager *memory_manager, const char *obj_file);
static int loader_pass_2 (uint32_t progaddr, const struct symbol_manager *ESTAB,
                          struct memory_manager *memory_manager,
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
      fprintf (stderr, "[ERROR] An error occurs in pass 1 of loader.\n");
      ret = -1;
      goto ERROR;
    }

  if (loader_pass_2 (progaddr, ESTAB, memory_manager, obj_file_list, count) != 0)
    {
      fprintf (stderr, "[ERROR] An error occurs in pass 2 of loader.\n");
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
              struct symbol symbol;
              struct load_map_node *node = malloc (sizeof(*node));

              node->type = LOAD_MAP_NODE_TYPE_SYMBOL;
              sscanf (obj_buf + 1 + i * 12, "%6s%06X", node->name, &offset);
              node->address = base_address + offset;

              list_push_back (load_map, &node->list_node);

              strncpy (symbol.label, node->name, SYMBOL_NAME_MAX_LEN);
              symbol.LOCCTR = node->address;
              symbol_insert (ESTAB, &symbol);

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

static int loader_pass_2_to_one_obj (uint32_t *CSADDR, const struct symbol_manager *ESTAB,
                                     struct memory_manager *memory_manager, const char *obj_file)
{
// * Must be ascending order. *
#define STATE_HEADER_PART         0  // 'H', 'R'
#define STATE_TEXT_PART           1  // 'T'
#define STATE_MODIFICATION_PART   2  // 'M'

  const char ch_to_state[128] = {
      ['H'] = STATE_HEADER_PART,
      ['R'] = STATE_HEADER_PART,
      ['T'] = STATE_TEXT_PART,
      ['M'] = STATE_MODIFICATION_PART
  };

  int ret;
  FILE *obj_fp;
  char obj_buf[MAX_OBJECT_BUF_LEN+1];
  int state = STATE_HEADER_PART;
  uint32_t ref_no_to_address[256];
  bool ref_no_validation[256] = { false, };
  uint32_t base_address = *CSADDR;

  ref_no_validation[1] = true;
  ref_no_to_address[1] = base_address;

  obj_fp = fopen (obj_file, "rt");
  if (!obj_fp)
    {
      ret = -1;
      goto ERROR;
    }

  // fprintf (stderr, "[DEBUG] PASS 2 to %s\n", obj_file);

#define CHECK_AND_UPDATE_STATE(ch) \
  if (state > ch_to_state[ch]) \
  { \
    ret = -1; \
    goto ERROR; \
  } \
  state = ch_to_state[ch];

  while (fgets (obj_buf, MAX_OBJECT_BUF_LEN, obj_fp) != NULL)
    {
      // fprintf (stderr, "[DEBUG] %s", obj_buf);
      if (obj_buf[0] == 'H')
        {
          CHECK_AND_UPDATE_STATE (obj_buf[0]);
          char dummy_name[8];
          uint32_t dummy_hex, length;
          sscanf (obj_buf, "H%6s%06X%06X", dummy_name, &dummy_hex, &length);
          *CSADDR += length;
        }
      else if (obj_buf[0] == 'R')
        {
          CHECK_AND_UPDATE_STATE (obj_buf[0]);

          uint32_t ref_no;
          char label[8];
          int read_cnt;
          char *ptr = obj_buf + 1;
          // fprintf (stderr, "[DEBUG] Rest = %s", ptr);

          while (sscanf (ptr, "%02X%6s%n", &ref_no, label, &read_cnt) > 0)
            {
              ptr += read_cnt;
              // fprintf (stderr, "[DEBUG] read_cnt = %d, ref_no = %02X, label = %s, Rest = %s", read_cnt, ref_no, label, ptr);

              const struct symbol *symbol = symbol_find (ESTAB, label);
              if (!symbol)
                {
                  ret = -1;
                  goto ERROR;
                }

              ref_no_to_address[ref_no] = symbol->LOCCTR;
              ref_no_validation[ref_no] = true;
            }
        }
      else if (obj_buf[0] == 'T')
        {
          CHECK_AND_UPDATE_STATE (obj_buf[0]);

          uint32_t start_offset;
          uint32_t length;
          sscanf (obj_buf, "T%06X%02X", &start_offset, &length);
          for (int i = 0; i < length; ++i)
            {
              uint32_t val;
              sscanf (obj_buf + 9 + i * 2, "%02X", &val);
              memory_edit (memory_manager, base_address + start_offset + i, val);
            }
        }
      else if (obj_buf[0] == 'M')
        {
          CHECK_AND_UPDATE_STATE (obj_buf[0]);

          uint32_t offset;
          uint32_t length, ref_no;
          char op;

          sscanf (obj_buf, "M%06X%02X%c%02X", &offset, &length, &op, &ref_no);

          if (!ref_no_validation[ref_no])
            {
              ret = -1;
              goto ERROR;
            }

          // fprintf (stderr, "[DEBUG] offset = %06X, length = %02X, op = %c, ref_no = %02X\n", offset, length, op, ref_no);

          uint32_t value = ref_no_to_address[ref_no];
          uint32_t new_value = 0;

          for (int i = 0; i < 3; ++i)
            {
              uint32_t val;
              memory_get (memory_manager, base_address + offset + i, &val);
              new_value <<= 8;
              new_value += val;
            }

          if (length == 5)
            {
              if (op == '+')
                new_value = ((new_value + value) & 0xFFFFF) + ((new_value >> 20) << 20);
              else if (op == '-')
                new_value = ((new_value - value) & 0xFFFFF) + ((new_value >> 20) << 20);
              else
                {ret = -1; goto ERROR;}
            }
          else if (length == 6)
            {
              if (op == '+')
                new_value += value;
              else if (op == '-')
                new_value -= value;
              else
                {ret = -1; goto ERROR;}
            }

          memory_edit (memory_manager, base_address + offset, (uint8_t) (new_value >> 16));
          memory_edit (memory_manager, base_address + offset + 1, (uint8_t) (new_value >> 8));
          memory_edit (memory_manager, base_address + offset + 2, (uint8_t) new_value);
        }
      else
        continue;
    }
#undef CHECK_AND_UPDATE_STATE
#undef STATE_HEADER_PART
#undef STATE_TEXT_PART
#undef STATE_MODIFICATION_PART

  ret = 0;
  goto END;

  ERROR:
  END:
  if (obj_fp)
    fclose (obj_fp);

  return ret;
}

static int loader_pass_2 (uint32_t progaddr, const struct symbol_manager *ESTAB,
                          struct memory_manager *memory_manager,
                          const char *obj_file_list[], int count)
{
  uint32_t CSADDR = progaddr;

  for (int i = 0; i < count; ++i)
    {
      if (loader_pass_2_to_one_obj (&CSADDR, ESTAB, memory_manager, obj_file_list[i]) != 0)
        {
          fprintf (stderr, "[ERROR] An object file (%s) is invalid.\n", obj_file_list[i]);
          return -1;
        }
    }

  return 0;
}