#include "logo_screen.h"
#include "constants.h"
#include <pebble.h>

static Window *s_logo_window;
static Layer *s_logo_layer;
static AppTimer *s_logo_timer;
static int s_logo_frame;
static const int s_logo_frame_count = 40;
static const int s_logo_frame_ms = 50;

static void prv_logo_layer_update(Layer *layer, GContext *ctx)
{
    const int frame = s_logo_frame;
    if (frame <= 0)
    {
        return;
    }

    const GRect bounds = layer_get_bounds(layer);
    const int center_x = bounds.size.w / 2;
    const int center_y = bounds.size.h / 2;
    const int full_w = bounds.size.w * 7 / 10;
    const int full_h = bounds.size.h * 2 / 9;
    const int gap = full_w / 4;

    graphics_context_set_stroke_color(ctx, LIGHT_BLUE);

    const int outer_phase = 12;
    const int inner_phase = 12;
    const int center_phase = 10;

    if (frame > 0)
    {
        int progress = frame;
        if (progress > outer_phase)
        {
            progress = outer_phase;
        }
        int draw_w = full_w * progress / outer_phase;
        int draw_h = full_h * progress / outer_phase;
        GRect outer_rect = GRect(center_x - draw_w / 2,
                                 center_y - draw_h / 2,
                                 draw_w,
                                 draw_h);
        graphics_draw_round_rect(ctx, outer_rect, draw_h / 2);
    }

    if (frame > outer_phase)
    {
        int progress = frame - outer_phase;
        if (progress > inner_phase)
        {
            progress = inner_phase;
        }
        int inner_w = full_w * 75 / 100;
        int inner_h = full_h * 90 / 100;
        int draw_w = inner_w * progress / inner_phase;
        int draw_h = inner_h * progress / inner_phase;
        GRect inner_rect = GRect(center_x - draw_w / 2,
                                 center_y - draw_h / 2,
                                 draw_w,
                                 draw_h);
        graphics_draw_round_rect(ctx, inner_rect, draw_h / 2);
    }

    if (frame > outer_phase + inner_phase)
    {
        int progress = frame - outer_phase - inner_phase;
        if (progress > center_phase)
        {
            progress = center_phase;
        }
        int center_w = full_w * 40 / 100;
        int center_h = full_h * 25 / 100;
        int draw_w = center_w * progress / center_phase;
        int draw_h = center_h * progress / center_phase;
        GRect center_rect = GRect(center_x - draw_w / 2,
                                  center_y - draw_h / 2,
                                  draw_w,
                                  draw_h);
        graphics_draw_round_rect(ctx, center_rect, draw_h / 2);
    }
}

static void prv_logo_animation_timer(void *context)
{
    s_logo_frame++;
    layer_mark_dirty(s_logo_layer);
    if (s_logo_frame < s_logo_frame_count)
    {
        s_logo_timer = app_timer_register(s_logo_frame_ms, prv_logo_animation_timer, NULL);
    }
    else
    {
        s_logo_timer = NULL;
    }
}

static void prv_window_load(Window *window)
{
    window_set_background_color(window, DARK_BLUE);

    GRect bounds = layer_get_bounds(window_get_root_layer(window));
    s_logo_layer = layer_create(bounds);
    layer_set_update_proc(s_logo_layer, prv_logo_layer_update);
    layer_add_child(window_get_root_layer(window), s_logo_layer);

    s_logo_frame = 0;
    if (s_logo_timer)
    {
        app_timer_cancel(s_logo_timer);
        s_logo_timer = NULL;
    }
    s_logo_timer = app_timer_register(s_logo_frame_ms, prv_logo_animation_timer, NULL);
}

static void prv_window_unload(Window *window)
{
    if (s_logo_timer)
    {
        app_timer_cancel(s_logo_timer);
        s_logo_timer = NULL;
    }
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
