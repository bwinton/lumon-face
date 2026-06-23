#include "logo_screen.h"
#include "logo_animation.h"
#include "constants.h"
#include <pebble.h>

static Window *s_logo_window;
static Layer *s_logo_layer;

// Text layers
static TextLayer *s_text_layer_date;
static TextLayer *s_text_layer_battery;
static TextLayer *s_text_layer_heart;
static TextLayer *s_text_layer_steps;
static TextLayer *s_text_layer_compass;

// Screen enum
typedef enum
{
    SCREEN_DATE,
    SCREEN_BATTERY,
    SCREEN_HEART,
    SCREEN_STEPS,
    SCREEN_COMPASS,
    SCREEN_COUNT
} Screen;

// Persist last viewed screen across show/hide
static Screen s_current_screen = SCREEN_DATE;

// Data + availability flags
static int s_battery_level = -1;
static HealthValue s_step_count = -1;
static int s_heart_rate = -1;
static bool s_compass_ready = false;
static bool s_health_available = false;
static bool s_animation_played = false;

// ---------- Helpers ----------
static void hide_all_layers()
{
    layer_set_hidden(text_layer_get_layer(s_text_layer_date), true);
    layer_set_hidden(text_layer_get_layer(s_text_layer_battery), true);
    layer_set_hidden(text_layer_get_layer(s_text_layer_heart), true);
    layer_set_hidden(text_layer_get_layer(s_text_layer_steps), true);
    layer_set_hidden(text_layer_get_layer(s_text_layer_compass), true);
}

static bool screen_has_data(Screen screen)
{
    switch (screen)
    {
    case SCREEN_DATE:
        return true;
    case SCREEN_BATTERY:
        return s_battery_level >= 0;
    case SCREEN_HEART:
        return s_heart_rate >= 0;
    case SCREEN_STEPS:
        return s_step_count >= 0;
    case SCREEN_COMPASS:
        return s_compass_ready;
    default:
        return false;
    }
}

static Screen get_next_valid_screen(Screen start)
{
    Screen s = start;
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        s = (s + 1) % SCREEN_COUNT;
        if (screen_has_data(s))
            return s;
    }
    return SCREEN_DATE;
}

static Screen get_prev_valid_screen(Screen start)
{
    Screen s = start;
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        s = (s - 1 + SCREEN_COUNT) % SCREEN_COUNT;
        if (screen_has_data(s))
            return s;
    }
    return SCREEN_DATE;
}

// ---------- Updates ----------
static void update_date()
{
    if (s_current_screen != SCREEN_DATE)
        return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    static char buf[32];
    strftime(buf, sizeof(buf), "%d %b %Y", t);
    text_layer_set_text(s_text_layer_date, buf);
}

static void update_steps()
{
    if (!s_health_available)
        return;

    s_step_count = health_service_sum_today(HealthMetricStepCount);

    if (s_current_screen != SCREEN_STEPS)
        return;

    static char buf[32];
    snprintf(buf, sizeof(buf), "Steps: %d", (int)s_step_count);
    text_layer_set_text(s_text_layer_steps, buf);
}

static void update_heart_rate()
{
    if (!s_health_available)
        return;

    if (health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL)) & HealthServiceAccessibilityMaskAvailable)
    {
        s_heart_rate = (int)health_service_peek_current_value(HealthMetricHeartRateBPM);
    }

    if (s_current_screen != SCREEN_HEART)
        return;

    static char buf[32];
    if (s_heart_rate >= 0)
    {
        snprintf(buf, sizeof(buf), "HR: %d bpm", s_heart_rate);
    }
    else
    {
        snprintf(buf, sizeof(buf), "HR: N/A");
    }
    text_layer_set_text(s_text_layer_heart, buf);
}

static const char *get_compass_direction(int deg)
{
    static const char *dirs[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    return dirs[(deg + 22) / 45 % 8];
}

// ---------- Services ----------
static void battery_callback(BatteryChargeState state)
{
    s_battery_level = state.charge_percent;

    if (s_current_screen != SCREEN_BATTERY)
        return;

    static char buf[32];
    snprintf(buf, sizeof(buf), "Battery: %d%%", s_battery_level);
    text_layer_set_text(s_text_layer_battery, buf);
}

static void health_handler(HealthEventType event, void *context)
{
    if (event == HealthEventMovementUpdate)
        update_steps();
    if (event == HealthEventHeartRateUpdate)
        update_heart_rate();
}

static void compass_handler(CompassHeadingData heading)
{
    s_compass_ready = true;

    if (s_current_screen != SCREEN_COMPASS)
        return;

    int degrees = TRIGANGLE_TO_DEG(heading.true_heading);
    const char *dir = get_compass_direction(degrees);

    static char buf[32];
    snprintf(buf, sizeof(buf), "%s (%d°)", dir, degrees);
    text_layer_set_text(s_text_layer_compass, buf);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
    update_date();
    update_steps();
    update_heart_rate();
}

// ---------- Screen control ----------
static void show_current_screen()
{
    hide_all_layers();

    if (!screen_has_data(s_current_screen))
    {
        s_current_screen = get_next_valid_screen(s_current_screen);
    }

    switch (s_current_screen)
    {
    case SCREEN_DATE:
        layer_set_hidden(text_layer_get_layer(s_text_layer_date), false);
        update_date();
        break;
    case SCREEN_BATTERY:
        layer_set_hidden(text_layer_get_layer(s_text_layer_battery), false);
        battery_callback(battery_state_service_peek());
        break;
    case SCREEN_HEART:
        layer_set_hidden(text_layer_get_layer(s_text_layer_heart), false);
        update_heart_rate();
        break;
    case SCREEN_STEPS:
        layer_set_hidden(text_layer_get_layer(s_text_layer_steps), false);
        update_steps();
        break;
    case SCREEN_COMPASS:
        layer_set_hidden(text_layer_get_layer(s_text_layer_compass), false);
        break;
    default:
        break;
    }

    // Low power compass toggle
    if (s_current_screen == SCREEN_COMPASS)
    {
        compass_service_subscribe(compass_handler);
    }
    else
    {
        compass_service_unsubscribe();
    }
}

static void animation_complete(void *context)
{
    s_animation_played = true;
    Layer *layer = (Layer *)context;
    draw_logo_static(layer);
    show_current_screen();
}

// ---------- Window ----------
static void prv_window_load(Window *window)
{
    window_set_background_color(window, DARK_BLUE);

    Layer *root = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root);

    s_logo_layer = layer_create(bounds);
    layer_add_child(root, s_logo_layer);

    // Create text layers
    bounds.origin.y = bounds.size.h * 4 / 6;
    bounds.size.h = bounds.size.h / 3;
    s_text_layer_date = text_layer_create(bounds);
    s_text_layer_battery = text_layer_create(bounds);
    s_text_layer_heart = text_layer_create(bounds);
    s_text_layer_steps = text_layer_create(bounds);
    s_text_layer_compass = text_layer_create(bounds);

    TextLayer *layers[] = {
        s_text_layer_date,
        s_text_layer_battery,
        s_text_layer_heart,
        s_text_layer_steps,
        s_text_layer_compass};

    for (int i = 0; i < 5; i++)
    {
        text_layer_set_background_color(layers[i], GColorClear);
        text_layer_set_text_color(layers[i], LIGHT_BLUE);
        text_layer_set_text_alignment(layers[i], GTextAlignmentCenter);
        text_layer_set_font(layers[i], fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
        layer_add_child(root, text_layer_get_layer(layers[i]));
    }

    hide_all_layers();

    // Animation only first time
    if (!s_animation_played)
    {
        start_logo_animation(s_logo_layer);
        logo_animation_set_complete_callback(animation_complete, s_logo_layer);
    }
    else
    {
        draw_logo_static(s_logo_layer);
        show_current_screen();
    }

    s_health_available = health_service_any_activity_accessible(HealthMetricStepCount | HealthMetricHeartRateBPM, time_start_of_today(), time(NULL));

    battery_state_service_subscribe(battery_callback);

    if (s_health_available)
    {
        health_service_events_subscribe(health_handler, NULL);
    }

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void prv_window_unload(Window *window)
{
    stop_logo_animation();

    battery_state_service_unsubscribe();

    if (s_health_available)
    {
        health_service_events_unsubscribe();
    }

    compass_service_unsubscribe();
    tick_timer_service_unsubscribe();

    text_layer_destroy(s_text_layer_date);
    text_layer_destroy(s_text_layer_battery);
    text_layer_destroy(s_text_layer_heart);
    text_layer_destroy(s_text_layer_steps);
    text_layer_destroy(s_text_layer_compass);

    layer_destroy(s_logo_layer);
}

// ---------- Public API ----------
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
    return window_stack_get_top_window() == s_logo_window;
}

void logo_deinit(void)
{
    window_destroy(s_logo_window);
}
