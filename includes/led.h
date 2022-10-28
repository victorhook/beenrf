#ifndef LED_H
#define LED_H

#include "core.h"


status_t led_init();

void led_update();

void led_set(uint32_t color);


#endif /* LED_H */
