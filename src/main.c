#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "polling.h"

enum
{
    TIME_INTERVAL = 5,
};


int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;
    pid_t pid = 0;

    if (argc > 1)
    {
        pid = atoi(argv[1]);
    }
    else
    {
        printf("Input pid for check process\n");
        return EXIT_FAILURE;
    }

    ret = polling_pid(pid, TIME_INTERVAL);

    return ret;
}
