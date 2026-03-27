#include "task_speed_cadence.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"
#include "esp_log.h"
#include "settings.h"
#include "hall.h"
#include "data.h"
#include "pins.h"

//  The time in microseconds after the last pulse before you consider the wheel or crank as stopped.
#define WHEEL_STOPPED_TIMEOUT_US 3000000  // 3 seconds → below ~2.5 km/h
#define CRANK_STOPPED_TIMEOUT_US 3000000  // 3 seconds → below ~20 RPM

static volatile float s_circumference = 0.0f;

static void on_settings_changed(void) {
    float diameter = settings_get_wheel_diameter();
    s_circumference = M_PI * diameter;
    ESP_LOGI(SPEED_CADENCE_TAG, "wheel diameter=%.2fm circumference=%.3fm", diameter, s_circumference);
}

void task_speed_cadence(void *params) {
    ESP_LOGI(SPEED_CADENCE_TAG, "------- init task speed/cadence");

    settings_add_callback(on_settings_changed);
    on_settings_changed();  // init with current value
    
    hall_sensor_t wheel = {0};
    hall_sensor_t crank = {0};
    hall_sensor_init(&wheel, PIN_HALL_WHEEL);
    hall_sensor_init(&crank, PIN_HALL_CRANK);

    while (1) {
        float speed = 0.0f;
        float cadence = 0.0f;

        if (!hall_sensor_is_stopped(&wheel, WHEEL_STOPPED_TIMEOUT_US)) {
            float rps = 1000000.0f / hall_sensor_get_interval_us(&wheel);
            speed = rps * s_circumference * 3.6f;
        }

        if (!hall_sensor_is_stopped(&crank, CRANK_STOPPED_TIMEOUT_US)) {
            float rps = 1000000.0f / hall_sensor_get_interval_us(&crank);
            cadence = rps * 60.0f;
        }

        ESP_LOGI(SPEED_CADENCE_TAG, "speed=%.1f km/h  cadence=%.0f rpm", speed, cadence);
        data_set_speed(speed);
        data_set_cadence(cadence);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}