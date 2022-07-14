#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "polling.h"
#include "process_handling.h"

enum
{
    TIME_INTERVAL = 5,
};


int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;
    struct process_info process = {0};
    if (argc > 1)
    {
        snprintf(process.cmd, sizeof(process.cmd) - 1, "%s", argv[1]);
    }
    else
    {
        printf("Input pid for check process\n");
        return EXIT_FAILURE;
    }
    ret = start_process(&process);
    if (EXIT_SUCCESS != ret)
    {
        return ret;
    }
    ret = polling_pid(&process, TIME_INTERVAL);

    return ret;
}
