#include "aht20.h"
#include <aht.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pins.h"

aht_t dev = { 0 };

bool configure_aht() {
    printf("------- init task AHT\n");
    dev.mode = AHT_MODE_NORMAL;
    dev.type = AHT_TYPE;

    esp_err_t err;
    
    err = aht_init_desc(&dev, AHT_ADDR, 0, PIN_I2C_SDA, PIN_I2C_SCL);
    if (err != ESP_OK) {
        ESP_LOGW(AHT_TAG, "AHT descriptor init failed: %s", esp_err_to_name(err));
        return false;
    }

    err = aht_init(&dev);
    if (err != ESP_OK) {
        ESP_LOGW(AHT_TAG, "AHT init failed: %s", esp_err_to_name(err));
        return false;
    }

    bool calibrated;
    err = aht_get_status(&dev, NULL, &calibrated);
    if (err != ESP_OK) {
        ESP_LOGW(AHT_TAG, "AHT status read failed: %s", esp_err_to_name(err));
        return false;
    }

    if (calibrated)
        ESP_LOGI(AHT_TAG, "Sensor calibrated");
    else
        ESP_LOGW(AHT_TAG, "Sensor not calibrated!");
    
    return true;
}

aht_data_t read_aht() {
    float temperature, humidity;
    esp_err_t res = aht_get_data(&dev, &temperature, &humidity);
    if (res == ESP_OK)
        ESP_LOGI(AHT_TAG, "Temperature: %.1f°C, Humidity: %.2f%%", temperature, humidity);
    else
        ESP_LOGE(AHT_TAG, "Error reading data: %d (%s)", res, esp_err_to_name(res));

    return (aht_data_t) {
        .temperature = temperature,
        .humidity = humidity,
        .valid = res == ESP_OK
    };
}
