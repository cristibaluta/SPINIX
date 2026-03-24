// sensors.h
#ifndef SENSORS_H
#define SENSORS_H

#include "aht10.h"

typedef struct {
    float temperature;
    float humidity;
    float barometer;
    float speed;
    float cadence;
    float lat;
    float lon;
    float alt;
} sensor_data_t;

extern sensor_data_t g_sensor_data;

#endif