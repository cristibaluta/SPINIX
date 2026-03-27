#pragma once
#include <stdio.h>

typedef struct {
    float lat;
    float lon;
    float altitude;
    float speed;
    uint8_t cadence;
    float temperature;
    float humidity;
    float pressure;
    uint8_t heart_rate;
    unsigned long timestamp;
} bike_data_t;

typedef enum {
    LED_COLOR_NONE = 0,
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
} led_color_t;

extern volatile led_color_t g_led_color;
