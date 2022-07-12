#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "process_handling.h"

int check_process_alive(pid_t const pid, bool *const is_alive)
{
    int ret = EXIT_SUCCESS;

    *is_alive = true;

    // sig 0 - checking process alive.
    ret = kill(pid, 0);
    if (0 > ret)
    {
        if (ESRCH == errno)
        {
            errno = 0;
            *is_alive = false;
            ret = EXIT_SUCCESS;
        }
        else
        {
            printf("kill() failed. %s\n", strerror(errno));
            ret = EXIT_FAILURE;
        }
    }

    return ret;
}
