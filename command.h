#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdbool.h>
#include "history.h"
#include "memory.h"
#include "opcode.h"
#include "command_def.h"

struct command_state
  {
    struct history_manager *history_manager;
    struct memory_manager *memory_manager;
    struct opcode_manager *opcode_manager;
    uint32_t saved_dump_start;
  };

bool command_loop (struct command_state *state);

#endif
