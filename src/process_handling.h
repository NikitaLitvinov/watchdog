#ifndef SIMPLE_WATCHDOG_PROCESS_HANDLING_H
#define SIMPLE_WATCHDOG_PROCESS_HANDLING_H

struct process_info
{
    char *process_name;
    char *process_cmd;
};

int start_process(struct process_info *process);

#endif //SIMPLE_WATCHDOG_PROCESS_HANDLING_H
