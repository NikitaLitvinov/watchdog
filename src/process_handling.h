#ifndef SIMPLE_WATCHDOG_PROCESS_HANDLING_H
#define SIMPLE_WATCHDOG_PROCESS_HANDLING_H

#include <limits.h>
#include <stdbool.h>

struct process_info
{
    char cmd[PATH_MAX];
    pid_t pid;
    bool running;
};

int check_process_alive(pid_t pid, bool *is_alive);

int start_process(struct process_info *process);

void stop_process(struct process_info *process);

#endif //SIMPLE_WATCHDOG_PROCESS_HANDLING_H
