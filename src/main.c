#include "core.h"
#include "radio.h"
#include "led.h"
#include "motor_control.h"


// --- Note: For sdcc, we MUST define all ISRs in main file! --- #

/* Main systick timer */
SI_INTERRUPT(TIMER1_ISR, TIMER1_IRQn);

/* Button reset */
SI_INTERRUPT(INT0_ISR, INT0_IRQn);

/* PWM Interrupt */
SI_INTERRUPT(PCA0_ISR, PCA0_IRQn);


int main() {
  sys_init();
  led_init();
  radio_init();
  motor_control_init();

  while (1) {
    radio_update();
    motor_control_update();
    led_set(0);
  }

  return 0;
}
