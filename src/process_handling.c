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

void stop_process(struct process_info *const process)
{
    int ret = EXIT_SUCCESS;

    ret = kill(process->pid, SIGTERM);
    if (0 > ret)
    {
        printf("kill() failed. %s\n", strerror(errno));
    }

    printf("Process %d stop successfully!\n", process->pid);

    process->running = false;
    process->pid = -1;
}

int start_process(struct process_info *const process)
{
    enum
    {
        // strlen("pidof ") = 6 + \0
        CMD_BUFFER_SIZE = PATH_MAX + 7,
        // 4194304 + \0.
        PID_LEN = 8,
    };
    char cmd_str[CMD_BUFFER_SIZE] = {0};
    char pid_str[PID_LEN] = {0};
    char *fgets_res = NULL;
    FILE *result = NULL;
    pid_t pid_wait = -1;
    pid_t pid = -1;
    int ret = 0;

    pid = vfork();
    if (pid < 0)
    {
        printf("vfork() failed\n");
        return EXIT_FAILURE;
    }

    if (pid == 0)
    {
        char *const cmd[] = {"/bin/sh", "-c", process->process_cmd, NULL};
        ret = execv("/bin/sh", cmd);
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

    if (EXIT_SUCCESS != ret)
    {
        printf("Can't start %s\n", process->process_name);
        return EXIT_FAILURE;
    }

    // Не можем использовать PID ребенка, т.к. отслеживаемый процесс демонизируется и его PID не равен PID ребенка.
    snprintf(cmd_str, CMD_BUFFER_SIZE - 1, "pidof %s", process->process_name);

    result = popen(cmd_str, "r");
    if (NULL == result)
    {
        printf("peopen() failed\n");
        return EXIT_FAILURE;
    }

    fgets_res = fgets(pid_str, PID_LEN, result);
    if (NULL == fgets_res)
    {
        printf("Can't get PID for %s\n", process->process_name);
        pclose(result);
        return EXIT_FAILURE;
    }

    pid = strtoul(pid_str, NULL, 10);

    pclose(result);

    process->pid = pid;
    process->running = true;

    printf("Process %s (PID %d) start successfully!\n", process->process_name, process->pid);

    return EXIT_SUCCESS;
}