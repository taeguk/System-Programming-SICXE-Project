#include "command.h"
#include <stdio.h>
#include <string.h>

#define MEMORY_SIZE (1 * 1024 * 1024) /* 1MB */
#define OPCODE_FILE "opcode.txt"

/* Opcode File을 읽어들여서, opcode manager를 구축해주는 함수. */
static struct opcode_manager *read_opcode_file ()
{
  FILE *fp = fopen (OPCODE_FILE, "rt");

  if (!fp)
    goto ERROR;

  struct opcode_manager *manager = opcode_manager_construct ();
  struct opcode opcode;
  char format_buf[16];
  unsigned int val;

  /* 파일에서 opcode 정보를 읽어들이면서 opcode manager에 그 정보를 추가한다. */
  while (fscanf (fp, "%X %6s %5s", /* TODO */ 
                 &val, opcode.name, format_buf) != EOF)
    {
      if (strcmp (format_buf, "1") == 0)
        opcode.op_format = OPCODE_FORMAT_1;
      else if (strcmp (format_buf, "2") == 0)
        opcode.op_format = OPCODE_FORMAT_2;
      else if (strcmp (format_buf, "3/4") == 0)
        opcode.op_format = OPCODE_FORMAT_3_4;
      else
        goto ERROR;

      opcode.val = val;
      opcode_insert (manager, &opcode);
    }

  return manager;

ERROR:
  if (fp)
    fclose (fp);
  if (manager)
    opcode_manager_destroy (manager);

  return NULL;
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
  state.saved_dump_start = 0;

  /* Command Loop로 진입하여, 사용자의 입력을 처리한다.  */
  command_loop(&state);

  return 0;
}
