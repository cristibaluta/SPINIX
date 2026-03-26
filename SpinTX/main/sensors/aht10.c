#include "aht10.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define AHT_ADDR  0x38

i2c_master_dev_handle_t devHandle;


void configureAht(i2c_master_bus_handle_t i2cBus) {
	
	i2c_device_config_t devCfg = {
	    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
	    .device_address = AHT_ADDR,
	    .scl_speed_hz = 100000,
	};
	ESP_ERROR_CHECK(i2c_master_bus_add_device(i2cBus, &devCfg, &devHandle));
}

esp_err_t ahtRead(float *temperature, float *humidity) {

	uint8_t cmd[] = {0xAC, 0x33, 0x00};   // trigger measurement
	ESP_ERROR_CHECK(i2c_master_transmit(devHandle, cmd, sizeof(cmd), -1));
	
    vTaskDelay(pdMS_TO_TICKS(80));
	
	uint8_t result[6];
	ESP_ERROR_CHECK(i2c_master_receive(devHandle, result, sizeof(result), -1));

    uint32_t hum_raw = ((uint32_t)result[1] << 12) |
                       ((uint32_t)result[2] << 4) |
                       ((result[3] >> 4) & 0x0F);
    uint32_t temp_raw = ((uint32_t)(result[3] & 0x0F) << 16) |
                        ((uint32_t)result[4] << 8) |
                        result[5];

    *humidity = hum_raw * 100.0f / 1048576.0f;
    *temperature = temp_raw * 200.0f / 1048576.0f - 50.0f;

    return ESP_OK;
}