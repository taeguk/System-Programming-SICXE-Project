#ifndef __OPCODE_H__
#define __OPCODE_H__

#include <stdint.h>

#define OPCODE_NAME_MAX_LEN 8

/* opcode의 포멧을 의미하는 enum */
enum opcode_format
  {
    /* FAKE OPCODES */
    OPCODE_START, OPCODE_END,
    OPCODE_BYTE, OPCODE_WORD,
    OPCODE_RESB, OPCODE_RESW,
    OPCODE_BASE, OPCODE_NOBASE, 

    /* REAL OPCODES */
    OPCODE_FORMAT_1, OPCODE_FORMAT_2, OPCODE_FORMAT_3_4
  };

enum opcode_detail_format
  {
    /* For fake opcodes */
    OPCODE_FAKE,

    /* For format 1 */
    OPCODE_FORMAT_1_GENERAL,

    /* For format 2 */
    OPCODE_FORMAT_2_GENERAL, // <mnemonic> r1, r2
    OPCODE_FORMAT_2_ONE_REGISTER, // <mnemonic> r1
    OPCODE_FORMAT_2_REGISTER_N, // <mnemonic> r1, n
    OPCODE_FORMAT_2_ONE_N, // <mnemonic> n

    /* For format 3, 4 */
    OPCODE_FORMAT_3_4_GENERAL, // <mnemonic> m
    OPCODE_FORMAT_3_4_NO_OPERAND, // <mnemonic>
  };

/* opcode를 의미하는 구조체 */
struct opcode
  {
    uint8_t val;  // opcode의 값. 두자리 수 hex 값.
    char name[OPCODE_NAME_MAX_LEN + 1];  // opcode의 이름 (mnemonic)
    enum opcode_format op_format;
    enum opcode_detail_format detail_format;
  };

/* opcode manager는 무조건 factory 함수를 통해서만 생성될 수 있다. */
struct opcode_manager;

/* opcode manager를 생성하고 소멸시키는 함수들 */
struct opcode_manager *opcode_manager_construct ();
void opcode_manager_destroy (struct opcode_manager *manager);

/* opcode manager에 opcode를 추가하는 함수 */
void opcode_insert (struct opcode_manager *manager, const struct opcode *opcode);

/* opcode manager에서 이름 (mnemonic) 을 통해 opcode를 찾는 함수 */
const struct opcode *opcode_find (const struct opcode_manager *manager, const char *name);  // Be cautious to dangling pointer problem.

/* opcode manager 내의 opcode들을 모두 출력하는 함수 */
///************** 수정해야함!!!!! 버그 있음.!! ********///
void opcode_print_list (const struct opcode_manager *manager);

#endif
