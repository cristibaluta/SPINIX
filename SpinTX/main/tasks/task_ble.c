#include "task_ble.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rc_ble.h"
#include "data.h"
#include "settings.h"
#include "task_rgb_led.h"

static const char *TAG = "BLE";

static void on_ble_command(const char *command, uint16_t conn_handle) {

    if (strncmp(command, "GET_GPS", 7) == 0) {
        g_transfer_req.requested = true;
        g_transfer_req.conn_handle = conn_handle;
    }
    else if (strncmp(command, "SET_WHEEL:", 10) == 0) {
        float diameter = atof(command + 10);  // skip "SET_WHEEL:" and parse the rest
        if (diameter > 0.3f && diameter < 1.5f) {  // sanity check
            settings_set_wheel_diameter(diameter);
            ESP_LOGI(TAG, "wheel diameter updated: %.3fm", diameter);
        }
    }
    else if (strncmp(command, "SET_BLUE", 8) == 0) {
        g_led_color = LED_COLOR_BLUE;
    }
    else if (strncmp(command, "SET_RED", 7) == 0) {
        g_led_color = LED_COLOR_RED;
    }
    else if (strncmp(command, "SET_GREEN", 9) == 0) {
        g_led_color = LED_COLOR_GREEN;
    }
}

void task_ble(void *params) {
    ESP_LOGI(TAG, "------- init");

    rc_ble_init("spn-t");
    rc_ble_set_command_callback(on_ble_command);

    while (1) {
        ESP_LOGI(TAG, ">>>>>> should send data? %d", g_transfer_req.requested);
        if (g_transfer_req.requested) {
            g_transfer_req.requested = false;
            send_gps_data(g_transfer_req.conn_handle);
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
