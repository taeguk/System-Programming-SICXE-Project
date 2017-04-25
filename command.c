#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>

#include "command.h"
#include "assemble.h"
#include "loader.h"
#include "run.h"

#define COMMAND_TOKEN_MAX_NUM 8

/* 사용자가 입력한 명령에 대한 parsing / processing 결과에 대한 코드 */
#define COMMAND_STATUS_SUCCESS 0
#define COMMAND_STATUS_INVALID_INPUT 1
#define COMMAND_STATUS_TOO_MANY_TOKENS 2
#define COMMAND_STATUS_FAIL_TO_PROCESS 3

enum command_type
  {
    COMMAND_HELP, COMMAND_DIR, COMMAND_QUIT, COMMAND_HISTORY,
    COMMAND_DUMP, COMMAND_EDIT, COMMAND_FILL, COMMAND_RESET,
    COMMAND_OPCODE, COMMAND_OPCODELIST,
    COMMAND_ASSEMBLE, COMMAND_TYPE, COMMAND_SYMBOL,
    COMMAND_PROGADDR, COMMAND_LOADER,
    COMMAND_RUN, COMMAND_BP
  };

/* 사용자가 입력한 명령을 의미하는 구조체 */
struct command
  {
    enum command_type type;
    size_t token_cnt;
    char *token_list[COMMAND_TOKEN_MAX_NUM+1];  // 사용자가 입력한 명령어가 token별로 쪼개서 여기에 들어간다.
    char *input;  // 사용자 입력한 명령어 라인 전부가 이 곳에 들어간다.
  };

static int command_fetch (struct command *command);
static int command_process (struct command_state *state, struct command *command, bool *quit);

static int command_h_help (struct command_state *state, struct command *command);
static int command_h_dir (struct command_state *state, struct command *command);
static int command_h_history (struct command_state *state, struct command *command);
static int command_h_dump (struct command_state *state, struct command *command);
static int command_h_edit (struct command_state *state, struct command *command);
static int command_h_fill (struct command_state *state, struct command *command);
static int command_h_reset (struct command_state *state, struct command *command);
static int command_h_opcode (struct command_state *state, struct command *command);
static int command_h_opcodelist (struct command_state *state, struct command *command);
static int command_h_assemble (struct command_state *state, struct command *command);
static int command_h_type (struct command_state *state, struct command *command);
static int command_h_symbol (struct command_state *state, struct command *command);
static int command_h_progaddr (struct command_state *state, struct command *command);
static int command_h_loader (struct command_state *state, struct command *command);
static int command_h_run(struct command_state *state, struct command *command);
static int command_h_bp (struct command_state *state, struct command *command);

bool command_loop (struct command_state *state)
{
  struct command command;
  bool quit = false;

  while (!quit) 
    {
      int error_code;

      printf("sicsim> ");
      error_code = command_fetch (&command);

      switch (error_code)
        {
        case COMMAND_STATUS_SUCCESS:
          break;

        case COMMAND_STATUS_INVALID_INPUT:
          fprintf (stderr, "[ERROR] Invalid command!\n");
          continue;

        case COMMAND_STATUS_TOO_MANY_TOKENS:
          fprintf (stderr, "[ERROR] Too many tokens.\n");
          continue;

        default:
          assert (false);
        }

      error_code = command_process (state, &command, &quit);

      switch (error_code)
        {
        case COMMAND_STATUS_SUCCESS:
          break;

        case COMMAND_STATUS_INVALID_INPUT:
          fprintf (stderr, "[ERROR] Invalid command parameters!!!\n");
          continue;

        case COMMAND_STATUS_FAIL_TO_PROCESS:
          fprintf (stderr, "[ERROR] Fail to process your command.\n");
          continue;

        default:
          assert (false);
        }

        history_insert (state->history_manager, command.input);
    }

  return true;
}

/* Command를 fetch하는 함수. 
 * 사용자로 부터 명령어를 입력받고, 그 것을 바탕으로 command 구조체를 구축한다.
 */
static int command_fetch (struct command *command)
{
  static char input[COMMAND_INPUT_MAX_LEN];
  static char input_token[COMMAND_INPUT_MAX_LEN];

  fgets (input, COMMAND_INPUT_MAX_LEN, stdin);
  strncpy (input_token, input, COMMAND_INPUT_MAX_LEN);

  command->input = input;
  command->token_cnt = 0;
  command->token_list[command->token_cnt] = strtok (input_token, " ,\t\n");
  while (command->token_cnt <= COMMAND_TOKEN_MAX_NUM && command->token_list[command->token_cnt])
    command->token_list[++command->token_cnt] = strtok (NULL, " ,\t\n");

  if (command->token_cnt <= 0)
    return COMMAND_STATUS_INVALID_INPUT;

  if (command->token_cnt > COMMAND_TOKEN_MAX_NUM)
    return COMMAND_STATUS_TOO_MANY_TOKENS;

#define COMPARE_WITH(STR) \
  (strcmp (command->token_list[0], (STR)) == 0)

  if (COMPARE_WITH ("h") || COMPARE_WITH ("help"))
    command->type = COMMAND_HELP
      ;
  else if (COMPARE_WITH ("d") || COMPARE_WITH ("dir"))
    command->type = COMMAND_DIR
      ;
  else if (COMPARE_WITH ("q") || COMPARE_WITH ("quit"))
    command->type = COMMAND_QUIT
      ;
  else if (COMPARE_WITH ("hi") || COMPARE_WITH ("history"))
    command->type = COMMAND_HISTORY
      ;
  else if (COMPARE_WITH ("du") || COMPARE_WITH ("dump"))
    command->type = COMMAND_DUMP
      ;
  else if (COMPARE_WITH ("e") || COMPARE_WITH ("edit"))
    command->type = COMMAND_EDIT
      ;
  else if (COMPARE_WITH ("f") || COMPARE_WITH ("fill"))
    command->type = COMMAND_FILL
      ;
  else if (COMPARE_WITH ("reset"))
    command->type = COMMAND_RESET
      ;
  else if (COMPARE_WITH ("opcode"))
    command->type = COMMAND_OPCODE
      ;
  else if (COMPARE_WITH ("opcodelist"))
    command->type = COMMAND_OPCODELIST
      ;
  else if (COMPARE_WITH ("assemble"))
    command->type = COMMAND_ASSEMBLE
      ;
  else if (COMPARE_WITH ("type"))
    command->type = COMMAND_TYPE
      ;
  else if (COMPARE_WITH ("symbol"))
    command->type = COMMAND_SYMBOL
      ;
  else if (COMPARE_WITH ("progaddr"))
    command->type = COMMAND_PROGADDR
      ;
  else if (COMPARE_WITH ("loader"))
    command->type = COMMAND_LOADER
      ;
  else if (COMPARE_WITH ("run"))
    command->type = COMMAND_RUN
      ;
  else if (COMPARE_WITH ("bp"))
    command->type = COMMAND_BP
      ;
  else
    return COMMAND_STATUS_INVALID_INPUT;

#undef COMPARE_WITH

  return COMMAND_STATUS_SUCCESS;
}

/* Command를 처리하는 함수.
 * command type에 따라 서로 다른 handler를 호출해 준다.
 */
static int command_process (struct command_state *state, struct command *command, bool *quit)
{
  *quit = false;

  switch (command->type)
    {
    case COMMAND_HELP:
      return command_h_help (state, command);
        ;
    case COMMAND_DIR:
      return command_h_dir (state, command);
        ;
    case COMMAND_QUIT:
      *quit = true;
      return COMMAND_STATUS_SUCCESS;
        ;
    case COMMAND_HISTORY:
      return command_h_history (state, command);
        ;
    case COMMAND_DUMP:
      return command_h_dump (state, command);
        ;
    case COMMAND_EDIT:
      return command_h_edit (state, command);
        ;
    case COMMAND_FILL:
      return command_h_fill (state, command);
        ;
    case COMMAND_RESET:
      return command_h_reset (state, command);
        ;
    case COMMAND_OPCODE:
      return command_h_opcode (state, command);
        ;
    case COMMAND_OPCODELIST:
      return command_h_opcodelist (state, command);
        ;
    case COMMAND_ASSEMBLE:
      return command_h_assemble (state, command);
        ;
    case COMMAND_TYPE:
      return command_h_type (state, command);
        ;
    case COMMAND_SYMBOL:
      return command_h_symbol (state, command);
        ;
    case COMMAND_PROGADDR:
      return command_h_progaddr (state, command);
        ;
    case COMMAND_LOADER:
      return command_h_loader (state, command);
        ;
    case COMMAND_RUN:
      return command_h_run (state, command);
        ;
    case COMMAND_BP:
      return command_h_bp (state, command);
        ;
    default:
      return COMMAND_STATUS_INVALID_INPUT;
    }
}

/* help 명령어에 대한 handler. */
static int command_h_help (__attribute__((unused)) struct command_state *state, __attribute__((unused)) struct command *command)
{
  if (command->token_cnt != 1)
    return COMMAND_STATUS_INVALID_INPUT;

  printf ("h[elp]\n"
          "d[ir]\n"
          "q[uit]\n"
          "hi[story]\n"
          "du[mp] [start, end]\n"
          "e[dit] address, value\n"
          "f[ill] start, end, value\n"
          "reset\n"
          "opcode mnemonic\n"
          "opcodelist\n"
          "assemble filename\n"
          "type filename\n"
          "symbol\n"
          "progaddr address\n"
          "loader object_filename_1 [object file names...]\n"
          "run\n"
          "bp [address]\n"
          "bp clear\n"
  );
  return COMMAND_STATUS_SUCCESS;
}

/* dir 명령어에 대한 handler */
static int command_h_dir (__attribute__((unused)) struct command_state *state, __attribute__((unused)) struct command *command)
{
  if (command->token_cnt != 1)
    return COMMAND_STATUS_INVALID_INPUT;

  DIR *dir;
  struct dirent *entry;
  struct stat stat;

  if (!(dir = opendir (".")))
    {
      fprintf (stderr, "[ERROR] Cannot open directory '.'\n");
      return COMMAND_STATUS_FAIL_TO_PROCESS;
    }

  char buf[1024];  // need to be refactored.
  int cnt = 0;
  printf ("\t");
  while ((entry = readdir (dir)))
    {
      lstat (entry->d_name, &stat);

      if (S_ISDIR (stat.st_mode))
        sprintf (buf, "%s/", entry->d_name);
      else if (S_IXUSR & stat.st_mode)
        sprintf (buf, "%s*", entry->d_name);
      else
        sprintf (buf, "%s ", entry->d_name);

      printf ("%-20s", buf);

      if (++cnt % 4 == 0)
        printf("\n\t");
    }
  if (cnt % 4 != 0)
    printf ("\n");

  closedir (dir);

  return COMMAND_STATUS_SUCCESS;
}

/* history 명령어에 대한 handler */
static int command_h_history (struct command_state *state, struct command *command)
{
  if (command->token_cnt != 1)
    return COMMAND_STATUS_INVALID_INPUT;

  history_print (state->history_manager, command->input);
  return COMMAND_STATUS_SUCCESS;
}

/* dump 명령어에 대한 handler */
static int command_h_dump (struct command_state *state, struct command *command)
{
  uint32_t start, end;
  bool enable_max_end;
  uint32_t memory_size = memory_get_memory_size (state->memory_manager);

  // 명령어의 형태가 매개변수없는 dump 일 때,
  if (command->token_cnt == 1)
    {
      start = state->saved_dump_start;
      end = start + 159;
      // memory size를 넘어가는 경우, 다음 dump때 출력할 위치를 0으로 초기화한다.
      if (end > memory_size)
        state->saved_dump_start = 0;
      else
        state->saved_dump_start += 160;
      enable_max_end = true;
    }
  // 명령어의 형태가 dump start 일 때,
  else if (command->token_cnt == 2)
    {
      start = strtol (command->token_list[1], NULL, 16);
      end = start + 159;
      enable_max_end = true;
    }
  // 명령어의 형태가 dump start, end 일 때,
  else if (command->token_cnt == 3)
    {
      start = strtol (command->token_list[1], NULL, 16);
      end = strtol (command->token_list[2], NULL, 16);
      enable_max_end = false;
    }
  else
    return COMMAND_STATUS_INVALID_INPUT;

  if (!memory_dump (state->memory_manager, start, end, enable_max_end))
    {
      return COMMAND_STATUS_INVALID_INPUT;
    }

  return COMMAND_STATUS_SUCCESS;
}

/* edit 명령어에 대한 handler */
static int command_h_edit (struct command_state *state, struct command *command)
{
  if (command->token_cnt != 3)
    return COMMAND_STATUS_INVALID_INPUT;

  uint32_t offset;
  uint32_t val;
  
  offset = strtol (command->token_list[1], NULL, 16);
  val = strtol (command->token_list[2], NULL, 16);
  if (val > 0xFF)
    return COMMAND_STATUS_INVALID_INPUT;

  if (!memory_edit (state->memory_manager, offset, (uint8_t) val))
    {
      return COMMAND_STATUS_INVALID_INPUT;
    }

  return COMMAND_STATUS_SUCCESS;
}

/* fill 명령어에 대한 handler */
static int command_h_fill (struct command_state *state, struct command *command)
{
  if (command->token_cnt != 4)
    return COMMAND_STATUS_INVALID_INPUT;

  uint32_t start, end;
  uint32_t val;
  
  start = strtol (command->token_list[1], NULL, 16);
  end = strtol (command->token_list[2], NULL, 16);
  val = strtol (command->token_list[3], NULL, 16);
  if (val > 0xFF)
    return COMMAND_STATUS_INVALID_INPUT;

  if (!memory_fill (state->memory_manager, start, end, (uint8_t) val))
    {
      return COMMAND_STATUS_INVALID_INPUT;
    }

  return COMMAND_STATUS_SUCCESS;
}

/* reset 명령어에 대한 handler */
static int command_h_reset (struct command_state *state, struct command *command)
{
  if (command->token_cnt != 1)
    return COMMAND_STATUS_INVALID_INPUT;

  memory_reset (state->memory_manager);
  return COMMAND_STATUS_SUCCESS;
}

/* opcode 명령어에 대한 handler */
static int command_h_opcode (struct command_state *state, struct command *command)
{
  if (command->token_cnt != 2)
    return COMMAND_STATUS_INVALID_INPUT;

  const struct opcode *opcode = opcode_find (state->opcode_manager, command->token_list[1]);

  if (opcode)
    {
      printf ("opcode is %02X\n", opcode->val);
      return COMMAND_STATUS_SUCCESS;
    }
  else
    {
      fprintf (stderr, "[ERROR] Cannot find opcode.\n");
      return COMMAND_STATUS_FAIL_TO_PROCESS;
    }
}

/* opcodelist 명령어에 대한 handler */
static int command_h_opcodelist (struct command_state *state, struct command *command)
{
  if (command->token_cnt != 1)
    return COMMAND_STATUS_INVALID_INPUT;

  opcode_print_list (state->opcode_manager);
  return COMMAND_STATUS_SUCCESS;
}

static int command_h_assemble (struct command_state *state, struct command *command)
{
  if (command->token_cnt != 2)
    return COMMAND_STATUS_INVALID_INPUT;

  struct symbol_manager *symbol_manager = symbol_manager_construct ();
  int error_code = assemble (command->token_list[1], state->opcode_manager, symbol_manager);
  if (error_code != 0)
    {
      fprintf (stderr, "[ERROR] Assemble Fail.\n");
      symbol_manager_destroy (symbol_manager);
      return COMMAND_STATUS_FAIL_TO_PROCESS;
    }

  if (state->symbol_manager)
    symbol_manager_destroy (state->symbol_manager);

  state->symbol_manager = symbol_manager;

  return COMMAND_STATUS_SUCCESS;
}

static int command_h_type (__attribute__((unused)) struct command_state *state, struct command *command)
{
  if (command->token_cnt != 2)
    return COMMAND_STATUS_INVALID_INPUT;

  FILE *fp = fopen (command->token_list[1], "rt");

  if (!fp)
    {
      fprintf (stderr, "[ERROR] Invalid file name: %s\n", command->token_list[1]);
      return COMMAND_STATUS_INVALID_INPUT;
    }

  char buf[512];

  // TODO: 마지막 개행 처리??
  while (fgets (buf, sizeof(buf), fp))
    fputs (buf, stdout);

  fclose (fp);

  return COMMAND_STATUS_SUCCESS;
}

static int command_h_symbol (struct command_state *state, struct command *command)
{
  if (command->token_cnt != 1)
    return COMMAND_STATUS_INVALID_INPUT;

  if (!state->symbol_manager)
    {
      fprintf (stderr, "[ERROR] There is no symbol table.\n");
      return COMMAND_STATUS_FAIL_TO_PROCESS;
    }
  
  symbol_print_list (state->symbol_manager);

  return COMMAND_STATUS_SUCCESS;
}

static int command_h_progaddr (struct command_state *state, struct command *command)
{
  if (command->token_cnt != 2)
    return COMMAND_STATUS_INVALID_INPUT;

  uint32_t progaddr;
  progaddr = strtol (command->token_list[1], NULL, 16);

  if (progaddr > 0xFFFF)
    {
      fprintf (stderr, "[ERROR] Invalid progaddr : %s\n", command->token_list[1]);
      return COMMAND_STATUS_INVALID_INPUT;
    }

  state->progaddr = progaddr;

  return COMMAND_STATUS_SUCCESS;
}

static int command_h_loader (struct command_state *state, struct command *command)
{
  if (command->token_cnt == 1)
    {
      fprintf (stderr, "[ERROR] No object file names.\n");
      return COMMAND_STATUS_INVALID_INPUT;
    }
  else if (command->token_cnt > 4)
    {
      fprintf (stderr, "[ERROR] Maximum of the number of object files is 4.\n");
      return COMMAND_STATUS_INVALID_INPUT;
    }

  static char *object_filename_list[3];

  for (size_t i = 1; i < command->token_cnt; ++i)
    object_filename_list[i-1] = command->token_list[i];

  // TODO: memory manager의 예외 안정성 고려하기.
  int error_code = loader(state->memory_manager, state->progaddr,
                          (const char **)object_filename_list, command->token_cnt-1);
  if (error_code != 0)
    {
      fprintf (stderr, "[ERROR] Loader Fail.\n");
      return COMMAND_STATUS_FAIL_TO_PROCESS;
    }

  return COMMAND_STATUS_SUCCESS;
}

static int command_h_run(struct command_state *state, struct command *command)
{
  if (!state->is_running)
    state->reg_set.PC = state->progaddr;
  int error_code = run (state->memory_manager, state->debug_manager, &state->reg_set, &state->is_running);
  if (error_code != 0)
    {
      fprintf (stderr, "[ERROR] Run Fail.\n");
      return COMMAND_STATUS_FAIL_TO_PROCESS;
    }

  printf (
      "A : %08X  X : %08X \n"
      "L : %08X  PC: %08X \n"
      "B : %08X  S : %08X \n"
      "T : %08X           \n",
      state->reg_set.A, state->reg_set.X,
      state->reg_set.L, state->reg_set.PC,
      state->reg_set.B, state->reg_set.S,
      state->reg_set.T
  );

  return COMMAND_STATUS_SUCCESS;
}

static int command_h_bp (struct command_state *state, struct command *command)
{
  if (command->token_cnt > 2)
    return COMMAND_STATUS_INVALID_INPUT;

  if (command->token_cnt == 1)
    {
      debug_bp_print_list (state->debug_manager);
    }
  else
    {
      if (strcmp (command->token_list[1], "clear") == 0)
        {
          debug_bp_clear (state->debug_manager);
        }
      else
        {
          uint32_t address = strtol (command->token_list[1], NULL, 16);
          debug_bp_add (state->debug_manager, address);
        }
    }

  return COMMAND_STATUS_SUCCESS;
}
