#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "process_handling.h"

void signal_handling_do_noting(int const sig __attribute__((unused)))
{
}

int start_process(struct process_info *const process)
{
    pid_t pid_wait = -1;
    pid_t pid = -1;
    int ret = 0;

    pid = vfork();
    if (0 > pid)
    {
        printf("fork() failed.\n");
        return EXIT_FAILURE;
    }

    if (0 == pid)
    {
        char *const cmd[] = {"/bin/sh", "-c", process->process_cmd, NULL};

        printf("Process %s start.\n", process->process_name);
        ret = execv("/bin/sh", cmd);
        _exit(ret);
    }

    // Ignore SIGINT in parent to process it in child.
    signal(SIGINT, &signal_handling_do_noting);

    do
    {
        pid_wait = waitpid(pid, &ret, 0);
        if (-1 >= pid_wait && errno != EINTR)
        {
            return EXIT_FAILURE;
        }
    } while (pid_wait != pid);

    printf("Process %s finish.\n", process->process_name);

    return EXIT_SUCCESS;
}