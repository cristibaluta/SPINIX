// Capacitive soil moisture sensor

#ifndef SOIL_MOIST_CAP_H
#define SOIL_MOIST_CAP_H

#include "esp_adc/adc_oneshot.h"
// #include "esp_adc/adc_continuous.h"

adc_oneshot_unit_handle_t configure_moisture_sensor();
float read_soil_moisture(adc_oneshot_unit_handle_t handle);

#endif