#include <stdbool.h>
#include <stdio.h>

#include "run.h"
#include "opcode.h"

struct inst_param
  {
    union
      {
        struct
          {
            uint16_t r2     : 4;
            uint16_t r1     : 4;
            uint16_t opcode : 8;
          } f2_param;

        struct
          {
            uint32_t address: 12;
            uint32_t e      : 1;  // must be 0
            uint32_t p      : 1;
            uint32_t b      : 1;
            uint32_t x      : 1;
            uint32_t i      : 1;
            uint32_t n      : 1;
            uint32_t opcode : 6;
          } f3_param;

        struct
          {
            uint32_t address: 20;
            uint32_t e      : 1;  // must be 1
            uint32_t p      : 1;
            uint32_t b      : 1;
            uint32_t x      : 1;
            uint32_t i      : 1;
            uint32_t n      : 1;
            uint32_t opcode : 6;
          } f4_param;

        uint32_t val;
      } param;
    bool extend;
  };

typedef void (*inst_handler_t) (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);

static void _h_STA (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_STL (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_STCH (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_STX (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_LDA (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_LDB (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_LDT (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_LDX (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_LDCH (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_JSUB (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_JEQ (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_J (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_JGT (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_JLT (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_RSUB (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_COMP (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_COMPR (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_TD (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_RD (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_WD (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_CLEAR (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);
static void _h_TIXR (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set);

static bool load_mem (struct memory_manager *memory_manager, const struct run_register_set *reg_set,
                      struct inst_param inst_param, uint32_t *value, size_t bytes, bool jump_op);
static bool store_mem (struct memory_manager *memory_manager, const struct run_register_set *reg_set,
                   struct inst_param inst_param, uint32_t value, size_t bytes);

static bool load_reg (struct run_register_set *reg_set, int reg_no, uint32_t *reg_val);
static bool store_reg (struct run_register_set *reg_set, int reg_no, uint32_t reg_val);

int run (struct memory_manager *memory_manager, const struct debug_manager *debug_manager,
         struct run_register_set *reg_set)
{
  // 1. PC로 부터 opcode를 읽는다.
  // 2. opcode마다 추가적으로 0~3바이트를 더 읽고,
  //    - opcode -> instruction format number table 작성
  // 3. debug_manager에게 bp check를 요청한다.
  // 3-1. bp에 걸릴 경우, run을 멈춤.(반환)
  // 3-2. bp에 안걸릴 경우 계속 진행.
  // 4. instruction을 수행한다.
  //    - opcode -> instruction handler table 작성.
  // 5. 다시 1부터 반복.

  enum opcode_format opcode_format_table[256] =
      {
          [0x00] = OPCODE_FORMAT_3_4,
          [0x68] = OPCODE_FORMAT_3_4,
          [0x74] = OPCODE_FORMAT_3_4,
          [0x04] = OPCODE_FORMAT_3_4,
          [0x50] = OPCODE_FORMAT_3_4,
          [0x0C] = OPCODE_FORMAT_3_4,
          [0x14] = OPCODE_FORMAT_3_4,
          [0x10] = OPCODE_FORMAT_3_4,
          [0x54] = OPCODE_FORMAT_3_4,
          [0x48] = OPCODE_FORMAT_3_4,
          [0x30] = OPCODE_FORMAT_3_4,
          [0x34] = OPCODE_FORMAT_3_4,
          [0x38] = OPCODE_FORMAT_3_4,
          [0x3C] = OPCODE_FORMAT_3_4,
          [0x28] = OPCODE_FORMAT_3_4,
          [0xE0] = OPCODE_FORMAT_3_4,
          [0xD8] = OPCODE_FORMAT_3_4,
          [0x4C] = OPCODE_FORMAT_3_4,
          [0xDC] = OPCODE_FORMAT_3_4,
          [0xB4] = OPCODE_FORMAT_2,
          [0xA0] = OPCODE_FORMAT_2,
          [0xB8] = OPCODE_FORMAT_2
      };
  inst_handler_t inst_handler_table[256] =
      {
          [0x00] = _h_LDA,
          [0x68] = _h_LDB,
          [0x74] = _h_LDT,
          [0x04] = _h_LDX,
          [0x50] = _h_LDCH,
          [0x0C] = _h_STA,
          [0x14] = _h_STL,
          [0x10] = _h_STX,
          [0x54] = _h_STCH,
          [0x48] = _h_JSUB,
          [0x30] = _h_JEQ,
          [0x34] = _h_JGT,
          [0x38] = _h_JLT,
          [0x3C] = _h_J,
          [0x28] = _h_COMP,
          [0xE0] = _h_TD,
          [0xD8] = _h_RD,
          [0x4C] = _h_RSUB,
          [0xDC] = _h_WD,
          [0xB4] = _h_CLEAR,
          [0xA0] = _h_COMPR,
          [0xB8] = _h_TIXR
      };

  reg_set->L = 0x00FFFFFF;
  while (reg_set->PC != 0x00FFFFFF)
    {
      uint8_t opcode;
      uint32_t inst_val = 0;
      uint8_t mem_val;
      int inst_size;

      memory_get (memory_manager, reg_set->PC, &mem_val);
      inst_val = mem_val;
      opcode = mem_val & 0xFC;

      //fprintf (stderr, "[DEBUG] running opcode %02X, PC = %08X, SW = %08X\n", mem_val, reg_set->PC, reg_set->SW);

      switch (opcode_format_table[opcode])
        {
        case OPCODE_FORMAT_1:
          inst_size = 1;
          break;
        case OPCODE_FORMAT_2:
          memory_get (memory_manager, reg_set->PC + 1, &mem_val);
          inst_size = 2;
          inst_val = (inst_val << 8) + mem_val;
          break;
        case OPCODE_FORMAT_3_4:
          memory_get (memory_manager, reg_set->PC + 1, &mem_val);
          inst_val = (inst_val << 8) + mem_val;
          memory_get (memory_manager, reg_set->PC + 2, &mem_val);
          inst_val = (inst_val << 8) + mem_val;
          inst_size = 3;
          if (inst_val & (1 << 12))
            {
              memory_get (memory_manager, reg_set->PC + 3, &mem_val);
              inst_val = (inst_val << 8) + mem_val;
              ++inst_size;
            }
          break;
        default:
          return -1;
        }

      if (debug_bp_check (debug_manager, reg_set->PC, reg_set->PC + inst_size))
        {
          // bp에 걸림.
          printf ("BP!\n");
          return 0;
        }

      struct inst_param inst_param = {
          .extend = (inst_size == 4),
          .param.val = inst_val
      };

      inst_handler_t inst_handler = inst_handler_table[opcode];
      if (!inst_handler)
        {
          fprintf (stderr, "[ERROR] Not supported opcode : %02X\n", opcode);
          return -1;
        }

      reg_set->PC += inst_size;
      inst_handler (memory_manager, inst_param, reg_set);

      /*
      fprintf (stderr,
          "A : %08X  X : %08X \n"
              "L : %08X  PC: %08X \n"
              "B : %08X  S : %08X \n"
              "T : %08X           \n",
          reg_set->A, reg_set->X,
          reg_set->L, reg_set->PC,
          reg_set->B, reg_set->S,
          reg_set->T
      );
       */

      //getchar();
    }

  return 0;
}

// TODO: range check.
static bool load_mem (struct memory_manager *memory_manager, const struct run_register_set *reg_set,
                  struct inst_param inst_param, uint32_t *value, size_t bytes, bool jump_op)
{
  uint32_t target_address;

#define INST_PARAM(S) \
  (inst_param.extend ? inst_param.param.f4_param.S : inst_param.param.f3_param.S)

  if (INST_PARAM (b) == 1 && INST_PARAM (p) == 0)  // Base relative
    target_address = INST_PARAM (address) + reg_set->B;
  else if (INST_PARAM (b) == 0 && INST_PARAM (p) == 1)  // PC relative
    {
      int32_t value;
      uint32_t boundary;

      if (inst_param.extend)
        boundary = (1 << 19);
      else
        boundary = (1 << 11);

      if (INST_PARAM (address) >= boundary)
        value = INST_PARAM (address) - (boundary << 1);
      else
        value = INST_PARAM (address);

      target_address = reg_set->PC + value;
    }
  else
    target_address = INST_PARAM (address);

  if (INST_PARAM (x) == 1)
    {
      target_address += reg_set->X;
    }

  // fprintf (stderr, "[DEBUG - load_mem] address = %08X, target_address = %08X\n", INST_PARAM (address), target_address);

  bool is_immediate, is_simple, is_indirect;
  if (jump_op)
    {
      is_immediate = INST_PARAM (n) == 1 && INST_PARAM (i) == 1;
      is_simple = INST_PARAM (n) == 1 && INST_PARAM (i) == 0;
      is_indirect = false;
    }
  else
    {
      is_immediate = INST_PARAM (n) == 0 && INST_PARAM (i) == 1;
      is_simple = INST_PARAM (n) == 1 && INST_PARAM (i) == 1;
      is_indirect = INST_PARAM (n) == 1 && INST_PARAM (i) == 0;
    }

  if (is_immediate)  // immediate
    {
      //fprintf (stderr, "[DEBUG - load_mem - immediate]\n");
      *value = target_address;
    }
  else if (is_simple)  // simple
    {
      //fprintf (stderr, "[DEBUG - load_mem - simple]\n");
      uint8_t mem_val;
      *value = 0;
      for (int i = 0; i < bytes; ++i)
        {
          memory_get (memory_manager, target_address + i, &mem_val);
          *value = (*value << 8) + mem_val;
        }
    }
  else if (is_indirect)  // indirect
    {
      //fprintf (stderr, "[DEBUG - load_mem - indirect]\n");
      uint32_t address = 0;
      uint8_t mem_val;

      for (int i = 0; i < 3; ++i)
        {
          memory_get (memory_manager, target_address + i, &mem_val);
          address = (address << 8) + mem_val;
        }

      //fprintf (stderr, "[DEBUG - load_mem - indirect] final address = %08X\n", address);

      *value = 0;
      for (int i = 0; i < bytes; ++i)
        {
          memory_get (memory_manager, address + i, &mem_val);
          *value = (*value << 8) + mem_val;
        }
    }
  else
    {
      return false;
    }

#undef CONTROL_INST

  return true;
}

// TODO: range check.
static bool store_mem (struct memory_manager *memory_manager, const struct run_register_set *reg_set,
                   struct inst_param inst_param, uint32_t value, size_t bytes)
{
  uint32_t target_address;

#define INST_PARAM(S) \
  (inst_param.extend ? inst_param.param.f4_param.S : inst_param.param.f3_param.S)

  if (INST_PARAM (b) == 1 && INST_PARAM (p) == 0)  // Base relative
    target_address = INST_PARAM (address) + reg_set->B;
  else if (INST_PARAM (b) == 0 && INST_PARAM (p) == 1)  // PC relative
    {
      int32_t value;
      uint32_t boundary;

      if (inst_param.extend)
        boundary = (1 << 19);
      else
        boundary = (1 << 11);

      if (INST_PARAM (address) >= boundary)
        value = INST_PARAM (address) - (boundary << 1);
      else
        value = INST_PARAM (address);

      target_address = reg_set->PC + value;
    }
  else
    target_address = INST_PARAM (address);

  if (INST_PARAM (x) == 1)
    {
      target_address += reg_set->X;
    }

  // fprintf (stderr, "[DEBUG - store_mem] address = %08X, target_address = %08X\n", INST_PARAM (address), target_address);

  if (INST_PARAM (n) == 0 && INST_PARAM (i) == 1)  // immediate
    {
      // WTF?
      return false;
    }
  else if (INST_PARAM (n) == 1 && INST_PARAM (i) == 1)  // simple
    {
      for (int i = bytes-1; i >= 0; --i)
        {
          memory_edit (memory_manager, target_address + i, value);
          value >>= 8;
        }
    }
  else if (INST_PARAM (n) == 1 && INST_PARAM (i) == 0)  // indirect
    {
      uint8_t mem_val;
      uint32_t address = 0;
      for (int i = 0; i < 3; ++i)
        {
          memory_get (memory_manager, target_address + i, &mem_val);
          address = (address << 8) + mem_val;
        }
      for (int i = 2; i >= 0; --i)
        {
          memory_edit (memory_manager, address + i, value);
          value >>= 8;
        }
    }
  else
    {
      return false;
    }

#undef CONTROL_INST

  return true;
}

static bool load_reg (struct run_register_set *reg_set, int reg_no, uint32_t *reg_val)
{
  switch (reg_no)
    {
      case 0:
        *reg_val = reg_set->A;
      break;
      case 1:
        *reg_val = reg_set->X;
      break;
      case 2:
        *reg_val = reg_set->L;
      break;
      case 3:
        *reg_val = reg_set->B;
      break;
      case 4:
        *reg_val = reg_set->S;
      break;
      case 5:
        *reg_val = reg_set->T;
      break;
      case 6:
        // *reg_val = reg_set->F;
        return false; // Not supported.
      case 8:
        *reg_val = reg_set->PC;
      break;
      case 9:
        *reg_val = reg_set->SW;
      break;
      default:
        return false;  // invalid
    }
  return true;
}

static bool store_reg (struct run_register_set *reg_set, int reg_no, uint32_t reg_val)
{
  switch (reg_no)
    {
      case 0:
        reg_set->A = reg_val;
      break;
      case 1:
        reg_set->X = reg_val;
      break;
      case 2:
        reg_set->L = reg_val;
      break;
      case 3:
        reg_set->B = reg_val;
      break;
      case 4:
        reg_set->S = reg_val;
      break;
      case 5:
        reg_set->T = reg_val;
      break;
      case 6:
        // reg_set->F = reg_val;
        return false; // Not supported.
      case 8:
        reg_set->PC = reg_val;
      break;
      case 9:
        reg_set->SW = reg_val;
      break;
      default:
        return false;
    }
  return true;
}

static void _h_STA (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  store_mem (memory_manager, reg_set, inst_param, reg_set->A, 3);
}
static void _h_STL (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  store_mem (memory_manager, reg_set, inst_param, reg_set->L, 3);
}
static void _h_STCH (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  store_mem (memory_manager, reg_set, inst_param, reg_set->A & 0xFF, 1);
}
static void _h_STX (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  store_mem (memory_manager, reg_set, inst_param, reg_set->X, 3);
}
static void _h_LDA (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  load_mem (memory_manager, reg_set, inst_param, &reg_set->A, 3, false);
}
static void _h_LDB (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  load_mem (memory_manager, reg_set, inst_param, &reg_set->B, 3, false);
}
static void _h_LDT (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  load_mem (memory_manager, reg_set, inst_param, &reg_set->T, 3, false);
}
static void _h_LDX (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  load_mem (memory_manager, reg_set, inst_param, &reg_set->X, 3, false);
}
static void _h_LDCH (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  uint32_t value;
  load_mem (memory_manager, reg_set, inst_param, &value, 1, false);
  reg_set->A = (reg_set->A & 0xFFFFFF00) + (value & 0xFF);
}
static void _h_JSUB (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  uint32_t jump_address;
  load_mem (memory_manager, reg_set, inst_param, &jump_address, 3, true);
  // fprintf (stderr, "[DEBUG - JSUB] jump_address = %08X\n", jump_address);

  reg_set->L = reg_set->PC;
  reg_set->PC = jump_address;
}
static void _h_JEQ (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  if (reg_set->SW == 0)
    {
      load_mem (memory_manager, reg_set, inst_param, &reg_set->PC, 3, true);
      // fprintf (stderr, "[DEBUG - JEQ] jump_address = %08X\n", reg_set->PC);
    }
}
static void _h_J (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  load_mem (memory_manager, reg_set, inst_param, &reg_set->PC, 3, true);
}
static void _h_JLT (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  if ((int32_t) reg_set->SW < 0)
    {
      load_mem (memory_manager, reg_set, inst_param, &reg_set->PC, 3, true);
      // fprintf (stderr, "[DEBUG - JLT] jump_address = %08X\n", reg_set->PC);
    }
}
static void _h_JGT (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  if ((int32_t) reg_set->SW > 0)
    load_mem (memory_manager, reg_set, inst_param, &reg_set->PC, 3, true);
}
static void _h_RSUB (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  reg_set->PC = reg_set->L;
}
static void _h_COMP (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  uint32_t value;
  load_mem (memory_manager, reg_set, inst_param, &value, 3, false);
  if (reg_set->A > value)
    reg_set->SW = 1;
  else if (reg_set->A < value)
    reg_set->SW = -1;
  else
    reg_set->SW = 0;
}
static void _h_COMPR (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  uint32_t reg_val_1, reg_val_2;
  load_reg (reg_set, inst_param.param.f2_param.r1, &reg_val_1);
  load_reg (reg_set, inst_param.param.f2_param.r2, &reg_val_2);
  if (reg_val_1 > reg_val_2)
    reg_set->SW = 1;
  else if (reg_val_1 < reg_val_2)
    reg_set->SW = -1;
  else
    reg_set->SW = 0;
}
static void _h_TD (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  /* I/O instructions are not supported.
   * This behavior is just for test */
  // fprintf (stderr, "[DEBUG] TD\n");
  reg_set->SW = 1;
}
static void _h_RD (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  /* I/O instructions are not supported.
   * This behavior is just for test */
  static char device_input_mock[] = "HELLO WORLD\0I'M TAEGUK\0";
  static int device_input_idx = 0;

  reg_set->A = (reg_set->A & 0xFFFFFF00) + (uint8_t) device_input_mock[device_input_idx++];

  fprintf (stdout, "--- [Input Device] input = '%c'\n", (char)(reg_set->A & 0xFF));

  if (device_input_idx >= sizeof(device_input_mock)/sizeof(char))
    device_input_idx = 0;
}
static void _h_WD (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  /* I/O instructions are not supported.
   * This behavior is just for test */
  fprintf (stdout, "+++ [Output Device] output = '%c'\n", (char)(reg_set->A & 0xFF));
}
static void _h_CLEAR (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  store_reg (reg_set, inst_param.param.f2_param.r1, 0);
}
static void _h_TIXR (struct memory_manager *memory_manager, struct inst_param inst_param, struct run_register_set *reg_set)
{
  uint32_t reg_val;
  load_reg (reg_set, inst_param.param.f2_param.r1, &reg_val);
  ++reg_set->X;

  if (reg_set->X > reg_val)
    reg_set->SW = 1;
  else if (reg_set->X < reg_val)
    reg_set->SW = -1;
  else
    reg_set->SW = 0;
}