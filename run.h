#ifndef __RUN_H__
#define __RUN_H__

#include <stdint.h>

#include "memory.h"
#include "debug.h"

struct run_register_set
  {
    uint32_t A, X, L, PC, B, S, T, SW;
  };

/* 성공 시 0, 그렇지 않을 경우 그외의 값을 반환. */
int run (struct memory_manager *memory_manager, const struct debug_manager *debug_manager, 
         struct run_register_set *reg_set);

#endif
