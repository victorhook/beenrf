#ifndef STATE_H
#define STATE_H

#include "core.h"

typedef struct {
    uint16_t thrust;
    uint16_t steer;
} motor_control_t;


typedef struct {
    motor_control_t current;
    motor_control_t target;
} state_t;

extern state_t state;


#endif /* STATE_H */
