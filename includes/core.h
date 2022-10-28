#ifndef CORE_H
#define CORE_H

#include "SI_EFM8BB1_Register_Enums.h"
#include "SI_EFM8BB1_Defs.h"
#include "stdint.h"
#include "string.h"
#include "stdbool.h"
#include "machine.h"


typedef enum {
    STATUS_OK = 0
} status_t;

typedef enum {
  PIN_MODE_OUT_OPEN_DRAIN = 0x0,
  PIN_MODE_OUT_PUSH_PULL  = 0x01,
  PIN_MODE_IN_ANALOG      = 0x10,
  PIN_MODE_IN_DIGITAL     = 0x11
} pin_mode_e;

typedef enum {
  P0_0 = 0x00,
  P0_1 = 0x01,
  P0_2 = 0x02,
  P0_3 = 0x03,
  P0_4 = 0x04,
  P0_5 = 0x05,
  P0_6 = 0x06,
  P0_7 = 0x07,
  P1_0 = 0x10,
  P1_1 = 0x11,
  P1_2 = 0x12,
  P1_3 = 0x13,
  P1_4 = 0x14,
  P1_5 = 0x15,
  P1_6 = 0x16,
  P1_7 = 0x17,
  P2_0 = 0x20,
  P2_1 = 0x21,
} gpio_e;


#define OPTIMIZE_MEMORY


#define HIGH 1
#define LOW  0

/* Reset watchdog */
#define reset_watchdog() (WDTCN = 0xA5)

/* Enable global interrupts */
#define sei() (IE_EA = 1)

/* Disable global interrupts */
#define cli() (IE_EA = 0)

/* Sets the value of a 16 bit SFR */
#define set_sfr_16(low, high, value) { \
  low = value; \
  high = value >> 8; \
}

/* Waits for given number of milliseconds. Not very accurate. */
void delay(uint32_t ms);

uint32_t millis();

void sys_init();

#ifndef OPTIMIZE_MEMORY
void digitalWrite(uint8_t pin, uint8_t value);

void pinMode(gpio_e pin, pin_mode_e mode);

uint64_t micros();
#endif

#endif /* CORE_H */
