#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "polling.h"
#include "process_handling.h"

enum
{
    TIMEOUT_SEC = 5,
};

static int argv_handling(int const argc, char *const *const argv, struct process_info *const process)
{
    enum {
        // Exclude from iteration `./watchdog` and `start`.
        COUNT_OF_SERVICE_PARAM = 2,
    };
    int ret = EXIT_SUCCESS;
    char const example_str[] = "Input process name with param for controlling.\n" \
                                  "Example: ./watchdog start <process_name> [args]";

    if (argc > COUNT_OF_SERVICE_PARAM)
    {
        size_t len = 0;

        if (0 != strcmp("start", argv[1]))
        {
            printf("%s\n", example_str);
            return EXIT_FAILURE;
        }

        process->process_name = argv[2];

        for (int i = COUNT_OF_SERVICE_PARAM; i < argc; ++i)
        {
            len += strlen(argv[i]);
        }

        process->process_cmd = malloc(len * sizeof(char) + argc);
        if (NULL == process->process_cmd)
        {
            printf("malloc() failed. %s", strerror(errno));
            return EXIT_FAILURE;
        }

        for (int i = COUNT_OF_SERVICE_PARAM; i < argc; ++i)
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
    bool need_restart = true;

    ret = argv_handling(argc, argv, &process);
    if (EXIT_SUCCESS != ret)
    {
        return ret;
    }

    printf("Watchdog start.\n");

    while (true)
    {
        ret = start_process(&process);
        if (EXIT_SUCCESS != ret)
        {
            break;
        }

        printf("Process will be restart with timeout %d sec.\n"
               "To stop restart and exit press Ctrl-C.\n", TIMEOUT_SEC);

        ret = timer_for_restart(TIMEOUT_SEC, &need_restart);
        if (EXIT_SUCCESS != ret || false == need_restart)
        {
            break;
        }
    }

    printf("Watchdog finish.\n");

    free(process.process_cmd);

    return ret;
}
