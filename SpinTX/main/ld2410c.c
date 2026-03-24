#include "ld2410c.h"
#include "driver/gpio.h"

int gpio_pin;

void configure_presence_sensor(int pin) {
    gpio_pin = pin;
	// Configure the presence pin as input with pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,   // active-high logic, adjust if needed
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

int get_presence_level() {

    int level = gpio_get_level(gpio_pin);
    return level;
}
