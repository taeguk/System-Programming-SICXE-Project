#ifndef __MEMORY_H__
#define __MEMORY_H__

struct memory_manager;

struct memory_manager *memory_manager_construct (uint32_t memory_size);
void memory_manager_destroy (struct memory_manager *manager);

void memory_edit (struct memory_manager *manager, uint32_t offset, uint8_t val);
bool memory_fill (struct memory_manager *manager, uint32_t start, uint32_t end, uint8_t val);  // [start, end]
void memory_reset (struct memory_manager *manager);

// if start < 0, start is set to 0.
// if end < 0, end is set to (memory size - 1).
bool memory_dump (struct memory_manager *manager, uint32_t start, uint32_t end);  // [start, end].

#endif
