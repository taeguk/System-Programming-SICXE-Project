#ifndef __SYMBOL_H__
#define __SYMBOL_H__

#include <stdint.h>

#define SYMBOL_NAME_MAX_LEN 8

/* symbol을 의미하는 구조체 */
struct symbol
  {
    char label[SYMBOL_NAME_MAX_LEN + 1];
    uint32_t LOCCTR;
  };

/* symbol manager는 무조건 factory 함수를 통해서만 생성될 수 있다. */
struct symbol_manager;

/* symbol manager를 생성하고 소멸시키는 함수들 */
struct symbol_manager *symbol_manager_construct ();
void symbol_manager_destroy (struct symbol_manager *manager);

/* symbol manager에 symbol를 추가하는 함수 */
void symbol_insert (struct symbol_manager *manager, const struct symbol *symbol);

/* symbol manager에서 이름 (mnemonic) 을 통해 symbol를 찾는 함수 */
const struct symbol *symbol_find (const struct symbol_manager *manager, const char *label);  // Be cautious to dangling pointer problem.

/* symbol manager 내의 symbol들을 모두 출력하는 함수 */
void symbol_print_list (struct symbol_manager *manager);

#endif
