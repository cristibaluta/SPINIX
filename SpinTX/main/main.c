#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "i2cdev.h"
#include "pins.h"

// Components


// Tasks
#include "task_ble.h"
#include "task_buttons.h"
#include "task_display.h"
#include "task_storage.h"
#include "task_rgb_led.h"
#include "task_weather.h"
#include "task_gps.h"


#define I2C_PORT  0
#define I2C_FREQ 100000
#define DEBOUNCE_MS 1000

// i2c_master_bus_handle_t i2c_bus;
// i2c_master_dev_handle_t aht_handle;
// adc_oneshot_unit_handle_t soil_handle;

void i2c_master_init() {
	// i2c_master_bus_config_t i2c_mst_config = {
	//     .clk_source = I2C_CLK_SRC_DEFAULT,
	//     .i2c_port = I2C_PORT,
	//     .sda_io_num = SDA_GPIO,
	//     .scl_io_num = SCL_GPIO,
	//     .glitch_ignore_cnt = 7,
	//     .flags.enable_internal_pullup = true,
	// };
	// ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus));
}

void i2c_scan(int sda, int scl) {
	// i2c_master_bus_handle_t bus;
    // i2c_master_bus_config_t cfg = {
    //     .i2c_port = I2C_NUM_0,
    //     .sda_io_num = sda,
    //     .scl_io_num = scl,
    //     .clk_source = I2C_CLK_SRC_DEFAULT,
    // };
    // ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &bus));
    // printf("Scanning SDA=%d SCL=%d\n", sda, scl);
    // for (int addr = 1; addr < 127; addr++) {
    //     if (i2c_master_probe(bus, addr, 100) == ESP_OK) {
    //         printf("Found device with address: 0x%02X\n", addr);
    //     }
    // }
    // i2c_del_master_bus(bus);
}

void app_main(void)
{
    printf("-------- init app\n");
	ESP_ERROR_CHECK(i2cdev_init());

    // This is for testing purposes. Run it first so it can delete the master bus and left free to use later
	i2c_scan(PIN_I2C_SDA, PIN_I2C_SCL);

    xTaskCreatePinnedToCore(task_weather, WEATHER_TAG, configMINIMAL_STACK_SIZE, NULL, 5, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(task_buttons, BUTTONS_TAG, configMINIMAL_STACK_SIZE, NULL, 5, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(task_ble, BLE_TAG, configMINIMAL_STACK_SIZE*8, NULL, 5, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(task_rgb_led, LED_TAG, configMINIMAL_STACK_SIZE, NULL, 5, NULL, tskNO_AFFINITY);

}
