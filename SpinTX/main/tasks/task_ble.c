#include "task_ble.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rc_ble.h"
#include "data.h"
#include "task_rgb_led.h"

static void on_ble_command(const char *command, uint16_t conn_handle) {
    if (strncmp(command, "GET_GPS", 7) == 0) {
        g_transfer_req.requested = true;
        g_transfer_req.conn_handle = conn_handle;
    } else if (strncmp(command, "SET_BLUE", 8) == 0) {
        g_led_color = LED_COLOR_BLUE;
    } else if (strncmp(command, "SET_RED", 7) == 0) {
        g_led_color = LED_COLOR_RED;
    } else if (strncmp(command, "SET_GREEN", 9) == 0) {
        g_led_color = LED_COLOR_GREEN;
    }
}

void task_ble(void *params) {
    printf("------- init task BLE\n");
    
    rc_ble_init("spn-t");
    rc_ble_set_command_callback(on_ble_command);

    while (1) {
        printf(">>>>>> should send data? %d\n", g_transfer_req.requested);
        if (g_transfer_req.requested) {
            g_transfer_req.requested = false;
            send_gps_data(g_transfer_req.conn_handle);
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
