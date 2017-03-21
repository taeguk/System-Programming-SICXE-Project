#include "opcode.h"
#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define OPCODE_HASH_TABLE_SIZE 20

struct opcode_manager
  {
    struct list *buckets[OPCODE_HASH_TABLE_SIZE]; // opcode들을 저장하는 linked list 기반의 hash table.
  };

/* opcode manager에서 bucket으로서 사용되는 list를 위한 node */
struct opcode_node
  {
    struct opcode opcode;
    struct list_node list_node;
  };

static size_t hash_string (const char *str, size_t hash_size);

/* opcode manager를 생성하는 factory 함수. */
struct opcode_manager *opcode_manager_construct ()
{
  struct opcode_manager *manager = malloc (sizeof(*manager));

  for (int i = 0; i < OPCODE_HASH_TABLE_SIZE; ++i)
    manager->buckets[i] = list_construct ();

  return manager;
}

/* opcode manager를 소멸시키는 함수 */
void opcode_manager_destroy (struct opcode_manager *manager)
{
  for (int i = 0; i < OPCODE_HASH_TABLE_SIZE; ++i)
    {
      struct list_node *node;
      while ((node = list_pop_front (manager->buckets[i])))
        {
          free (list_entry (node, struct opcode_node, list_node));
        }
      list_destroy (manager->buckets[i]);
    }
  free (manager);
}

/* opcode manager에 opcode를 추가하는 함수 */
void opcode_insert (struct opcode_manager *manager, const struct opcode *opcode)
{
  size_t hash = hash_string (opcode->name, OPCODE_HASH_TABLE_SIZE);
  struct opcode_node *node = malloc (sizeof(*node));

  node->opcode = *opcode;

  list_push_front (manager->buckets[hash], &node->list_node);
}

/* opcode manager에서 이름 (mnemonic) 을 통해 opcode를 찾는 함수 */
const struct opcode *opcode_find (struct opcode_manager *manager, const char *name)
{
  size_t hash = hash_string (name, OPCODE_HASH_TABLE_SIZE);
  struct list_node *node;

  for (node = list_begin (manager->buckets[hash]);
       node != list_end (manager->buckets[hash]);
       node = list_next (node))
    {
      struct opcode_node *opcode_node = list_entry (node, struct opcode_node, list_node);
      if (strncmp (opcode_node->opcode.name, name, OPCODE_NAME_MAX_LEN) == 0)
        return &opcode_node->opcode;
    }

  return NULL;
}

/* opcode manager 내의 opcode들을 모두 출력하는 함수 */
void opcode_print_list (struct opcode_manager *manager)
{
  for (int i = 0; i < OPCODE_HASH_TABLE_SIZE; ++i)
    {
      struct list_node *node, *next_node;

      printf ("%d : ", i);
      
      for (node = list_begin (manager->buckets[i]);
           ;
           node = next_node)
        {
          struct opcode_node *opcode_node = list_entry (node, struct opcode_node, list_node);
          printf ("[%s,%02X]", opcode_node->opcode.name, opcode_node->opcode.val);
          next_node = list_next (node);
          if (next_node == list_end (manager->buckets[i]))
            break;
          else
            printf (" -> ");
        }
      printf ("\n");
    }
}

/* 문자열을 정수형태로 변환해주는 hash function */
static size_t hash_string (const char *str, size_t hash_size)
{
  int32_t hash = 2729;
  int c;
  while((c = *str++))
    hash = (hash * 585) + c;
  return hash % hash_size;
}
