#ifndef RADIO_H
#define RADIO_H

#include "core.h"


typedef enum {
    RADIO_COMMAND_SETPOINT = 1
} radio_command_t;


/* Initializes the radio. */
status_t radio_init();

/* Updates the radio. */
void radio_update();


#endif /* RADIO_H */
