#pragma once
#include <stdbool.h>

bool storage_init(void);
bool storage_open_track(void);
bool storage_write_line(const char *line);
void storage_close_track(void);