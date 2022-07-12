#include "polling.h"

enum {
    TIME_INTERVAL = 5,
};


int main()
{
    int ret = polling_pid(TIME_INTERVAL);
    return ret;
}
