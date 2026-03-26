#ifndef AHT10_H
#define AHT10_H

#include "driver/i2c_master.h"
#include "esp_err.h" 

void configureAht(i2c_master_bus_handle_t i2cBus);
esp_err_t ahtRead(float *temperature, float *humidity);

#endif