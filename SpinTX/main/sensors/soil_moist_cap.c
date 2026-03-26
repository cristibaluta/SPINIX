#include "soil_moist_cap.h"

adc_oneshot_unit_handle_t configure_moisture_sensor() {
	adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
	adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,  // 0–3.3V range
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    adc_oneshot_config_channel(adc_handle, MOISTURE_ADC_CHANNEL, &config);
    return adc_handle;
}

float read_soil_moisture(adc_oneshot_unit_handle_t handle) {
	int adc_raw = 0;
    adc_oneshot_read(handle, MOISTURE_ADC_CHANNEL, &adc_raw);
	float moisture_percent = (adc_raw / 4095.0) * 100.0;
	return moisture_percent;
}
