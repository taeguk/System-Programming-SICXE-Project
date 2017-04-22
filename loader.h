#ifndef __LOADER_H__
#define __LOADER_H__

#include <stdint.h>

#include "memory.h"

/* 성공 시 0, 그렇지 않을 경우 그외의 값을 반환. */
int loader (struct memory_manager *memory_manager, uint32_t progaddr, uint32_t *EXECADDR,
            const char *obj_file_list[], int count);

#endif
