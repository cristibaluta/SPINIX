#pragma once

#include <stdint.h>
#include <stdbool.h>

void settings_init(void);
float settings_get_wheel_diameter(void);
void settings_set_wheel_diameter(float diameter);