// This is a hall sensor

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    volatile int64_t last_pulse_time;
    volatile int64_t interval_us;
    uint8_t pin;
} hall_sensor_t;

void hall_sensor_init(hall_sensor_t *sensor, uint8_t pin);
int64_t hall_sensor_get_interval_us(hall_sensor_t *sensor);
bool hall_sensor_is_stopped(hall_sensor_t *sensor, int64_t timeout_us);