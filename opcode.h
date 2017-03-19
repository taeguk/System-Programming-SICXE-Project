#ifndef __OPCODE_H__
#define __OPCODE_H__

#include <stdint.h>
#include "list.h"

#define OPCODE_NAME_MAX_LEN 8

#define OPCODE_HASH_TABLE_SIZE 20

enum opcode_format
  {
    FORMAT_1, FORMAT_2, FORMAT_3_4
  };

struct opcode
  {
    uint8_t val;
    char name[OPCODE_NAME_MAX_LEN + 1];
    enum opcode_format op_format;
  };

struct opcode_manager
  {
    struct list buckets[OPCODE_HASH_TABLE_SIZE];
  };

void opcode_manager_init (struct opcode_manager *manager);
void opcode_manager_destroy (struct opcode_manager *manager);

void opcode_insert (struct opcode_manager *manager, const struct opcode *opcode);
struct opcode opcode_find (struct opcode_manager *manager, const char *name);  // need to be optimized.

void opcode_print_list (struct opcode_manager *manager);

#endif
