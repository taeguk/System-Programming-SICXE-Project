#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdint.h>
#include <stdbool.h>

/* debug manager 오직 factory 함수를 통해서만 생성될 수 있다. */
struct debug_manager;

/* debug manager를 생성하고 소멸시키는 함수들 */
struct debug_manager *debug_manager_construct ();
void debug_manager_destroy (struct debug_manager *manager);

/* break points를 조작하는 함수들 */
bool debug_bp_add (struct debug_manager *manager, uint32_t address);
void debug_bp_clear (struct debug_manager *manager);
bool debug_bp_check (const struct debug_manager *manager, uint32_t start, uint32_t end);  // [start, end)
void debug_bp_print_list (const struct debug_manager *manager);

#endif
