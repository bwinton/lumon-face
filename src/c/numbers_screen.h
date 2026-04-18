#pragma once

#include <pebble.h>

void numbers_screen_init(GRect bounds);
void numbers_screen_add_children(Layer *window_layer);
void numbers_screen_deinit(void);
void numbers_screen_update_time(void);
void numbers_screen_update_animation(void);
bool numbers_screen_minute_transition_active(void);