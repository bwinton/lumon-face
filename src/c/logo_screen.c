#include "logo_screen.h"
#include "constants.h"
#include <pebble.h>

static Window *s_logo_window;

static void prv_window_load(Window *window) {
    window_set_background_color(window, DARK_BLUE);
}

static void prv_window_unload(Window *window) {
    // Nothing to clean up
}

void logo_init(void) {
    s_logo_window = window_create();
    window_set_window_handlers(s_logo_window, (WindowHandlers) {
        .load = prv_window_load,
        .unload = prv_window_unload,
    });
}

void logo_show(void) {
    window_stack_push(s_logo_window, true);
}

void logo_hide(void) {
    window_stack_pop(true);
}

bool logo_is_showing(void) {
    Window *top_window = window_stack_get_top_window();
    return top_window == s_logo_window;
}

void logo_deinit(void) {
    window_destroy(s_logo_window);
}