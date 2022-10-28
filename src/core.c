#include "core.h"


// Clock resolution: 1959.1836734693877 ns
// (1/(24.5 * 1e6 / 48))
// systick represents ~2 us, allows for ~140 minutes run-time.
static uint32_t sys_tick = 0;

uint32_t millis() {
  //__asm__(
  //  "mov	dpl,_sys_tick\n"
  //  "mov	dph,(_sys_tick + 1)\n"
  //  "mov	b,(_sys_tick + 2)\n"
  //  "mov	a,(_sys_tick + 3)\n"
  //  "ret\n"
  //);
  return sys_tick;
}

void delay(uint32_t ms) {
  uint32_t t0 = millis();
  while ((millis() - t0) < ms);
}

#define LIMIT_PIN_MODE_OUTPUT 0x10
#define LIMIT_PIN_P0          0x10
#define LIMIT_PIN_P1          0x20

#define set_bit(reg, bit, value) \
        reg &= ~(1 << bit); \
        reg |= (value << bit)


static void disable_watchdog() {
  // Magic numbers come from datasheet section 20.3
  WDTCN = 0xDE; //First key
  WDTCN = 0xAD; //Second key
}


void sys_init() {
  disable_watchdog();
  XBR2    = XBR2_XBARE__ENABLED;      // Enable crossbar and weak pull-ups

  P0MDOUT |= P0MDOUT_B3__PUSH_PULL;
  P0MDOUT |= P0MDOUT_B4__PUSH_PULL;
  P0MDOUT |= P0MDOUT_B5__PUSH_PULL;

  // Motor output
  P0MDOUT |= P0MDOUT_B1__PUSH_PULL;  // Right
  P0MDOUT |= P0MDOUT_B2__PUSH_PULL;  // Right
  P1MDOUT |= P1MDOUT_B0__PUSH_PULL;  // Left
  P1MDOUT |= P1MDOUT_B1__PUSH_PULL;  // Left

  #ifdef DEV_BOARD
    // SPI - Local devboard
    P1MDOUT |= P1MDOUT_B3__PUSH_PULL;
    P1MDOUT |= P1MDOUT_B5__PUSH_PULL;
    P1MDOUT |= P1MDOUT_B6__PUSH_PULL;
    P1MDOUT |= P1MDOUT_B7__PUSH_PULL;
    P1MDIN  |= P1MDIN_B2__DIGITAL;  // MISO
  #else
    // SPI - BeeNrf
    P1MDOUT |= P1MDOUT_B2__PUSH_PULL; // CE
    P1MDOUT |= P1MDOUT_B3__PUSH_PULL; // CS
    P1MDOUT |= P1MDOUT_B4__PUSH_PULL; // SCK
    P1MDOUT |= P1MDOUT_B5__PUSH_PULL; // MOSI
    P1MDIN  |= P1MDIN_B6__DIGITAL;    // MISO

  #endif

  // We don't divide the clock to get 24.5 MHz
  CLKSEL = CLKSEL_CLKDIV__SYSCLK_DIV_1;

  // Setup Timer 1, which is used for sys_tick.
  CKCON0 = CKCON0_T1M__PRESCALE | CKCON0_SCA__SYSCLK_DIV_48;  // Clock Control, Divide SYSCLK with 48
  TMOD   = TMOD_T1M__MODE1;                           // Timer 0/1 Mode, Set to Mode 1: 16-bit timer
  TCON   = TCON_TR1__RUN;                           // Timer 0/1 Control, Enabled Timer 1

  // Enable interrupts
  IE = IE_EA__ENABLED |
       IE_ET1__ENABLED |
       IE_EX0__ENABLED; // External interrupt for button reset

  IT01CF |= IT01CF_IN0PL__ACTIVE_LOW | IT01CF_IN0SL__P0_7; // Button active low
}

// -- Interrupts -- //
SI_INTERRUPT(TIMER1_ISR, TIMER1_IRQn) {
  //P1_B6 = !P1_B6;
  sys_tick++;

  #define COUNT 510

  // Count to 510: ~1ms
  TCON &= ~TCON_TR1__RUN;
  TH1 = (0xffff - COUNT) >> 8;
  TL1 = (0xffff - COUNT) & 0xff;
  TCON |= TCON_TR1__RUN;
}


SI_INTERRUPT(INT0_ISR, INT0_IRQn) {
  // Set bit to 1 to perform software reset
  RSTSRC |= RSTSRC_SWRSF__SET;
}


#ifndef OPTIMIZE_MEMORY
void pinMode(gpio_e pin, pin_mode_e mode) {

  uint8_t bit = mode & 0x0f;    // Remove first part of hex; eg: 0x11 -> 0x01

  if (mode < LIMIT_PIN_MODE_OUTPUT) {
    if (pin < LIMIT_PIN_P0) {
      set_bit(P0MDOUT, bit, HIGH);
    } else if (pin < LIMIT_PIN_P1) {
      set_bit(P1MDOUT, bit, HIGH);
    } else {
      set_bit(P2MDOUT, bit, HIGH);
    }
  } else {
    if (pin < LIMIT_PIN_P0) {
      set_bit(P0MDIN, bit, HIGH);
    } else if (pin < LIMIT_PIN_P1) {
      set_bit(P1MDIN, bit, HIGH);
    } else {
      // Can't set P2 as input
    }
  }
}

void digitalWrite(uint8_t pin, uint8_t value) {
  uint8_t bit = pin & 0x0f;    // Remove first part of hex; eg: 0x11 -> 0x01

  if (pin < LIMIT_PIN_P0) {
    set_bit(P0MDOUT, pin, value == HIGH);
  } else if (pin < LIMIT_PIN_P1) {
    set_bit(P1MDOUT, pin, value == HIGH);
  } else {
    set_bit(P2MDOUT, pin, value == HIGH);
  }
}


uint64_t micros() {
  //return sys_tick * SYS_TICK_MICROS;
  return sys_tick;
}
#endif