idf_component_register(
    SRCS
		"${srcs}"
        "main.c"
        "aht10.c"
		"ld2410c.c"
    INCLUDE_DIRS "."
	PRIV_REQUIRES bt nvs_flash esp_driver_i2c
	REQUIRES esp_driver_gpio esp_adc aht rc_ble
)