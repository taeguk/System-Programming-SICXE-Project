#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdbool.h>
#include <stdint.h>

/* memory manager는 오직 factory 함수를 통해서만 생성될 수 있다. */
struct memory_manager;

/* memory manager를 생성하고 소멸시키는 함수들 */
struct memory_manager *memory_manager_construct (uint32_t memory_size);
void memory_manager_destroy (struct memory_manager *manager);

/* memory를 조작하는 함수들 */
bool memory_edit (struct memory_manager *manager, uint32_t offset, uint8_t val);
bool memory_fill (struct memory_manager *manager, uint32_t start, uint32_t end, uint8_t val);  // [start, end]
void memory_reset (struct memory_manager *manager);

/* memory를 dump하여 출력해주는 함수 */
bool memory_dump (struct memory_manager *manager, uint32_t start, uint32_t end, bool enable_max_end);  // [start, end].

/* memory 크기를 반환하는 함수 */
uint32_t memory_get_memory_size (struct memory_manager *manager);

#endif
