#include "scheduler.h"
#include "task.h"

// Tasks
#include "radio.h"
#include "motor_control.h"
#include "led.h"

task_t tasks[] = {
    {   // RADIO
        .init      = radio_init,
        .update    = radio_update,
        .period_ms = 20
    },
    {   // Motors
        .init      = motor_control_init,
        .update    = motor_control_update,
        .period_ms = 20
    },
    {   // Led
        .init      = led_init,
        .update    = led_update,
        .period_ms = 20
    },
};

#define NBR_OF_TASKS (sizeof(tasks) / sizeof(task_t))

static task_t* get_next_task() {
    task_t* t;
    uint32_t now = millis();
    for (uint8_t i = 0; i < NBR_OF_TASKS; i++) {
        t = &tasks[i];
        if (((uint16_t) (now - t->last_completed)) > t->period_ms) {
            return t;
        }
    }
    return &tasks[0];
}

void scheduler_run() {
    while (1) {
        task_t* next_task = get_next_task();
        next_task->update();
        next_task->last_completed = millis();
    }
}
