// Temperature and humdity sensor

#pragma once

#include <stdio.h>

#define AHT_TYPE AHT_TYPE_AHT1x
#define AHT_TAG "AHT_TAG"
#define AHT_ADDR AHT_I2C_ADDRESS_GND

typedef struct {
    float temperature;
    float humidity;
    bool valid;
} aht_data_t;

bool configure_aht();
aht_data_t read_aht();
