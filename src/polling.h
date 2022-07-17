#ifndef SIMPLE_WATCHDOG_POLLING_H
#define SIMPLE_WATCHDOG_POLLING_H

#include "process_handling.h"

int timer_for_restart(int timeout, bool *need_restart);

#endif //SIMPLE_WATCHDOG_POLLING_H
