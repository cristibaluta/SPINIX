#include "task_weather.h"
#include "aht20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pins.h"

void task_weather(void *params) {
    printf("------- init weather task\n");
    bool success = configure_aht();
    if (!success) {
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        printf("Free stack 1: %u\n", uxTaskGetStackHighWaterMark(NULL));
        aht_data data = read_aht();

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
