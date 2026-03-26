#include "data.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static bike_data_t s_data = {0};
static SemaphoreHandle_t s_mutex = NULL;

void data_init(void) {
    s_mutex = xSemaphoreCreateMutex();
}

void data_set_gps(float lat, float lon, float elevation, float speed) {
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_data.lat = lat;
        s_data.lon = lon;
        s_data.elevation = elevation;
        s_data.speed = speed;
        xSemaphoreGive(s_mutex);
    }
}

void data_set_temperature(float temp, float humidity) {
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_data.temperature = temp;
        s_data.humidity = humidity;
        xSemaphoreGive(s_mutex);
    }
}

void data_set_barometer(float pressure) {
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_data.pressure = pressure;
        xSemaphoreGive(s_mutex);
    }
}

void data_set_heart_rate(uint8_t hr) {
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_data.heart_rate = hr;
        xSemaphoreGive(s_mutex);
    }
}

void data_set_cadence(uint8_t cadence) {
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_data.cadence = cadence;
        xSemaphoreGive(s_mutex);
    }
}

// returns a full copy — caller works on the copy, not the live data
bike_data_t data_get_snapshot(void) {
    bike_data_t snapshot = {0};
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        snapshot = s_data;
        xSemaphoreGive(s_mutex);
    }
    return snapshot;
}