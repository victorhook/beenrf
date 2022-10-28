#include "led.h"
#include "core.h"

#ifndef DEV_BOARD
  static void zero();

  static void one();
#endif

#define LED_HIGH() (PIN_LED = HIGH)

#define LED_LOW()  (PIN_LED = LOW)

#define INIT_BLINK_DELAY_MS 200

status_t led_init() {
  #ifdef DEV_BOARD
    P1MDOUT |= P1MDOUT_B4__PUSH_PULL;
    led_set(0); delay(INIT_BLINK_DELAY_MS); led_set(1); delay(INIT_BLINK_DELAY_MS);
    led_set(0); delay(INIT_BLINK_DELAY_MS); led_set(1); delay(INIT_BLINK_DELAY_MS);
    led_set(0); delay(INIT_BLINK_DELAY_MS); led_set(1); delay(INIT_BLINK_DELAY_MS);
    led_set(0);
  #else
    P1MDOUT |= P1MDOUT_B3__PUSH_PULL;
    led_set(0x020000); delay(INIT_BLINK_DELAY_MS); led_set(0x000000); delay(INIT_BLINK_DELAY_MS);
    led_set(0x020000); delay(INIT_BLINK_DELAY_MS); led_set(0x000000); delay(INIT_BLINK_DELAY_MS);
    led_set(0x020000); delay(INIT_BLINK_DELAY_MS); led_set(0x000000); delay(INIT_BLINK_DELAY_MS);
    led_set(0x000000);
  #endif


  return STATUS_OK;
}


void led_set(uint32_t color) {
  #ifdef DEV_BOARD
    PIN_LED = !color;
  #else
  uint8_t buf[24];

  for (uint8_t i = 0; i < 24; i++) {
    buf[23-i] = (color & 0x01) ? 1 : 0;
    color = color >> 1;
  }

  cli();
  for (uint8_t i = 0; i < 24; i++) {
    buf[i] ? one() : zero();
  }
  sei();
  LED_LOW();
  #endif
}

void led_update() {

}

#ifndef DEV_BOARD

static void zero() __naked {
  LED_HIGH();
  __asm
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  __endasm;
  LED_LOW();
  __asm
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  ret
  __endasm;
}

static void one() __naked {
  LED_HIGH();
  __asm
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  __endasm;
  LED_LOW();
  __asm
  nop
  ret
  __endasm;
}

#endif