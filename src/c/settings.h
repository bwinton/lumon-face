#pragma once

#include <pebble.h>

typedef void (*SettingsLowPowerChangedHandler)(void *context);

void settings_init(void);
void settings_deinit(void);
bool settings_low_power_mode(void);
void settings_register_low_power_changed_handler(SettingsLowPowerChangedHandler handler, void *context);
void settings_unregister_low_power_changed_handler(void);
