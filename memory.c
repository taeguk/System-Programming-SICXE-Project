#include "memory.h"
#include <stdint.h>
#include <string.h>

struct memory_manager
  {
    void *memory;
    uint32_t memory_size;
  };

struct memory_manager *memory_manager_construct (uint32_t memory_size)
{
  struct memory_manager *manager = malloc (sizeof(*manager));
  
  manager->memory = malloc (memory_size);
  manager->memory_size = memory_size;

  return manager;
}

void memory_manager_destroy (struct memory_manager *manager)
{
  free (manager->memory);
  free (manager);
}

void memory_edit (struct memory_manager *manager, uint32_t offset, uint8_t val)
{
  ((uint8_t*) manager->address)[offset] = val;
}

bool memory_fill (struct memory_manager *manager, uint32_t start, uint32_t end, uint8_t val)
{
  if (start > end)
    return false;

  memset (manager->memory + start, val, end-start+1);

  return true;
}

void memory_reset (struct memory_manager *manager)
{
  memset (manager->memory, 0, memory_size);
}

bool memory_dump (struct memory_manager *manager, uint32_t start, uint32_t end)
{
  if (start < 0) start = 0;
  if (end < 0) end = manager->memory_size - 1;

  if (start > end)
    return false;  

  // will be implemented.

  return true;
}


