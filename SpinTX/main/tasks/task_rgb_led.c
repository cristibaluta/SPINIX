#include "task_rgb_led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "pins.h"
#include "led_strip.h"

volatile led_color_t g_led_color = LED_COLOR_NONE;

void configure_led() {
	// LED output
    gpio_config_t io_conf_led = {
        .pin_bit_mask = (1ULL << PIN_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_led);
}

void task_rgb_led(void *params) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = PIN_LED_RGB,
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
