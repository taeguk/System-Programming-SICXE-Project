#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdbool.h>
#include <stdint.h>

struct memory_manager;

struct memory_manager *memory_manager_construct (uint32_t memory_size);
void memory_manager_destroy (struct memory_manager *manager);

bool memory_edit (struct memory_manager *manager, uint32_t offset, uint8_t val);
bool memory_fill (struct memory_manager *manager, uint32_t start, uint32_t end, uint8_t val);  // [start, end]
void memory_reset (struct memory_manager *manager);

bool memory_dump (struct memory_manager *manager, uint32_t start, uint32_t end, bool enable_max_end);  // [start, end].

uint32_t memory_get_memory_size (struct memory_manager *manager);

#endif
