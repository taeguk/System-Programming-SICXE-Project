#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdbool.h>
#include "history.h"
#include "memory.h"
#include "opcode.h"
#include "symbol.h"
#include "command_def.h"

/* Command Loop의 state를 의미하는 구조체 
 * command loop 내에서 이 state 구조체 정보를 바탕으로 사용자 명령들을 처리하게 된다.
 * 만약, memory manager를 바꾼다던지 등등, state를 변경하고 싶다면, command loop내에서
 * 이 구조체 내의 값들을 변경하면 된다.
 */
struct command_state
  {
    struct history_manager *history_manager;
    struct memory_manager *memory_manager;
    struct opcode_manager *opcode_manager;
    struct symbol_manager *symbol_manager;
    uint32_t saved_dump_start; /* parameter가 없는 dump 명령어에서 위치를 저장하기 위해 쓰인다. */
  };

/* 사용자의 입력을 받아서 처리하는 Command Loop로 진입하는 함수. */
bool command_loop (struct command_state *state);

#endif
