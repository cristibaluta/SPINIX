#include "task_storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "data.h"
// Non-volative storage
#include "nvs_flash.h"

void task_storage(void *param) {
    while (1) {
        bike_data_t d = data_get_snapshot();  // atomic copy
        // storage_write_entry(d);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
