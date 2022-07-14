#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

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

void stop_process(struct process_info const *const process)
{
    int ret = EXIT_SUCCESS;

    ret = kill(process->pid, SIGTERM);
    if (0 > ret)
    {
        printf("kill() failed. %s\n", strerror(errno));
    }
}

int start_process(struct process_info *const process)
{
    enum
    {
        CMD_BUFFER_SIZE = PATH_MAX + 8,
        // 4194304 + \0.
        PID_LEN = 8,
    };
    char cmd_str[CMD_BUFFER_SIZE] = {0};
    char line[PID_LEN] = {0};
    pid_t pid = -1;
    FILE *result = NULL;
    char *fgets_res = NULL;
    pid_t pid_wait = -1;
    int ret = 0;

    char const *const sh = "/bin/sh";
    char *argv[] = {sh, "-c", process->cmd, NULL};

    pid = vfork();
    if (pid < 0)
    {
        return EXIT_FAILURE;
    }

    if (pid == 0)
    {
        ret = execv(sh, argv);
        _exit(ret);
    }

    do
    {
        pid_wait = waitpid(pid, &ret, 0);
        if (pid_wait <= -1 && errno != EINTR)
        {
            return EXIT_FAILURE;
        }
    } while (pid_wait != pid);

    if (0 != ret)
    {
        return EXIT_FAILURE;
    }

    // Не можем использовать PID ребенка, т.к. отслеживаемый процесс демонизируется и его PID не равен PID ребенка.
    snprintf(cmd_str, CMD_BUFFER_SIZE - 1, "pidof %s", process->cmd);

    result = popen(cmd_str, "r");
    if (NULL == result)
    {
        printf("Can't start %s\n", process->cmd);
        return EXIT_FAILURE;
    }

    fgets_res = fgets(line, PID_LEN, result);
    if (NULL == fgets_res)
    {
        printf("Can't get PID for %s\n", process->cmd);
        return EXIT_FAILURE;
    }

    pid = strtoul(line, NULL, 10);

    pclose(result);

    printf("PID for monitoring - %d\n", pid);

    process->pid = pid;

    return EXIT_SUCCESS;
}