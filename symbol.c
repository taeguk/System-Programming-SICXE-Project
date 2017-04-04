#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "symbol.h"
#include "list.h"

#define SYMBOL_HASH_TABLE_SIZE 40

struct symbol_manager
  {
    struct list *buckets[SYMBOL_HASH_TABLE_SIZE]; // symbol들을 저장하는 linked list 기반의 hash table.
  };

/* symbol manager에서 bucket으로서 사용되는 list를 위한 node */
struct symbol_node
  {
    struct symbol symbol;
    struct list_node list_node;
  };

static size_t hash_string (const char *str, size_t hash_size);

/* symbol manager를 생성하는 factory 함수. */
struct symbol_manager *symbol_manager_construct ()
{
  struct symbol_manager *manager = malloc (sizeof(*manager));

  for (int i = 0; i < SYMBOL_HASH_TABLE_SIZE; ++i)
    manager->buckets[i] = list_construct ();

  return manager;
}

/* symbol manager를 소멸시키는 함수 */
void symbol_manager_destroy (struct symbol_manager *manager)
{
  for (int i = 0; i < SYMBOL_HASH_TABLE_SIZE; ++i)
    {
      while (!list_empty (manager->buckets[i]))
        {
          struct list_node *node = list_pop_front (manager->buckets[i]);
          free (list_entry (node, struct symbol_node, list_node));
        }
      list_destroy (manager->buckets[i]);
    }
  free (manager);
}

/* symbol manager에 symbol를 추가하는 함수 */
void symbol_insert (struct symbol_manager *manager, const struct symbol *symbol)
{
  size_t hash = hash_string (symbol->label, SYMBOL_HASH_TABLE_SIZE);
  struct symbol_node *node = malloc (sizeof(*node));

  node->symbol = *symbol;

  list_push_front (manager->buckets[hash], &node->list_node);
}

/* symbol manager에서 이름 (mnemonic) 을 통해 symbol를 찾는 함수 */
const struct symbol *symbol_find (const struct symbol_manager *manager, const char *label)
{
  size_t hash = hash_string (label, SYMBOL_HASH_TABLE_SIZE);
  struct list_node *node;

  for (node = list_begin (manager->buckets[hash]);
       node != list_end (manager->buckets[hash]);
       node = list_next (node))
    {
      struct symbol_node *symbol_node = list_entry (node, struct symbol_node, list_node);
      if (strncmp (symbol_node->symbol.label, label, SYMBOL_NAME_MAX_LEN) == 0)
        return &symbol_node->symbol;
    }

  return NULL;
}

static int symbol_compare_func (const void *A, const void *B)
{
  struct symbol_node *nodeA = *(struct symbol_node **) A;
  struct symbol_node *nodeB = *(struct symbol_node **) B;
  return strcmp (nodeA->symbol.label, nodeB->symbol.label);
}

/* symbol manager 내의 symbol들을 모두 출력하는 함수 */
void symbol_print_list (struct symbol_manager *manager)
{
  struct symbol_node *all_node_list[1111]={0};
  int cnt = 0;

  for (int i = 0; i < SYMBOL_HASH_TABLE_SIZE; ++i)
    {
      struct list_node *node;
      
      for (node = list_begin (manager->buckets[i]);
           node != list_end (manager->buckets[i]);
           node = list_next (node))
        {
          struct symbol_node *symbol_node = list_entry (node, struct symbol_node, list_node);
          all_node_list[cnt++] = symbol_node;
        }
    }

  qsort (all_node_list, cnt, sizeof(struct symbol_node*), symbol_compare_func);

  for (int i = 0; i < cnt; ++i)
    {
      struct symbol_node *symbol_node = all_node_list[i];
      printf ("\t%s\t%04X\n", symbol_node->symbol.label, symbol_node->symbol.LOCCTR);
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
