#pragma once

#include <pebble.h>

/**
 * Starts the sequential ellipse and line drawing animation.
 *
 * @param layer The layer that will be marked dirty and used for rendering.
 */
void start_logo_animation(Layer *layer);
void stop_logo_animation();