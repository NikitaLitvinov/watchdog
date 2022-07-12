#ifndef SIMPLE_WATCHDOG_PROCESS_HANDLING_H
#define SIMPLE_WATCHDOG_PROCESS_HANDLING_H

#include <stdbool.h>

int check_process_alive(pid_t pid, bool *is_alive);

#endif //SIMPLE_WATCHDOG_PROCESS_HANDLING_H
