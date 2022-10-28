#ifndef MACHINE_H
#define MACHINE_H

#include "core.h"


#define PIN_NRF_INT       P0_B0
#define PIN_MOTOR_RIGHT_2 P0_B1  // M2
#define PIN_MOTOR_RIGHT_1 P0_B2  // M2
#define PIN_VBAT          P0_B6
#define PIN_BTN           P0_B7
#define PIN_MOTOR_LEFT_2  P1_B0
#define PIN_MOTOR_LEFT_1  P1_B1


//#define DEV_BOARD

#ifdef DEV_BOARD
    // Local devboard
    #define PIN_LED           P1_B4

    // SPI
    #define PIN_NRF_CE P1_B5
    #define PIN_NRF_CS P1_B6
    #define PIN_SCK    P1_B7
    #define PIN_MOSI   P1_B3
    #define PIN_MISO   P1_B2
#else
    // Real BeeNRF
    #define PIN_LED           P0_B3

    // SPI
    #define PIN_NRF_CE        P1_B2
    #define PIN_NRF_CS        P1_B3
    #define PIN_SCK           P1_B4
    #define PIN_MOSI          P1_B5
    #define PIN_MISO          P1_B6
#endif


#endif /* MACHINE_H */
