#pragma once

#include <pebble.h>

void grid_motion_init(TextLayer *grid_layers[], const GRect grid_orig[30], const int time_positions[4], int time_positions_count);
void grid_motion_update(bool skip_time_digits);
void grid_motion_reset_offsets_at_indices(const int indices[], int count);
void grid_motion_deinit(void);
