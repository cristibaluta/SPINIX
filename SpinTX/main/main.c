#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "led_strip.h"

#include "nvs_flash.h"

#include "rc_ble.h"
#include "ld2410c.h"
#include "sensors.h"
#include "buttons.h"
#include <aht.h>

#define SDA_GPIO  5
#define SCL_GPIO  6
#define LED_GPIO  8
#define BUTTON_GPIO 9
#define MOISTURE_ADC_CHANNEL 0
#define PRESENCE_GPIO 1

#define I2C_PORT  0
#define I2C_FREQ 100000
#define OLED_ADDR 0x3C

#define DEBOUNCE_MS   1000

#define ADDR AHT_I2C_ADDRESS_GND
#define AHT_TYPE AHT_TYPE_AHT1x
static const char *AHT_TAG = "AHT";

i2c_master_bus_handle_t i2c_bus;
i2c_master_dev_handle_t aht_handle;
adc_oneshot_unit_handle_t soil_handle;

typedef enum {
    LED_COLOR_NONE = 0,
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
} led_color_t;

static volatile led_color_t g_led_color = LED_COLOR_NONE;

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

void configure_oled() {
	// ssd1306_init_oled_i2c(OLED_ADDR, SDA_GPIO, SCL_GPIO);
}

// void configure_button() {
// 	// 1. Configure pin as input with pull-up
//     gpio_config_t io_conf = {
//         .pin_bit_mask = (1ULL << BUTTON_GPIO),
//         .mode = GPIO_MODE_INPUT,
//         .pull_up_en = GPIO_PULLUP_ENABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE,
//     };
//     gpio_config(&io_conf);
// }

void configure_led() {
	// LED output
    gpio_config_t io_conf_led = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_led);
}

// Sensors task
void task_aht(void *pvParameters)
{
    printf("------- init task AHT\n");
    aht_t dev = { 0 };
    dev.mode = AHT_MODE_NORMAL;
    dev.type = AHT_TYPE;

    esp_err_t err;
    
    err = aht_init_desc(&dev, ADDR, 0, SDA_GPIO, SCL_GPIO);
    if (err != ESP_OK) {
        ESP_LOGW(AHT_TAG, "AHT descriptor init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    err = aht_init(&dev);
    if (err != ESP_OK) {
        ESP_LOGW(AHT_TAG, "AHT init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    bool calibrated;
    err = aht_get_status(&dev, NULL, &calibrated);
    if (err != ESP_OK) {
        ESP_LOGW(AHT_TAG, "AHT status read failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    if (calibrated)
        ESP_LOGI(AHT_TAG, "Sensor calibrated");
    else
        ESP_LOGW(AHT_TAG, "Sensor not calibrated!");

    float temperature, humidity;

    while (1)
    {
        printf("Free stack 1: %u\n", uxTaskGetStackHighWaterMark(NULL));
        esp_err_t res = aht_get_data(&dev, &temperature, &humidity);
        if (res == ESP_OK)
            ESP_LOGI(AHT_TAG, "Temperature: %.1f°C, Humidity: %.2f%%", temperature, humidity);
        else
            ESP_LOGE(AHT_TAG, "Error reading data: %d (%s)", res, esp_err_to_name(res));

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Buttons task
void task_buttons(void *pvParameters)
{
    printf("------- init task buttons\n");
	configure_led();
    while(1) {
        // printf("Free stack 2: %u\n", uxTaskGetStackHighWaterMark(NULL));
		bool button_pressed = gpio_get_level(BUTTON_GPIO) == 0;
		if (button_pressed) {
            printf("-------- button press\n");
			gpio_set_level(LED_GPIO, 0);
		} else {
			gpio_set_level(LED_GPIO, 1);
		}
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void task_rgb_led(void *pvParameters) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = 1,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
    };
    led_strip_handle_t strip;
    led_strip_new_rmt_device(&strip_config, &rmt_config, &strip);

    led_color_t current_color = LED_COLOR_NONE;

    while (1) {
        if (g_led_color != current_color) {
            current_color = g_led_color;
            switch (current_color) {
                case LED_COLOR_RED:
                    led_strip_set_pixel(strip, 0, 255, 0, 0);
                    break;
                case LED_COLOR_GREEN:
                    led_strip_set_pixel(strip, 0, 0, 255, 0);
                    break;
                case LED_COLOR_BLUE:
                    led_strip_set_pixel(strip, 0, 0, 0, 255);
                    break;
                case LED_COLOR_NONE:
                    led_strip_clear(strip);
                    break;
            }
            led_strip_refresh(strip);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// BLE tasks
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
void task_ble(void *pvParameters) {
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


void app_main(void)
{
    printf("-------- init app\n");
	ESP_ERROR_CHECK(i2cdev_init());

    // This is for testing purposes. Run it first so it can delete the master bus and left free to use later
	i2c_scan(SDA_GPIO, SCL_GPIO);

    // xTaskCreatePinnedToCore(task_aht, AHT_TAG, configMINIMAL_STACK_SIZE, NULL, 5, NULL, tskNO_AFFINITY);
    // xTaskCreatePinnedToCore(task_buttons, "BUTTONS_TAG", configMINIMAL_STACK_SIZE, NULL, 5, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(task_ble, "BLE_TAG", configMINIMAL_STACK_SIZE*8, NULL, 5, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(task_rgb_led, "LED_TAG", configMINIMAL_STACK_SIZE, NULL, 5, NULL, tskNO_AFFINITY);

}
