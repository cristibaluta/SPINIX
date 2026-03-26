#pragma once

#define LED_TAG "LED_TAG"

void task_rgb_led(void *params);

typedef enum {
    LED_COLOR_NONE = 0,
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
} led_color_t;

extern volatile led_color_t g_led_color;
