#ifndef __ASSEMBLE_H__
#define __ASSEMBLE_H__

#include "opcode.h"
#include "symbol.h"

struct assemble_result
  {
    int error_code;
    char *error_msg;
  };

int assemble (const char *filename, const struct opcode_manager *opcode_manager,
              struct symbol_manager *symbol_manager,
              struct assemble_result *result);

#endif
