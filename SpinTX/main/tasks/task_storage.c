#include "task_storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "storage.h"
#include "data.h"

static const char *TAG = "TASK_STORAGE";

void task_storage(void *params) {
    ESP_LOGI(TAG, "------- init task storage");

    if (!storage_init()) {
        ESP_LOGE(TAG, "storage init failed, task exiting");
        vTaskDelete(NULL);
        return;
    }

    if (!storage_open_track()) {
        ESP_LOGE(TAG, "failed to open track, task exiting");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        bike_data_t d = data_get_snapshot();

        char line[128];
        snprintf(line, sizeof(line),
                 "%.6f,%.6f,%.1f,%.1f,%i,%lu",
                 d.lat, d.lon,
                 d.altitude,
                 d.speed,
                 d.cadence,
                 (unsigned long)d.timestamp);

        if (!storage_write_line(line)) {
            ESP_LOGE(TAG, "write failed");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}