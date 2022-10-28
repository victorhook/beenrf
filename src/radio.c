#include "radio.h"
#include "core.h"
#include "nrf24.h"
#include "led.h"
#include "state.h"
#include "string.h"


status_t radio_init() {
    nrf_init();
    return STATUS_OK;
}


void radio_update() {
    static uint8_t buf[32];
    uint8_t bytes_read = nrf_read(buf);
    if (bytes_read > 0) {

        radio_command_t cmd = buf[0];
        switch (cmd) {
            case RADIO_COMMAND_SETPOINT:
                memcpy(&state.target, &buf[1], sizeof(motor_control_t));
                led_set(1);
                break;
            default:
                led_set(0x000a00);
                break;
        }

        delay(20);
    }
}
