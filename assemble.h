#ifndef __ASSEMBLE_H__
#define __ASSEMBLE_H__

#include "opcode.h"
#include "symbol.h"

/*
// Not used now.. But it can be used later.
struct assemble_result
  {
    int error_code;
    char *error_msg;
  };
*/

/* 성공 시 0, 그렇지 않을 경우 그외의 값을 반환. */
int assemble (const char *filename, const struct opcode_manager *opcode_manager,
              struct symbol_manager *symbol_manager /*, struct assemble_result *result */);

#endif
