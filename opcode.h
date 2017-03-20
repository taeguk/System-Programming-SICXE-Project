#ifndef __OPCODE_H__
#define __OPCODE_H__

#include <stdint.h>

#define OPCODE_NAME_MAX_LEN 8

enum opcode_format
  {
    OPCODE_FORMAT_1, OPCODE_FORMAT_2, OPCODE_FORMAT_3_4
  };

struct opcode
  {
    uint8_t val;
    char name[OPCODE_NAME_MAX_LEN + 1];
    enum opcode_format op_format;
  };

struct opcode_manager;

struct opcode_manager *opcode_manager_construct ();
void opcode_manager_destroy (struct opcode_manager *manager);

void opcode_insert (struct opcode_manager *manager, const struct opcode *opcode);
const struct opcode *opcode_find (struct opcode_manager *manager, const char *name);  // Be cautious to dangling pointer problem.

void opcode_print_list (struct opcode_manager *manager);

#endif
