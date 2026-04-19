#include "logo_screen.h"
#include "logo_animation.h"
#include "constants.h"

static Window *s_logo_window;
static Layer *s_logo_layer;

static void prv_window_load(Window *window)
{
    window_set_background_color(window, DARK_BLUE);

    GRect bounds = layer_get_bounds(window_get_root_layer(window));
    s_logo_layer = layer_create(bounds);
    layer_add_child(window_get_root_layer(window), s_logo_layer);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Logo loading");
    start_logo_animation(s_logo_layer);
}

static void prv_window_unload(Window *window)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Logo unloading");
    stop_logo_animation();
    if (s_logo_layer)
    {
        layer_destroy(s_logo_layer);
        s_logo_layer = NULL;
    }
}

void logo_init(void)
{
    s_logo_window = window_create();
    window_set_window_handlers(s_logo_window, (WindowHandlers){
                                                  .load = prv_window_load,
                                                  .unload = prv_window_unload,
                                              });
}

void logo_show(void)
{
    window_stack_push(s_logo_window, true);
}

void logo_hide(void)
{
    window_stack_pop(true);
}

bool logo_is_showing(void)
{
    Window *top_window = window_stack_get_top_window();
    return top_window == s_logo_window;
}

void logo_deinit(void)
{
    window_destroy(s_logo_window);
}
