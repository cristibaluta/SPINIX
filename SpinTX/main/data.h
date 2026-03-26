#pragma once
#include <stdio.h>

typedef struct {
    float lat;
    float lon;
    float elevation;
    float speed;
    float temperature;
    float humidity;
    float barometer;
    uint8_t heart_rate;
    uint8_t cadence;
} bike_data_t;

typedef enum {
    LED_COLOR_NONE = 0,
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
} led_color_t;

extern volatile led_color_t g_led_color;

// call once at startup before any task starts
void data_init(void);

void data_set_gps(float lat, float lon, float elevation, float speed);
void data_set_temperature(float temp, float humidity);
void data_set_heart_rate(uint8_t hr);
void data_set_cadence(uint8_t cadence);

// Returns a copy of the data
bike_data_t data_get_snapshot(void);