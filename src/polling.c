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

static int create_timer(int *const timer_fd, int const timeout)
{
    struct itimerspec timer = {0};
    struct timespec now = {0};

    if (0 != clock_gettime(CLOCK_MONOTONIC, &now))
    {
        printf("clock_gettime() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    timer.it_value.tv_sec = now.tv_sec + timeout;
    timer.it_value.tv_nsec = 0;
    timer.it_interval.tv_sec = timeout;
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
    if (0 > ret)
    {
        printf("sigemptyset() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    ret = sigaddset(&sigset, SIGINT);
    if (0 > ret)
    {
        printf("sigaddset(%s) failed. %s\n", strsignal(SIGQUIT), strerror(errno));
        return EXIT_FAILURE;
    }

    ret = sigprocmask(SIG_BLOCK, &sigset, NULL);
    if (0 > ret)
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

static int delete_signal_handling(void)
{
    int ret = EXIT_SUCCESS;
    sigset_t sigset = {0};

    ret = sigemptyset(&sigset);
    if (0 > ret)
    {
        printf("sigemptyset() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    ret = sigaddset(&sigset, SIGINT);
    if (0 > ret)
    {
        printf("sigaddset(%s) failed. %s\n", strsignal(SIGQUIT), strerror(errno));
        return EXIT_FAILURE;
    }

    ret = sigprocmask(SIG_UNBLOCK, &sigset, NULL);
    if (0 > ret)
    {
        printf("sigprocmask() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    return ret;
}

int timer_for_restart(int const timeout, bool *const need_restart)
{
    enum
    {
        MAX_EVENT_COUNT = 10,
    };

    int ret = EXIT_SUCCESS;
    int timer_fd = 0;
    int epoll_fd = 0;
    int sig_fd = 0;
    int read_events = 0;
    struct epoll_event events[MAX_EVENT_COUNT] = {0};

    ret = create_timer(&timer_fd, timeout);
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

    printf("Start timeout - %d sec.\n", timeout);

    read_events = epoll_wait(epoll_fd, events, MAX_EVENT_COUNT, -1);
    if (0 > read_events)
    {
        printf("epoll_wait() failed. %s\n", strerror(errno));
        ret = EXIT_FAILURE;
    }

    for (int i = 0; i < read_events; i++)
    {
        if (events[i].data.fd == timer_fd)
        {
            size_t res = 0;
            if (0 > read(timer_fd, &res, sizeof(res)))
            {
                printf("read(timer_fd) failed. %s\n", strerror(errno));
            }
            else
            {
                printf("Timeout!\n");
                *need_restart = true;
            }
        }
        else if (events[i].data.fd == sig_fd)
        {
            struct signalfd_siginfo info = {0};
            if (0 > read(sig_fd, &info, sizeof(info)))
            {
                printf("read(sig_fd) failed. %s\n", strerror(errno));
            }
            else
            {
                printf("Catch signal %s\n", strsignal(info.ssi_signo));
                *need_restart = false;
            }
        }
    }

    printf("Finish timeout.\n");

    close_fd(&epoll_fd);
    close_fd(&sig_fd);
    close_fd(&timer_fd);

    ret = delete_signal_handling();
    if (EXIT_SUCCESS != ret)
    {
        printf("delete_signal_handling() failed.\n");
    }

    return EXIT_SUCCESS;
}
