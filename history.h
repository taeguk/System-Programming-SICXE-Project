#ifndef __HISTORY_H__
#define __HISTORY_H__

struct history_manager;

struct history_manager *history_manager_construct ();
void history_manager_destroy (struct history_manager *manager);

void history_insert (struct history_manager *manager, const char *command_str);

void history_print (struct history_manager *manager, const char *cur_command_str);

#endif
