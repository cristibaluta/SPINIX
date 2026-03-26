/* NMEA Parser example, that decode data stream from GPS receiver

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include "task_gps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nmea_parser.h"
#include <dps310.h>
#include "data.h"
#include "pins.h"

#define TIME_ZONE (+8)   //Beijing Time
#define YEAR_BASE (2000) //date in GPS starts from 2000

static dps310_t dps_dev = {0};

/**
 * @brief GPS Event Handler
 *
 * @param event_handler_arg handler specific arguments
 * @param event_base event base, here is fixed to ESP_NMEA_EVENT
 * @param event_id event id
 * @param event_data event specific arguments
 */
static void gps_event_handler(void *event_handler_arg, 
                                esp_event_base_t event_base, 
                                int32_t event_id, 
                                void *event_data)
{
    gps_t *gps = NULL;
    switch (event_id) {
    case GPS_UPDATE:
        gps = (gps_t *)event_data;
        /* print information parsed from GPS statements */
        ESP_LOGI(GPS_TAG, "%d/%d/%d %d:%d:%d => \r\n"
                 "\t\t\t\t\t\tlatitude   = %.05f°N\r\n"
                 "\t\t\t\t\t\tlongitude = %.05f°E\r\n"
                 "\t\t\t\t\t\taltitude   = %.02fm\r\n"
                 "\t\t\t\t\t\tspeed      = %fm/s",
                 gps->date.year + YEAR_BASE, gps->date.month, gps->date.day,
                 gps->tim.hour + TIME_ZONE, gps->tim.minute, gps->tim.second,
                 gps->latitude, gps->longitude, gps->altitude, gps->speed);

        data_set_gps(gps->latitude, gps->longitude, gps->altitude, gps->speed);
        break;
    case GPS_UNKNOWN:
        /* print unknown statements */
        ESP_LOGW(GPS_TAG, "Unknown statement:%s", (char *)event_data);
        break;
    default:
        break;
    }
}

void task_gps(void *params) {
    ESP_LOGI(GPS_TAG, "------- init task GPS");

    // init DPS310 over I2C
    // TODO there are two addresses we can use, check which one is valid
    dps310_config_t dps_config = DPS310_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(dps310_init_desc(&dps_dev, DPS310_I2C_ADDRESS_0, 0, PIN_I2C_SDA, PIN_I2C_SCL));
    ESP_ERROR_CHECK(dps310_init(&dps_dev, &dps_config));

    // init GPS NMEA
    nmea_parser_config_t nmea_config = NMEA_PARSER_CONFIG_DEFAULT();
    nmea_parser_handle_t nmea_hdl = nmea_parser_init(&nmea_config);
    nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);

    while (1) {
        float pressure;
        if (dps310_read_pressure(&dps_dev, &pressure) == ESP_OK) {
            ESP_LOGI(GPS_TAG, "pressure=%.2f hPa", pressure);
            data_set_barometer(pressure);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
