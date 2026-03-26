#pragma once

#define AHT_TYPE AHT_TYPE_AHT1x
#define AHT_TAG "AHT_TAG"
#define ADDR AHT_I2C_ADDRESS_GND

typedef struct {
    float temperature;
    float humidity;
} aht_data;

bool configure_aht();
aht_data read_aht();
