#include "storage.h"
#include <stdio.h>
#include <string.h>
#include "esp_flash.h"
#include "esp_partition.h"
#include "esp_flash_spi_init.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_idf_version.h"
#include "esp_chip_info.h"
#include "spi_flash_mmap.h"
#include "driver/spi_master.h"
#include "pins.h"

static const char *TAG = "STORAGE";
static FILE *s_track_file = NULL;

bool storage_init(void) {
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU cores, WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    printf("silicon revision %d, ", chip_info.revision);

    uint32_t size_flash_chip = 0;
    esp_flash_get_size(NULL, &size_flash_chip);
    printf("%uMB %s flash\n", (unsigned int)size_flash_chip >> 20,
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    // printf("Free heap: %u\n", (unsigned int) esp_get_free_heap_size());

    ESP_LOGI(TAG, "Initializing LittleFS");

    // register external flash on SPI bus
    const esp_flash_spi_device_config_t device_config = {
        .host_id = SPI2_HOST,
        .cs_id = 0,
        .cs_io_num = PIN_FLASH_CS,
        .io_mode = SPI_FLASH_DIO,
        .freq_mhz = 40,
    };

    esp_flash_t *ext_flash = NULL;
    ESP_ERROR_CHECK(spi_bus_add_flash_device(&ext_flash, &device_config));
    ESP_ERROR_CHECK(esp_flash_init(ext_flash));

    uint32_t flash_size;
    // Retrieve the size using the official API
    esp_err_t err = esp_flash_get_size(ext_flash, &flash_size);
    if (err == ESP_OK) {

    }

    // register as a partition
    ESP_ERROR_CHECK(esp_partition_register_external(
        ext_flash, 0, flash_size,
        "spin-tx", ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL));

    // mount LittleFS on it
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "spin-tx",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL)  {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d Kb, used: %d Kb", total/1024, used/1024);
    }

    return true;
}

bool storage_open_track(void) {
    // create a new file named by timestamp or incrementing index
    // for now use a fixed name for testing
    s_track_file = fopen("/littlefs/track.csv", "a");  // append mode
    if (!s_track_file) {
        ESP_LOGE(TAG, "failed to open track file");
        return false;
    }
    ESP_LOGI(TAG, "track file opened");
    return true;
}

bool storage_write_line(const char *line) {
    if (!s_track_file) return false;
    int written = fprintf(s_track_file, "%s\n", line);
    fflush(s_track_file);  // make sure it hits the flash
    return written > 0;
}

void storage_close_track(void) {
    if (s_track_file) {
        fclose(s_track_file);
        s_track_file = NULL;
        ESP_LOGI(TAG, "track file closed");
    }
}