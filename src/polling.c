#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>

#include "polling.h"

static inline void close_fd(int *const fd)
{
    int ret = close(*fd);

    if (-1 == ret)
    {
        printf("close(%d) failed. %s\n", *fd, strerror(errno));
    }
    *fd = -1;
}

static int create_timer(int *const timer_fd, int const timer_interval)
{
    struct itimerspec timer = {0};
    struct timespec now = {0};

    if (0 != clock_gettime(CLOCK_MONOTONIC, &now))
    {
        printf("clock_gettime() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    timer.it_value.tv_sec = now.tv_sec + timer_interval;
    timer.it_value.tv_nsec = 0;
    timer.it_interval.tv_sec = timer_interval;
    timer.it_interval.tv_nsec = 0;

    *timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (0 > *timer_fd)
    {
        printf("timerfd_create failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (0 > timerfd_settime(*timer_fd, TFD_TIMER_ABSTIME, &timer, NULL))
    {
        printf("timerfd_settime failed. %s\n", strerror(errno));
        close_fd(timer_fd);
        return EXIT_FAILURE;
    }

    printf("Timer create success!\n");

    return EXIT_SUCCESS;
}

static int create_poll(int *const epoll_fd, int const *const timer_fd,
                       int const *const sig_fd)
{
    struct epoll_event event = {0};

    *epoll_fd = epoll_create1(0);
    if (-1 == *epoll_fd)
    {
        printf("epoll_create() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    event.events = EPOLLIN;
    event.data.fd = *timer_fd;
    if (0 < epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, *timer_fd, &event))
    {
        printf("epoll_ctl() for timer fd failed. %s\n", strerror(errno));
        close_fd(epoll_fd);
        return EXIT_FAILURE;
    }

    event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    event.data.fd = *sig_fd;

    if (0 < epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, *sig_fd, &event))
    {
        printf("epoll_ctl() for sig fd failed. %s\n", strerror(errno));
        close_fd(epoll_fd);
        return EXIT_FAILURE;
    }

    printf("Poll create success!\n");

    return EXIT_SUCCESS;
}

static int create_signal(int *const sig_fd)
{
    int ret = EXIT_SUCCESS;
    sigset_t sigset = {0};

    ret = sigemptyset(&sigset);
    if (ret < 0)
    {
        printf("sigemptyset() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    ret = sigaddset(&sigset, SIGTERM);
    if (ret < 0)
    {
        printf("sigaddset(%s) failed. %s\n", strsignal(SIGTERM), strerror(errno));
        return EXIT_FAILURE;
    }

    ret = sigaddset(&sigset, SIGQUIT);
    if (ret < 0)
    {
        printf("sigaddset(%s) failed. %s\n", strsignal(SIGQUIT), strerror(errno));
        return EXIT_FAILURE;
    }

    ret = sigaddset(&sigset, SIGINT);
    if (ret < 0)
    {
        printf("sigaddset(%s) failed. %s\n", strsignal(SIGQUIT), strerror(errno));
        return EXIT_FAILURE;
    }

    ret = sigaddset(&sigset, SIGUSR1);
    if (ret < 0)
    {
        printf("sigaddset(%s) failed. %s\n", strsignal(SIGQUIT), strerror(errno));
        return EXIT_FAILURE;
    }

    ret = sigprocmask(SIG_BLOCK, &sigset, NULL);
    if (ret < 0)
    {
        printf("sigprocmask() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    *sig_fd = signalfd(-1, &sigset, 0);
    if (0 > *sig_fd)
    {
        printf("signalfd() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    printf("Signal fd create success!\n");

    return EXIT_SUCCESS;
}

int polling_pid(struct process_info *const process, int const timer_interval)
{
    enum
    {
        MAX_EVENT_COUNT = 10,
    };

    int ret = EXIT_SUCCESS;
    int timer_fd = 0;
    int epoll_fd = 0;
    int sig_fd = 0;
    struct epoll_event events[MAX_EVENT_COUNT] = {0};
    bool loop = true;

    ret = create_timer(&timer_fd, timer_interval);
    if (EXIT_SUCCESS != ret)
    {
        printf("create_timer() failed.\n");
        return ret;
    }

    ret = create_signal(&sig_fd);
    if (EXIT_SUCCESS != ret)
    {
        printf("create_signal() failed.\n");
        close_fd(&timer_fd);
        return ret;
    }

    ret = create_poll(&epoll_fd, &timer_fd, &sig_fd);
    if (EXIT_SUCCESS != ret)
    {
        printf("create_poll() failed.\n");
        close_fd(&timer_fd);
        close_fd(&sig_fd);
        return ret;
    }

    printf("Start polling process %s (PID %d).\n", process->process_name, process->pid);

    while (loop)
    {
        int read_events = epoll_wait(epoll_fd, events, MAX_EVENT_COUNT, -1);
        if (0 > read_events)
        {
            printf("epoll_wait() failed. %s\n", strerror(errno));
            ret = EXIT_FAILURE;
            loop = false;
            break;
        }
        for (int i = 0; i < read_events; i++)
        {
            if (events[i].data.fd == timer_fd)
            {
                size_t res = 0;
                if (0 > read(timer_fd, &res, sizeof(res)))
                {
                    printf("read() failed. %s\n", strerror(errno));
                }
                else
                {
                    if (true == process->running)
                    {
                        bool is_alive = false;
                        ret = check_process_alive(process->pid, &is_alive);
                        if (EXIT_SUCCESS != ret)
                        {
                            printf("check_process_alive() failed.\n");
                        }
                        else
                        {
                            if (false == is_alive)
                            {
                                printf("Process %s is not alive. Try restart.\n", process->process_name);
                                ret = start_process(process);
                                if (EXIT_SUCCESS != ret)
                                {
                                    printf("start_process() failed.\n");
                                }
                            }
                        }
                    }
                }
            }
            else if (events[i].data.fd == sig_fd)
            {
                struct signalfd_siginfo info = {0};
                if (0 > read(sig_fd, &info, sizeof(info)))
                {
                    printf("read() failed. %s\n", strerror(errno));
                }
                else
                {
                    printf("Catch signal %s\n", strsignal(info.ssi_signo));
                    if (SIGUSR1 == info.ssi_signo)
                    {
                        bool is_alive = false;
                        ret = check_process_alive(process->pid, &is_alive);
                        if (EXIT_SUCCESS != ret)
                        {
                            printf("check_process_alive() failed.\n");
                        }
                        else
                        {
                            if (true == is_alive && true == process->running)
                            {
                                printf("Stop process %s by signal USR1\n", process->process_name);
                                stop_process(process);
                            }
                            else
                            {
                                printf("Start process %s by signal USR1.\n", process->process_name);
                                ret = start_process(process);
                                if (EXIT_SUCCESS != ret)
                                {
                                    printf("start_process() failed.\n");
                                }
                            }
                        }
                    }
                    else
                    {
                        loop = false;
                        stop_process(process);
                        break;
                    }
                }
            }
        }
    }

    printf("Finish polling.\n");

    close_fd(&epoll_fd);
    close_fd(&sig_fd);
    close_fd(&timer_fd);

    return EXIT_SUCCESS;
}
