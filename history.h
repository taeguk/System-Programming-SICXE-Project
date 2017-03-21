#ifndef __HISTORY_H__
#define __HISTORY_H__

/* history manager는 오직 factory 함수를 통해서만 얻을 수 있다. */
struct history_manager;

/* history manager를 생성하고 소멸시키는 함수들 */
struct history_manager *history_manager_construct ();
void history_manager_destroy (struct history_manager *manager);

/* history manager에 history를 추가하는 함수 */
void history_insert (struct history_manager *manager, const char *command_str);

/* history manager내에 있는 history 들을 출력하는 함수 */
void history_print (struct history_manager *manager, const char *cur_command_str);

#endif
