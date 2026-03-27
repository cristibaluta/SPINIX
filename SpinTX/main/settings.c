#include "settings.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#define NVS_NAMESPACE   "settings"
#define KEY_WHEEL_DIAM  "wheel_diam"
#define DEFAULT_WHEEL_DIAMETER 0.67f  // 700c road bike in meters
#define SETTINGS_MAX_CALLBACKS 5

static const char *TAG = "SETTINGS";

static settings_cb_t s_callbacks[SETTINGS_MAX_CALLBACKS] = {0};
static int s_callback_count = 0;

void settings_add_callback(settings_cb_t callback) {
    if (s_callback_count < SETTINGS_MAX_CALLBACKS) {
        s_callbacks[s_callback_count++] = callback;
    } else {
        ESP_LOGW(TAG, "max callbacks reached, ignoring");
    }
}

static void notify_callbacks(void) {
    for (int i = 0; i < s_callback_count; i++) {
        if (s_callbacks[i] != NULL) {
            s_callbacks[i]();
        }
    }
}

void settings_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        nvs_flash_erase();
        nvs_flash_init();
    }
    ESP_LOGI(TAG, "NVS initialized");
}

float settings_get_wheel_diameter(void) {
    nvs_handle_t handle;
    float diameter = DEFAULT_WHEEL_DIAMETER;

    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_OK) {
        // NVS doesn't support float natively, store as uint32 blob
        uint32_t raw = 0;
        err = nvs_get_u32(handle, KEY_WHEEL_DIAM, &raw);
        if (err == ESP_OK) {
            memcpy(&diameter, &raw, sizeof(float));
        } else {
            ESP_LOGI(TAG, "wheel diameter not set, using default %.2fm", diameter);
        }
        nvs_close(handle);
    }
    
    return diameter;
}

void settings_set_wheel_diameter(float diameter) {
    // Save to NVS
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        uint32_t raw = 0;
        memcpy(&raw, &diameter, sizeof(float));
        nvs_set_u32(handle, KEY_WHEEL_DIAM, raw);
        nvs_commit(handle);
        nvs_close(handle);
        ESP_LOGI(TAG, "wheel diameter saved: %.2fm", diameter);
    }

    notify_callbacks();
}