#pragma once

#include <pebble.h>

void minute_transition_init(GRect bounds, int cell_height);
void minute_transition_add_children(Layer *window_layer);
void minute_transition_deinit(void);

void minute_transition_maybe_start(const struct tm *tick_time, TextLayer *grid_layers[], char grid_buffer[][2]);
void minute_transition_update_animation(TextLayer *grid_layers[], const GRect grid_orig[30]);

bool minute_transition_active(void);
bool minute_transition_should_display_old_digits(void);
bool minute_transition_should_display_new_digits(void);
