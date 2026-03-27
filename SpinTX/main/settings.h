#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef void (*settings_cb_t)(void);

void settings_init(void);
void settings_add_callback(settings_cb_t callback);

float settings_get_wheel_diameter(void);
void settings_set_wheel_diameter(float diameter);