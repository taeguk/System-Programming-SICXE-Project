#include "assemble.h"
#include "symbol.h"
#include "opcode.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define ASSEMBLE_STATEMENT_TOKEN_MAX_NUM 8
#define ASSEMBLE_STATEMENT_MAX_LEN 100 

#define SYMBOL_PART_MAX_LEN 7
#define MNEMONIC_PART_MAX_LEN

struct statement
  {
    bool is_comment;
    const char *symbol;
    const struct opcode *opcode;
    bool extend;
    /* tokens for operand */
    size_t token_cnt;
    char *token_list[ASSEMBLE_STATEMENT_TOKEN_MAX_NUM+1];
    char *input;
  };

static int assemble_pass_1 (const char *asm_file, const char *mid_file, 
                            const struct opcode_manager *opcode_manager,
                            struct symbol_manager *symbol_manager,
                            struct assemble_result *result);
static int assemble_pass_2 (const char *mid_file, const char *obj_file, 
                            const struct opcode_manager *opcode_manager,
                            const struct symbol_manager *symbol_manager,
                            struct assemble_result *result);

int assemble (const char *filename, const struct opcode_manager *opcode_manager,
              struct assemble_result *result)
{
  /*
   * 1. filename의 확장자가 .asm 인지 체크
   * 2. listing file과 outputfile 열기.
   */
  struct symbol_manager *symbol_manager = symbol_manager_construct ();
  char *dot = strrchr (filename, '.');
  assemble_pass_1 (filename, "TEMP_MID.txt", opcode_manager, symbol_manager, NULL);
}

static int fetch_statement (FILE *asm_fp, const struct opcode_manager *opcode_manager,
                            struct statement *statement)
{
  static char input[ASSEMBLE_STATEMENT_MAX_LEN];
  static char input_token[ASSEMBLE_STATEMENT_MAX_LEN];

  if (fgets (input, ASSEMBLE_STATEMENT_MAX_LEN, asm_fp) == NULL)
    return 0;

  printf ("[*] fgets -> %s", input);

  strcpy (input_token, input);

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
      for (int i = operand_token_offset; i < statement->token_cnt; ++i)
        statement->token_list[i - operand_token_offset] = statement->token_list[i];
      statement->token_cnt -= operand_token_offset;
    }

  return 0;
}

static int assemble_pass_1 (const char *asm_file, const char *mid_file, 
                            const struct opcode_manager *opcode_manager,
                            struct symbol_manager *symbol_manager,
                            struct assemble_result *result)
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
  size_t LOCCTR = 0;
  bool end = false;

  if (fetch_statement (asm_fp, opcode_manager, &statement) != 0)
    {
      /* handling error */
      ret = -1;
      goto ERROR;
    }

  if (!statement.is_comment && statement.opcode->op_format == OPCODE_START)
    {
      LOCCTR = atoi (statement.token_list[0]);
      fprintf (mid_fp, "%04X\t%s", LOCCTR, statement.input);

      if (fetch_statement (asm_fp, opcode_manager, &statement) != 0)
        {
          /* handling error */
          ret = -1;
          goto ERROR;
        }
    }

  while (!end)
    {
      /* process */

      if (statement.is_comment)
        {
          fprintf (mid_fp, "\t%s", statement.input);
        }
      else
        {
          fprintf (mid_fp, "%04X\t%s", LOCCTR, statement.input);

          // symbol을 symbol table에 넣음.
          if (statement.symbol)
            {
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
              LOCCTR += 3;
            }
          else if (statement.opcode->op_format == OPCODE_RESB)
            {
              int cnt = atoi (statement.token_list[0]);
              LOCCTR += cnt;
            }
          else if (statement.opcode->op_format == OPCODE_RESW)
            {
              int cnt = atoi (statement.token_list[0]);
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
        }

      if (fetch_statement (asm_fp, opcode_manager, &statement) != 0)
        {
          /* handling error */
          ret = -1;
          goto ERROR;
        }

      printf ("[*] statement -> is_comment = %d, op_format = %d\n", statement.is_comment, statement.is_comment ? -1 : statement.opcode->op_format);

      if (feof (asm_fp) != 0)
        end = true;
      else if (!statement.is_comment && statement.opcode->op_format == OPCODE_END)
        end = true;
    }
    
  fprintf (mid_fp, "\t%s", statement.input);

  ret = 0;
  goto END;

ERROR:
END:
  if (asm_fp)
    fclose (asm_fp);
  if (mid_fp)
    fclose (mid_fp);

  return ret;
}

static int assemble_pass_2 (const char *mid_file, const char *obj_file, 
                            const struct opcode_manager *opcode_manager,
                            const struct symbol_manager *symbol_manager,
                            struct assemble_result *result)
{

}
