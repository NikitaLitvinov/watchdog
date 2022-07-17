#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "polling.h"
#include "process_handling.h"

enum
{
    TIME_INTERVAL = 5,
};

int argv_handling(int const argc, char *const *const argv, struct process_info *const process)
{
    int ret = EXIT_SUCCESS;
    char const example_str[] = "Input process name with param for controlling.\n" \
                                  "Example: ./watchdog start <process_name> [args]";
    if (argc > 2)
    {
        size_t len = 0;

        if (0 != strcmp("start", argv[1]))
        {
            printf("%s\n", example_str);
            return EXIT_FAILURE;
        }

        process->process_name = argv[2];

        for (int i = 2; i < argc; ++i)
        {
            len = strlen(argv[i]);
        }
        process->process_cmd = malloc(len * sizeof(char) + argc);
        if (NULL == process->process_cmd)
        {
            printf("malloc() failed. %s", strerror(errno));
            return EXIT_FAILURE;
        }
        for (int i = 2; i < argc; ++i)
        {
            strcat(process->process_cmd, argv[i]);
            strcat(process->process_cmd, " ");
        }
    }
    else
    {
        printf("%s\n", example_str);
        return EXIT_FAILURE;
    }

    return ret;
}

int main(int argc, char **argv)
{
    int ret = EXIT_SUCCESS;
    struct process_info process = {0};

    ret = argv_handling(argc, argv, &process);
    if (EXIT_SUCCESS != ret)
    {
        return ret;
    }

    ret = start_process(&process);
    if (EXIT_SUCCESS != ret)
    {
        free(process.process_cmd);
        return ret;
    }

    ret = polling_pid(&process, TIME_INTERVAL);
    if (EXIT_SUCCESS != ret)
    {
        free(process.process_cmd);
        return ret;
    }

    free(process.process_cmd);

    return ret;
}
