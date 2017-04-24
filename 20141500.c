#include <stdio.h>
#include <string.h>

#include "command.h"

#define MEMORY_SIZE (1 * 1024 * 1024) /* 1MB */
#define OPCODE_FILE "opcode.txt"

static void insert_fake_opcodes (struct opcode_manager *manager)
{
  static const struct opcode fake_list[] = 
    {
        { 0, "START", OPCODE_START, OPCODE_FAKE },
        { 0, "END", OPCODE_END, OPCODE_FAKE },
        { 0, "BYTE", OPCODE_BYTE, OPCODE_FAKE },
        { 0, "WORD", OPCODE_WORD, OPCODE_FAKE },
        { 0, "RESB", OPCODE_RESB, OPCODE_FAKE },
        { 0, "RESW", OPCODE_RESW, OPCODE_FAKE },
        { 0, "BASE", OPCODE_BASE, OPCODE_FAKE },
        { 0, "NOBASE", OPCODE_NOBASE, OPCODE_FAKE }
    };

  for (size_t i = 0; i < sizeof(fake_list) / sizeof(*fake_list); ++i)
    {
      opcode_insert (manager, &fake_list[i]);
    }
}

/* Opcode File을 읽어들여서, opcode manager를 구축해주는 함수. */
static struct opcode_manager *read_opcode_file ()
{
  FILE *fp = fopen (OPCODE_FILE, "rt");
  struct opcode_manager *manager = NULL;

  if (!fp)
    goto ERROR;

  struct opcode opcode;
  char format_buf[16];
  unsigned int val;

  manager = opcode_manager_construct ();

  /* 파일에서 opcode 정보를 읽어들이면서 opcode manager에 그 정보를 추가한다. */
  while (fscanf (fp, "%X %6s %5s", /* TODO */ 
                 &val, opcode.name, format_buf) != EOF)
    {
#define COMPARE_WITH(STR) \
  (strcmp (opcode.name, (STR)) == 0)

      if (strcmp (format_buf, "1") == 0)
        {
          opcode.op_format = OPCODE_FORMAT_1;
          opcode.detail_format = OPCODE_FORMAT_1_GENERAL;
        }
      else if (strcmp (format_buf, "2") == 0)
        {
          opcode.op_format = OPCODE_FORMAT_2;

          if (COMPARE_WITH ("CLEAR") || COMPARE_WITH ("TIXR"))
            opcode.detail_format = OPCODE_FORMAT_2_ONE_REGISTER;
          else if (COMPARE_WITH ("SHIFTL") || COMPARE_WITH ("SHIFTR"))
            opcode.detail_format = OPCODE_FORMAT_2_REGISTER_N;
          else if (COMPARE_WITH ("SVC"))
            opcode.detail_format = OPCODE_FORMAT_2_ONE_N;
          else
            opcode.detail_format = OPCODE_FORMAT_2_GENERAL;
        }
      else if (strcmp (format_buf, "3/4") == 0)
        {
          opcode.op_format = OPCODE_FORMAT_3_4;

          if (COMPARE_WITH ("RSUB"))
            opcode.detail_format = OPCODE_FORMAT_3_4_NO_OPERAND;
          else
            opcode.detail_format = OPCODE_FORMAT_3_4_GENERAL;
        }
      else
        goto ERROR;

      opcode.val = val;
      opcode_insert (manager, &opcode);

#undef COMPARE_WITH
    }

  goto END;

ERROR:
  if (manager)
    {
      opcode_manager_destroy (manager);
      manager = NULL;
    }

END:
  if (fp)
    fclose (fp);

  return manager;
}

int main()
{
  struct command_state state;

  /* Command Loop의 State를 초기화한다. */

  state.history_manager = history_manager_construct ();
  state.memory_manager = memory_manager_construct (MEMORY_SIZE);
  if (!(state.opcode_manager = read_opcode_file ()))
    {
      fprintf (stderr, "[ERROR] Can't read opcode file \"%s\"\n", OPCODE_FILE);
      return 1;
    }
  insert_fake_opcodes (state.opcode_manager);
  state.symbol_manager = NULL;
  state.debug_manager = debug_manager_construct ();
  state.saved_dump_start = 0;
  state.progaddr = 0;

  /* Command Loop로 진입하여, 사용자의 입력을 처리한다. */
  command_loop(&state);

  return 0;
}
