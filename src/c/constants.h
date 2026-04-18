#pragma once

#include <pebble.h>

// Colors
#define DARK_BLUE GColorCobaltBlue
#define LIGHT_BLUE GColorCeleste

// Grid configuration
#define GRID_COLS 6
#define GRID_ROWS 5
#define GRID_COUNT (GRID_COLS * GRID_ROWS)

// Time positions in grid (0-based indices)
static const int TIME_POSITIONS[4] = {13, 14, 15, 16};