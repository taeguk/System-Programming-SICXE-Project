#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include "assemble.h"

#define ASSEMBLE_STATEMENT_TOKEN_MAX_NUM 8
#define ASSEMBLE_STATEMENT_MAX_LEN 100 

#define SYMBOL_PART_MAX_LEN 7
#define MNEMONIC_PART_MAX_LEN 7

#define TEXT_RECORD_MAX_BYTES_PER_ROW 30

#define MAX_ASM_FILENAME_LEN 280

#define MAX_MOD_REC_NUM 1000
#define MAX_PROGRAM_NAME_LEN 10
#define MAX_REC_HEAD_LEN 30
#define MAX_OBJ_BUF_LEN 1000
#define MAX_BYTE_BUF_LEN 1000

/* ASM 파일에서 한 줄 (statement)의 정보를 저장하는 구조체 */
struct statement
  {
    bool is_comment;
    const char *symbol;
    const struct opcode *opcode;
    bool extend;
    char *input; // statement 문자열을 그대로 간직하고 있는 pointer.

    /* tokens for operand */
    size_t token_cnt;
    char *token_list[ASSEMBLE_STATEMENT_TOKEN_MAX_NUM+1];
  };

union instruction_format_1
  {
    struct
      {
        uint8_t opcode : 8;
      } bit_field;
    uint8_t val;
  };

union instruction_format_2
  {
    struct
      {
        uint16_t r2     : 4;
        uint16_t r1     : 4;
        uint16_t opcode : 8;
      } bit_field;
    uint16_t val;
  };

union instruction_format_3
  {
    struct
      {
        uint32_t disp   : 12;
        uint32_t e      : 1;  // must be 0
        uint32_t p      : 1;
        uint32_t b      : 1;
        uint32_t x      : 1;
        uint32_t i      : 1;
        uint32_t n      : 1;
        uint32_t opcode : 6;
      } bit_field;
    uint32_t val;
  };

union instruction_format_4
  {
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
      } bit_field;
    uint32_t val;
  };

static int assemble_pass_1 (const char *asm_file, const char *mid_file, 
                            const struct opcode_manager *opcode_manager,
                            struct symbol_manager *symbol_manager);

static int assemble_pass_2 (const char *mid_file, const char *lst_file, const char *obj_file,
                            const struct opcode_manager *opcode_manager,
                            const struct symbol_manager *symbol_manager);

int assemble (const char *filename, const struct opcode_manager *opcode_manager,
              struct symbol_manager *symbol_manager)
{
  /*
   * 1. filename의 확장자가 .asm 인지 체크
   * 2. listing file과 outputfile 열기.
   */
  char name[MAX_ASM_FILENAME_LEN+1], mid_file[MAX_ASM_FILENAME_LEN+1], 
       lst_file[MAX_ASM_FILENAME_LEN+1], obj_file[MAX_ASM_FILENAME_LEN+1];
  
  strncpy (name, filename, MAX_ASM_FILENAME_LEN);
  
  char *dot = strrchr (name, '.');
  if (dot == NULL)
    {
      fprintf (stderr, "[ERROR] Invalid file: %s\n", filename);
      return -1;
    }

  *dot = '\0';
  snprintf (mid_file, MAX_ASM_FILENAME_LEN, "%s.mid", name);
  snprintf (lst_file, MAX_ASM_FILENAME_LEN, "%s.lst", name);
  snprintf (obj_file, MAX_ASM_FILENAME_LEN, "%s.obj", name);
  
  fprintf (stdout, "[Progress] Now, start pass 1.\n");
  if (assemble_pass_1 (filename, mid_file, opcode_manager, symbol_manager) != 0)
    {
      unlink (mid_file);
      return -1;
    }
  fprintf (stdout, "[Progress] Now, start pass 2.\n");
  if (assemble_pass_2 (mid_file, lst_file, obj_file, opcode_manager, symbol_manager) != 0)
    {
      unlink (mid_file);
      unlink (lst_file);
      unlink (obj_file);
      return -1;
    }
  unlink (mid_file);
  fprintf (stdout, "[Progress] Successfully finish to assemble.\n");
  return 0;
}

static int fetch_statement (FILE *fp, const struct opcode_manager *opcode_manager,
                            struct statement *statement, 
                            bool is_mid_file, uint32_t *LOCCTR, uint32_t *statement_size)
{
  static char input[ASSEMBLE_STATEMENT_MAX_LEN];
  static char input_token[ASSEMBLE_STATEMENT_MAX_LEN];

  if (fgets (input, ASSEMBLE_STATEMENT_MAX_LEN, fp) == NULL)
    return 0;

  int len = strlen (input);
  if (input[len-1] != '\n')
    return -1;

  input[len-1] = '\0';

  if (is_mid_file)
    {
      int offset, i;
      sscanf (input, "%X\t%X%n", LOCCTR, statement_size, &offset);
      for (i = 0; input[i + offset]; ++i)
        input[i] = input[i + offset];
      input[i] = 0;
    }
  strncpy (input_token, input, ASSEMBLE_STATEMENT_MAX_LEN);
  
  statement->input = input;
  statement->token_cnt = 0;
  statement->token_list[statement->token_cnt] = strtok (input_token, " ,\t\n");

  while (statement->token_cnt <= ASSEMBLE_STATEMENT_TOKEN_MAX_NUM && 
         statement->token_list[statement->token_cnt])
    statement->token_list[++statement->token_cnt] = strtok (NULL, " ,\t\n");

  if (statement->token_cnt <= 0)
    return -1;

  if (statement->token_cnt > ASSEMBLE_STATEMENT_TOKEN_MAX_NUM)
    return -1;

  // need to be refactored.
  // 주석일 경우,
  if (statement->token_list[0][0] == '.')
    {
      statement->is_comment = true;
      statement->opcode = NULL;
    }
  // 주석이 아닌 경우,
  else
    {
      statement->is_comment = false;
      const char *opcode_token;
      int operand_token_offset;

      if (statement->token_list[0][0] == '+')
        {
          statement->extend = true;
          opcode_token = &statement->token_list[0][1];
        }
      else
        {
          statement->extend = false;
          opcode_token = statement->token_list[0];
        }

      const struct opcode *opcode = opcode_find (opcode_manager, opcode_token);

      /* 첫 번째 token을 symbol로 인식할 것이냐 opcode로 인식할 것이냐의 문제가 있다.
       * 나는 이를 해결하기 위해서 opcode table를 검색하여 valid할 경우, opcode로 아닐 경우
       * symbol로 인식하는 방법을 사용하였다.
       */

      // Symbol이 없는 경우,
      if (opcode)
        {
          operand_token_offset = 1;
          statement->symbol = NULL;
          statement->opcode = opcode;
        }
      // Symbol이 있는 경우,
      else
        {
          if (statement->token_cnt < 2)
            return -1;/*error*/

          if (statement->token_list[1][0] == '+')
            {
              statement->extend = true;
              opcode_token = &statement->token_list[1][1];
            }
          else
            {
              statement->extend = false;
              opcode_token = statement->token_list[1];
            }

          opcode = opcode_find (opcode_manager, opcode_token);
          if (opcode == NULL)
            return -1;/*error*/

          operand_token_offset = 2;
          statement->symbol = statement->token_list[0];
          statement->opcode = opcode;
        }

      // token list에 있는 symbol과 opcode의 token을 지움.
      for (size_t i = operand_token_offset; i < statement->token_cnt; ++i)
        statement->token_list[i - operand_token_offset] = statement->token_list[i];
      statement->token_cnt -= operand_token_offset;
    }

  return 0;
}

static int assemble_pass_1 (const char *asm_file, const char *mid_file, 
                            const struct opcode_manager *opcode_manager,
                            struct symbol_manager *symbol_manager)
{
  FILE *asm_fp = fopen (asm_file, "rt");
  FILE *mid_fp = fopen (mid_file, "wt");
  int ret;

  if (!asm_fp || !mid_fp)
    {
      // result에 에러정보 할당
      ret = -1;
      goto ERROR;
    }

  struct statement statement;
  uint32_t LOCCTR = 0;
  int line_no = 5;

  if (fetch_statement (asm_fp, opcode_manager, &statement, false, NULL, NULL) != 0)
    {
      /* handling error */
      ret = -1;
      goto ERROR;
    }

  if (!statement.is_comment && statement.opcode->op_format == OPCODE_START)
    {
      LOCCTR = strtol (statement.token_list[0], NULL, 16);
      fprintf (mid_fp, "%04X\t0\t%s\n", LOCCTR, statement.input);

      if (fetch_statement (asm_fp, opcode_manager, &statement, false, NULL, NULL) != 0)
        {
          /* handling error */
          ret = -1;
          goto ERROR;
        }

      line_no += 5;
    }

  while (true)
    {
      /* process */

      if (statement.is_comment)
        {
          fprintf (mid_fp, "%04X\t0\t%s\n", LOCCTR, statement.input);
        }
      else
        {
          size_t old_LOCCTR = LOCCTR;

          // symbol을 symbol table에 넣음.
          if (statement.symbol)
            {
              // 이미 존재하는 symbol일 경우,,, error.
              if (symbol_find (symbol_manager, statement.symbol))
                {
                  ret = -1;
                  goto ERROR;
                }

              struct symbol symbol;
              strncpy (symbol.label, statement.symbol, SYMBOL_NAME_MAX_LEN);
              symbol.LOCCTR = LOCCTR;
              symbol_insert (symbol_manager, &symbol);
            }

          if (statement.opcode->op_format == OPCODE_FORMAT_1)
            {
              LOCCTR += 1;
            }
          else if (statement.opcode->op_format == OPCODE_FORMAT_2)
            {
              LOCCTR += 2;
            }
          else if (statement.opcode->op_format == OPCODE_FORMAT_3_4)
            {
              LOCCTR += 3;
            }
          else if (statement.opcode->op_format == OPCODE_BYTE)
            {
              if (statement.token_cnt != 1)
                {
                  ret = -1;
                  goto ERROR;
                }

              const char *operand = statement.token_list[0];
              int len, bytes;

              if (operand[1] != '\'')
                {
                  ret = -1;
                  goto ERROR;
                }

              len = strlen (operand);

              if (operand[0] == 'C')
                {
                  bytes = len - 3;
                }
              else if (operand[0] == 'X')
                {
                  bytes = (len - 3) / 2;
                }
              else
                {
                  ret = -1;
                  goto ERROR;
                }

              if (operand[len-1] != '\'')
                {
                  ret = -1;
                  goto ERROR;
                }

              LOCCTR += bytes;
            }
          else if (statement.opcode->op_format == OPCODE_WORD)
            {
              if (statement.token_cnt != 1)
                {
                  ret = -1;
                  goto ERROR;
                }
              LOCCTR += 3;
            }
          else if (statement.opcode->op_format == OPCODE_RESB)
            {
              if (statement.token_cnt != 1)
                {
                  ret = -1;
                  goto ERROR;
                }
              int cnt = strtol (statement.token_list[0], NULL, 10);
              LOCCTR += cnt;
            }
          else if (statement.opcode->op_format == OPCODE_RESW)
            {
              if (statement.token_cnt != 1)
                {
                  ret = -1;
                  goto ERROR;
                }
              int cnt = strtol (statement.token_list[0], NULL, 10);
              LOCCTR += cnt * 3;
            }
          else
            {
              // no variation of LOCCTR.
            }

          if (statement.extend)
            {
              if (statement.opcode->op_format == OPCODE_FORMAT_3_4)
                ++LOCCTR;
              else
                {
                  ret = -1;
                  goto ERROR;
                }
            }
          
          fprintf (mid_fp, "%04X\t%X\t%s\n", (unsigned int) old_LOCCTR,
                   (unsigned int )(LOCCTR-old_LOCCTR), statement.input);
        }

      if (feof (asm_fp) != 0)
        break;
      else if (!statement.is_comment && statement.opcode->op_format == OPCODE_END)
        break;

      if (fetch_statement (asm_fp, opcode_manager, &statement, false, NULL, NULL) != 0)
        {
          /* handling error */
          ret = -1;
          goto ERROR;
        }

      line_no += 5;
    }
    
  ret = 0;
  goto END;

ERROR:
  fprintf (stderr, "[ERROR] Line no %d: an error occurs in step 1.\n", line_no);
END:
  if (asm_fp)
    fclose (asm_fp);
  if (mid_fp)
    fclose (mid_fp);

  return ret;
}

static int convert_register_mnemonic_to_no (const char *register_mnemonic)
{
#define COMPARE_WITH(STR) \
  (strcmp (register_mnemonic, (STR)) == 0)

  if (COMPARE_WITH("A"))        return 0;
  else if (COMPARE_WITH("X"))   return 1;
  else if (COMPARE_WITH("L"))   return 2;
  else if (COMPARE_WITH("PL"))  return 8;
  else if (COMPARE_WITH("SW"))  return 9;
  else if (COMPARE_WITH("B"))   return 3;
  else if (COMPARE_WITH("S"))   return 4;
  else if (COMPARE_WITH("T"))   return 5;
  else if (COMPARE_WITH("F"))   return 6;
  else                          return -1;

#undef COMPARE_WITH
}

static int assemble_pass_2 (const char *mid_file, const char *lst_file, const char *obj_file, 
                            const struct opcode_manager *opcode_manager,
                            const struct symbol_manager *symbol_manager)
{
  FILE *mid_fp = fopen (mid_file, "rt");
  FILE *lst_fp = fopen (lst_file, "wt");
  FILE *obj_fp = fopen (obj_file, "wt");
  int ret;

  if (!mid_fp || !lst_fp || !obj_fp)
    {
      // result에 에러정보 할당
      ret = -1;
      goto ERROR;
    }

  int line_no = 5;
  struct statement statement;
  uint32_t start_LOCCTR, row_LOCCTR, LOCCTR;
  uint32_t statement_size;
  uint32_t object_code;
  bool exist_base = false;
  uint32_t base;
  uint32_t mod_LOCCTR_list[MAX_MOD_REC_NUM];
  int mod_LOCCTR_cnt = 0;
  char program_name[MAX_PROGRAM_NAME_LEN+1] = {0,};
  char byte_buf[MAX_BYTE_BUF_LEN+1];
  char obj_buf[MAX_OBJ_BUF_LEN+1], rec_head[MAX_REC_HEAD_LEN+1];
  
  obj_buf[0] = '\0';

#define VERIFY_TEXT_RECORD_MAX_BYTES(bytes) \
  if (LOCCTR + bytes > row_LOCCTR + TEXT_RECORD_MAX_BYTES_PER_ROW) \
    { \
      snprintf (rec_head, MAX_REC_HEAD_LEN, "T%06X%02X", row_LOCCTR, (uint8_t) strlen (obj_buf) / 2); \
      fprintf (obj_fp, "%s%s\n", rec_head, obj_buf); \
      row_LOCCTR = LOCCTR; \
      obj_buf[0] = '\0'; \
    }

  if (fetch_statement (mid_fp, opcode_manager, &statement, true, &LOCCTR, &statement_size) != 0)
    {
      /* handling error */
      ret = -1;
      goto ERROR;
    }

  start_LOCCTR = row_LOCCTR = LOCCTR;

  if (!statement.is_comment && statement.opcode->op_format == OPCODE_START)
    {
      if (statement.symbol)
        strncpy (program_name, statement.symbol, MAX_PROGRAM_NAME_LEN);

      fprintf (lst_fp, "%d\t%04X%s\n", line_no, LOCCTR, statement.input);
      fprintf (obj_fp, "%19s\n", "");

      if (fetch_statement (mid_fp, opcode_manager, &statement, true, &LOCCTR, &statement_size) != 0)
        {
          /* handling error */
          ret = -1;
          goto ERROR;
        }

      line_no += 5;
    }
  while (true)
    {
      /* process */

      if (statement.is_comment)
        {
          fprintf (lst_fp, "%d\t%s\n", line_no, statement.input);
        }
      else
        {
          /***************** Format 1의 Instruction의 경우 ****************/
          if (statement.opcode->op_format == OPCODE_FORMAT_1)
            {
              if (statement.token_cnt != 0)
                {
                  ret = -1;
                  goto ERROR;
                }
              object_code = statement.opcode->val;
            }
          /***************** Format 2의 Instruction의 경우 ****************/
          else if (statement.opcode->op_format == OPCODE_FORMAT_2)
            {
              union instruction_format_2 instruction;

              switch (statement.opcode->detail_format)
                {
                case OPCODE_FORMAT_2_GENERAL:
                  {
                    if (statement.token_cnt != 2)
                      {
                        ret = -1;
                        goto ERROR;
                      }
                    int reg_no_1, reg_no_2;
                    reg_no_1 = convert_register_mnemonic_to_no (statement.token_list[0]);
                    reg_no_2 = convert_register_mnemonic_to_no (statement.token_list[1]);
                    if (reg_no_1 == -1 || reg_no_2 == -1)
                      {
                        ret = -1;
                        goto ERROR;
                      }
                    instruction.bit_field.opcode = statement.opcode->val;
                    instruction.bit_field.r1 = reg_no_1;
                    instruction.bit_field.r2 = reg_no_2;
                  }
                  break;
                case OPCODE_FORMAT_2_ONE_REGISTER:
                  {
                    if (statement.token_cnt != 1)
                      {
                        ret = -1;
                        goto ERROR;
                      }
                    int reg_no = convert_register_mnemonic_to_no (statement.token_list[0]);
                    if (reg_no == -1)
                      {
                        ret = -1;
                        goto ERROR;
                      }
                    instruction.bit_field.opcode = statement.opcode->val;
                    instruction.bit_field.r1 = reg_no;
                    instruction.bit_field.r2 = 0;
                  }
                  break;
                case OPCODE_FORMAT_2_REGISTER_N:
                  {
                    if (statement.token_cnt != 2)
                      {
                        ret = -1;
                        goto ERROR;
                      }
                    int reg_no = convert_register_mnemonic_to_no (statement.token_list[0]);
                    char *endptr;
                    long int n = strtol (statement.token_list[1], &endptr, 16);
                    if (reg_no == -1 || *endptr != '\0' || n > 0xF || n < 0)
                      {
                        ret = -1;
                        goto ERROR;
                      }
                    instruction.bit_field.opcode = statement.opcode->val;
                    instruction.bit_field.r1 = reg_no;
                    instruction.bit_field.r2 = n;
                  }
                  break;
                case OPCODE_FORMAT_2_ONE_N:
                  {
                    if (statement.token_cnt != 1)
                      {
                        ret = -1;
                        goto ERROR;
                      }
                    char *endptr;
                    long int n = strtol (statement.token_list[0], &endptr, 16);
                    if (*endptr != '\0' || n > 0xF || n < 0)
                      {
                        ret = -1;
                        goto ERROR;
                      }
                    instruction.bit_field.opcode = statement.opcode->val;
                    instruction.bit_field.r1 = n;
                    instruction.bit_field.r2 = 0;
                  }
                  break;
                default:
                  /* Can't reach here */
                  assert (false);
                }
              object_code = instruction.val;
            }
          /***************** Format 3의 Instruction의 경우 ****************/
          else if (statement.opcode->op_format == OPCODE_FORMAT_3_4)
            {
              union instruction_format_3 instruction_for_3; instruction_for_3.val = 0;
              union instruction_format_4 instruction_for_4;

#define CONTROL_INST(S) \
              if (statement.extend) instruction_for_4.S; \
              else                  instruction_for_3.S;

              CONTROL_INST (bit_field.opcode = (statement.opcode->val >> 2));
              CONTROL_INST (bit_field.e = statement.extend);

              if (statement.opcode->detail_format == OPCODE_FORMAT_3_4_NO_OPERAND)
                {
                  if (statement.token_cnt != 0)
                    {
                      ret = -1;
                      goto ERROR;
                    }

                  CONTROL_INST (bit_field.n = 1);
                  CONTROL_INST (bit_field.i = 1);
                  CONTROL_INST (bit_field.x = 0);
                  CONTROL_INST (bit_field.b = 0);
                  CONTROL_INST (bit_field.p = 0);
                  if (statement.extend)
                    instruction_for_4.bit_field.address = 0;
                  else
                    instruction_for_3.bit_field.disp = 0;
                }
              else // OPCODE_FORMAT_3_4_GENERAL
                {
                  if (statement.token_cnt > 2 || statement.token_cnt < 1)
                    {
                      ret = -1;
                      goto ERROR;
                    }

                  /* Index 모드 처리. */
                  if (statement.token_cnt == 2)
                    {
                      if (strcmp (statement.token_list[1], "X") != 0)
                        {
                          ret = -1;
                          goto ERROR;
                        }
                      CONTROL_INST (bit_field.x = 1);
                    }
                  else
                    {
                      CONTROL_INST (bit_field.x = 0);
                    }

                  const char *operand = statement.token_list[0];

                  /* Addressing 모드 처리 */

                  bool operand_is_constant = false;

                  // Immediate addressing
                  if (operand[0] == '#')
                    {
                      CONTROL_INST(bit_field.n = 0);
                      CONTROL_INST(bit_field.i = 1);
                      if ('0' <= operand[1] && operand[1] <= '9')
                        operand_is_constant = true;
                      ++operand;
                    }
                  // Indirect addressing
                  else if (operand[0] == '@')
                    {
                      CONTROL_INST(bit_field.n = 1);
                      CONTROL_INST(bit_field.i = 0);
                      ++operand;
                    }
                  // simple addressing
                  else
                    {
                      CONTROL_INST(bit_field.n = 1);
                      CONTROL_INST(bit_field.i = 1);
                    }
                  
                  uint32_t operand_value;

                  if (operand_is_constant)
                    {
                      operand_value = strtol (operand, NULL, 10);
                    }
                  else
                    {
                      const struct symbol *symbol = symbol_find (symbol_manager, operand);
                      if (!symbol)
                        {
                          ret = -1;
                          goto ERROR;
                        }
                      operand_value = symbol->LOCCTR;
                    }

                  if (statement.extend)
                    {
                      instruction_for_4.bit_field.b = 0;
                      instruction_for_4.bit_field.p = 0;
                      instruction_for_4.bit_field.address = operand_value;
                      if (!operand_is_constant)
                        mod_LOCCTR_list[mod_LOCCTR_cnt++] = LOCCTR+1;
                    }
                  else if (operand_is_constant)
                    {
                      instruction_for_3.bit_field.b = 0;
                      instruction_for_3.bit_field.p = 0;
                      instruction_for_3.bit_field.disp = operand_value;
                    }
                  else
                    {
                      /* Displacement 계산 */
                      int32_t disp;
                      
                      /* 먼저 PC relative가 가능한 지 확인 */
                      const size_t PC = LOCCTR + statement_size;

                      disp = operand_value - PC;

                      // PC relative가 가능한 경우
                      if (-(1 << 11) <= disp && disp < (1 << 11))
                        {
                          instruction_for_3.bit_field.b = 0;
                          instruction_for_3.bit_field.p = 1;
                          instruction_for_3.bit_field.disp = disp;
                        }
                      // PC relative가 불가능한 경우
                      else
                        {
                          /* Base relative가 가능한 지 확인 */

                          // Base가 없을 경우, 에러..
                          if (!exist_base)
                            {
                              ret = -1;
                              goto ERROR;
                            }

                          disp = operand_value - base;

                          // Base relative가 가능한 경우
                          if (0 <= disp && disp < (1 << 12))
                            {
                              instruction_for_3.bit_field.b = 1;
                              instruction_for_3.bit_field.p = 0;
                              instruction_for_3.bit_field.disp = disp;
                            }
                          // Base relative가 불가능한 경우
                          else
                            {
                              ret = -1;
                              goto ERROR;
                            }
                        } /* PC Relative 가 불가능한 경우의 scope */
                    } /* Displacement를 계산해야하는 경우의 scope */
                } /* OPCODE_FORMAT_3_4_GENERAL인 경우의 scope */
              
              if (statement.extend)
                object_code = instruction_for_4.val;
              else
                object_code = instruction_for_3.val;

#undef CONTROL_INST
            } /* OPCODE_FORMAT_3_4인 경우의 scope */
          else if (statement.opcode->op_format == OPCODE_BASE)
            {
              if (statement.token_cnt != 1)
                {
                  ret = -1;
                  goto ERROR;
                }
              const struct symbol *symbol = symbol_find (symbol_manager, statement.token_list[0]);
              if (!symbol)
                {
                  ret = -1;
                  goto ERROR;
                }
              exist_base = true;
              base = symbol->LOCCTR;
            }
          else if (statement.opcode->op_format == OPCODE_NOBASE)
            {
              exist_base = false;
            }
          else if (statement.opcode->op_format == OPCODE_BYTE)
            {
              if (statement.token_cnt != 1)
                {
                  ret = -1;
                  goto ERROR;
                } 

              const char *operand = statement.token_list[0];
              int len = strlen (operand);

              if (len > 500)
                {
                  ret = -1;
                  goto ERROR;
                }

              if (operand[0] == 'C')
                {
                  int idx = 0;
                  for (int i = 2; i < len-1; ++i)
                    {
                      unsigned char ch = operand[i];
                      uint8_t val[2] = { ch / 16 , ch % 16 };
                      for (int j = 0; j < 2; ++j, ++idx)
                        {
                          if (/*0 <= val[j] && */val[j] <= 9)
                            byte_buf[idx] = val[j] + '0';
                          else
                            byte_buf[idx] = val[j] - 10 + 'A';
                        }
                    }
                  byte_buf[idx] = '\0';
                }
              else if (operand[0] == 'X')
                {
                  int i;
                  for (i = 2; i < len-1; ++i)
                    byte_buf[i-2] = operand[i];
                  byte_buf[i-2] = '\0';
                }
              else
                {
                  ret = -1;
                  goto ERROR;
                }
            } /* OPCODE_BYTE인 경우의 scope */
          else if (statement.opcode->op_format == OPCODE_WORD)
            {
              if (statement.token_cnt != 1)
                {
                  ret = -1;
                  goto ERROR;
                }
              int32_t val = strtol (statement.token_list[0], NULL, 10);
              object_code = val;
            }
          else
            {
              // nothing to do
            }
          
          const char *format;
          if (statement.opcode->op_format == OPCODE_FORMAT_1)
            {
              format = "%d\t%04X%-30s%02X\n";
              VERIFY_TEXT_RECORD_MAX_BYTES (1);
              sprintf (obj_buf + strlen (obj_buf), "%02X", object_code);
            }
          else if (statement.opcode->op_format == OPCODE_FORMAT_2)
            {
              format = "%d\t%04X%-30s%04X\n";
              VERIFY_TEXT_RECORD_MAX_BYTES (2);
              sprintf (obj_buf + strlen (obj_buf), "%04X", object_code);
            }
          else if (statement.opcode->op_format == OPCODE_FORMAT_3_4)
            {
              if (statement.extend)
                {
                  format = "%d\t%04X%-30s%08X\n";
                  VERIFY_TEXT_RECORD_MAX_BYTES (4);
                  sprintf (obj_buf + strlen (obj_buf), "%08X", object_code);
                }
              else
                {
                  format = "%d\t%04X%-30s%06X\n";
                  VERIFY_TEXT_RECORD_MAX_BYTES (3);
                  sprintf (obj_buf + strlen (obj_buf), "%06X", object_code);
                }
            }
          else if (statement.opcode->op_format == OPCODE_BYTE)
            {
              fprintf (lst_fp, "%d\t%04X%-30s%s\n", line_no, LOCCTR, statement.input, byte_buf);
              format = NULL;
              VERIFY_TEXT_RECORD_MAX_BYTES (strlen (byte_buf));
              sprintf (obj_buf + strlen (obj_buf), "%s", byte_buf);
            }
          else if (statement.opcode->op_format == OPCODE_WORD)
            {
              format = "%d\t%04X%-30s%06X\n";
              VERIFY_TEXT_RECORD_MAX_BYTES (3);
              sprintf (obj_buf + strlen (obj_buf), "%06X", object_code);
            }
          else
            {
              fprintf (lst_fp, "%d\t%s\n", line_no, statement.input);
              format = NULL;
            }

          if (format)
            fprintf (lst_fp, format, line_no, LOCCTR, statement.input, object_code);
        } /* 주석이 아닌 경우의 scope */

      if (feof (mid_fp) != 0)
        break;
      if (!statement.is_comment && statement.opcode->op_format == OPCODE_END)
        break;

      if (fetch_statement (mid_fp, opcode_manager, &statement, true, &LOCCTR, &statement_size) != 0)
        {
          /* handling error */
          ret = -1;
          goto ERROR;
        }

      line_no += 5;
    } /* Outer while문의 scope */
  
  LOCCTR += statement_size;

  VERIFY_TEXT_RECORD_MAX_BYTES (TEXT_RECORD_MAX_BYTES_PER_ROW);
  for (int i = 0; i < mod_LOCCTR_cnt; ++i)
    {
      snprintf (rec_head, MAX_REC_HEAD_LEN, "M%06X05", mod_LOCCTR_list[i]);
      fprintf (obj_fp, "%s\n", rec_head);
    }
  snprintf (rec_head, MAX_REC_HEAD_LEN, "E%06X", start_LOCCTR);
  fprintf (obj_fp, "%s\n", rec_head);
  snprintf (rec_head, MAX_REC_HEAD_LEN, "H%-6s%06X%06X", program_name, start_LOCCTR, LOCCTR - start_LOCCTR);
  fseek (obj_fp, 0, SEEK_SET);
  fprintf (obj_fp, "%s\n", rec_head);
    
  ret = 0;
  goto END;

#undef VERIFY_TEXT_RECORD_MAX_BYTES

ERROR:
  fprintf (stderr, "[ERROR] Line no %d: an error occurs in step 2.\n", line_no);
END:
  if (mid_fp)
    fclose (mid_fp);
  if (lst_fp)
    fclose (lst_fp);
  if (obj_fp)
    fclose (obj_fp);

  return ret;
}
