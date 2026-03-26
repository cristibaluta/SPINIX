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

// call once at startup before any task starts
void data_init(void);

void data_set_gps(float lat, float lon, float elevation, float speed);
void data_set_temperature(float temp, float humidity);
void data_set_heart_rate(uint8_t hr);
void data_set_cadence(uint8_t cadence);

// Returns a copy of the data
bike_data_t data_get_snapshot(void);