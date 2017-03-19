#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#include "list.h"

struct hash_table_node
  {
    struct list_node list_node;
  };

struct hash_table
  {
    struct list *buckets;
    size_t bucket_cnt;
  };

#define hash_table_entry (HASH_TABLE_NODE, ENTRY_STRUCT, NODE_MEMBER) \
  ((ENTRY_STRUCT *) ((int8_t *) &(HASH_TABLE_NODE)->list_node - offsetof(ENTRY_STRUCT, NODE_MEMBER.list_node))

void hash_table_init (struct hash_table *hash_table, size_t bucket_cnt);
void hash_table_destroy (struct hash_table *hash_table);

void hash_table_insert (struct hash_table *hash_table, struct hash_table_node *node);
void hash_table_find (struct hash_table *hash_table, struct hash_table_node *node);

///////////////////////////
static size_t hash_string (const char *str, size_t bucket_cnt)
{
  int32_t hash = 2729;
  int c;
  while(c = *str++)
    hash = (hash * 585) + c;
  return hash % bucket_cnt;
}

#endif
