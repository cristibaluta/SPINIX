#include "task_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "data.h"

void task_display(void *param) {
    while (1) {
        bike_data_t d = data_get_snapshot();
        // display_draw_speed(d.speed);
        // display_draw_coords(d.lat, d.lon);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
