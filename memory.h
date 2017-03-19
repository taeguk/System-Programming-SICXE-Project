#ifndef __MEMORY_H__
#define __MEMORY_H__

struct memory_manager
  {
    void *memory;
    uint32_t memory_size;
  };

uint8_t memory_edit (void *address, uint8_t val);
void memory_fill (void *start, void *end, uint8_t val);  // [start, end]
void memory_reset ();

// if start == NULL, start is set to 0x0.
// if end == NULL, end is set to max address.
void memory_dump (void *start, void *end, uint8_t val);  // [start, end].

#endif
