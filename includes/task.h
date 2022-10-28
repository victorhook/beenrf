#ifndef TASK_H
#define TASK_H

#include "core.h"


typedef status_t (*task_init)();

typedef void (*task_update)();

typedef struct {
    task_init   init;
    task_update update;
    uint8_t     period_ms;

    // Private
    uint16_t    last_completed;
} task_t;


#endif /* TASK_H */
