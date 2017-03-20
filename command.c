#include "command.h"
#include <string.h>
#include <stdlib.h>

#define COMMAND_TOKEN_MAX_NUM 8

static enum command_type
  {
    COMMAND_HELP, COMMAND_DIR, COMMAND_QUIT, COMMAND_HISTORY,
    COMMAND_DUMP, COMMAND_EDIT, COMMAND_FILL, COMMAND_RESET,
    COMMAND_RESET, COMMAND_OPCODE, COMMAND_OPCODELIST
  };

static struct command
  {
    enum command_type type;
    size_t token_cnt;
    char *token_list[COMMAND_TOKEN_MAX_NUM+1];
  };

static bool command_fetch (struct command *command);
static bool command_process (struct command *command, bool *quit);

static bool command_h_help (struct command *command);
static bool command_h_dir (struct command *command);
static bool command_h_history (struct command *command);
static bool command_h_dump (struct command *command);
static bool command_h_edit (struct command *command);
static bool command_h_fill (struct command *command);
static bool command_h_reset (struct command *command);
static bool command_h_opcode (struct command *command);
static bool command_h_opcodelist (struct command *command);

bool command_loop ()
{
  struct command command;
  bool quit = false;

  while (true) 
    {
      while(!command_fetch(&command)) 
        {
          // error handling
          fprintf(stderr, "Invalid input\n");
        }
      bool success = command_process(&command, &quit);
      if (success && quit)
        break; /* QUIT! */
      else if (!success)
        /* error handling will be added. */;
    }

  return true;
}

static bool command_fetch (struct command *command)
{
  static char input[COMMAND_INPUT_MAX_LEN];
  
  fgets (input, COMMAND_INPUT_MAX_LEN, stdin);

  command->token_cnt = 0;
  command->token_list[command->token_cnt] = strtok (input, " \t\n");
  while (command->token_cnt <= COMMAND_TOKEN_MAX_NUM && command->token_list[command->token_cnt])
    command->token[++command->token_cnt] = strtok (input, " \t\n");

  if (command->token_cnt <= 0 || command->token_cnt > COMMAND_TOKEN_MAX_NUM)
    return false;

#define COMPARE_WITH (STR) \
  (strncmp (command->token_list[0], (STR), COMMAND_INPUT_MAX_LEN) == 0)

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
  else
    return false;

  return true;
}

static bool command_process (struct command *command, bool *quit)
{
  *quit = false;

  switch (command->type)
    {
    case COMMAND_HELP:
      return command_h_help (command);
        ;
    case COMMAND_DIR:
      return command_h_dir (command);
        ;
    case COMMAND_QUIT:
      *quit = true;
      return true;
      ;
    case COMMAND_HISTORY:
      return command_h_history (command);
        ;
    case COMMAND_DUMP:
      return command_h_dump (command);
        ;
    case COMMAND_EDIT:
      return command_h_edit (command);
        ;
    case COMMAND_FILL:
      return command_h_fill (command);
        ;
    case COMMAND_RESET:
      return command_h_reset (command);
        ;
    case COMMAND_OPCODE:
      return command_h_opcode (command);
        ;
    case COMMAND_OPCODELIST:
      return command_h_opcodelist (command);
        ;
    default:
      return false;
    }

  return true;
}

static bool command_h_help (struct command *command);
static bool command_h_dir (struct command *command);
static bool command_h_history (struct command *command);
static bool command_h_dump (struct command *command);
static bool command_h_edit (struct command *command);
static bool command_h_fill (struct command *command);
static bool command_h_reset (struct command *command);
static bool command_h_opcode (struct command *command);
static bool command_h_opcodelist (struct command *command);

