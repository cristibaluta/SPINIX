#include "hall.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_attr.h"

static void IRAM_ATTR hall_isr_handler(void *arg) {
    hall_sensor_t *sensor = (hall_sensor_t *)arg;
    int64_t now = esp_timer_get_time();
    if (sensor->last_pulse_time > 0) {
        sensor->interval_us = now - sensor->last_pulse_time;
    }
    sensor->last_pulse_time = now;
}

void hall_sensor_init(hall_sensor_t *sensor, uint8_t pin) {
    sensor->pin = pin;
    sensor->last_pulse_time = 0;
    sensor->interval_us = 0;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(pin, hall_isr_handler, sensor);  // pass sensor as arg
}

bool hall_sensor_is_stopped(hall_sensor_t *sensor, int64_t timeout_us) {
    if (sensor->last_pulse_time == 0) return true;
    return (esp_timer_get_time() - sensor->last_pulse_time) > timeout_us;
}

int64_t hall_sensor_get_interval_us(hall_sensor_t *sensor) {
    return sensor->interval_us;
}