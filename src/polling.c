#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <string.h>

#include "polling.h"

static inline void delete_timer(int *const timer_fd)
{
    int ret = close(*timer_fd);
    if (-1 == ret)
    {
        printf("close(%d) failed. %s\n", *timer_fd, strerror(errno));
    }
    *timer_fd = -1;
}

static int create_timer(int *const timer_fd, int const timer_interval)
{
    struct itimerspec timer = {0};
    struct timespec now = {0};

    if(0 != clock_gettime(CLOCK_MONOTONIC,&now)){
        printf("clock_gettime() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    timer.it_value.tv_sec = now.tv_sec + timer_interval;
    timer.it_value.tv_nsec = 0;
    timer.it_interval.tv_sec = timer_interval;
    timer.it_interval.tv_nsec = 0;

    *timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (0 > *timer_fd )
    {
        printf("timerfd_create failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    if (0 > timerfd_settime(*timer_fd, TFD_TIMER_ABSTIME, &timer, NULL))
    {
        printf("timerfd_settime failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    printf("Timer create success!\n");

    return EXIT_SUCCESS;
}

static inline void delete_polling(int *const epoll_fd)
{
    int ret = close(*epoll_fd);
    if (-1 == ret)
    {
        printf("close(%d) failed. %s\n", *epoll_fd, strerror(errno));
    }
    *epoll_fd = -1;
}

static int create_poll(int *const epoll_fd, struct epoll_event *const events,  int const *const timer_fd)
{
    *epoll_fd = epoll_create(1);
    if (-1 == *epoll_fd)
    {
        printf("epoll_create() failed. %s\n",  strerror(errno));
        return EXIT_FAILURE;
    }

    events->events = EPOLLIN;
    if (0 < epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, *timer_fd, events))
    {
        printf("epoll_ctl() failed. %s\n", strerror(errno));
        delete_polling(epoll_fd);
        return EXIT_FAILURE;
    }

    printf("Poll create success!\n");

    return EXIT_SUCCESS;
}

int polling_pid(int const timer_interval)
{
    int ret = EXIT_SUCCESS;
    int timer_fd = 0;
    int epoll_fd = 0;
    size_t res = 0;
    struct epoll_event events = {0};

    ret = create_timer(&timer_fd, timer_interval);
    if (EXIT_SUCCESS != ret)
    {
        printf("create_timer() failed.\n");
        return ret;
    }
    ret = create_poll(&epoll_fd, &events, &timer_fd);
    if (EXIT_SUCCESS != ret)
    {
        printf("create_poll() failed.\n");
        delete_timer(&timer_fd);
        return ret;
    }

    while(1)
    {
        if (0 > epoll_wait(epoll_fd, &events, 1, 10000)) {
            printf("epoll_wait() failed. %s\n", strerror(errno));
            delete_polling(&epoll_fd);
            delete_timer(&timer_fd);
            return EXIT_FAILURE;
        }
        if (0 > read(timer_fd, &res, sizeof(res)))
        {
            printf("read() failed. %s\n", strerror(errno));
        }
        else
        {
            printf("Time out\n");
        }
    }

    delete_polling(&epoll_fd);
    delete_timer(&timer_fd);
    return ret;
}
