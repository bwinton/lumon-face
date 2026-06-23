#include <pebble.h>
#include "grid_motion.h"
#include "minute_transition.h"
#include "logo_screen.h"
#include "numbers_screen.h"
#include "settings.h"
#include "constants.h"

// Animation timing
#define ANIMATION_TICK 150
#define DOUBLE_TAP_TIMEOUT_MS 1000

static void prv_accel_tap_handler(AccelAxisType axis, int32_t direction);
static void prv_double_tap_timeout(void *context);

static Window *s_window;
static AppTimer *s_double_tap_timer = NULL;
static bool s_double_tap_pending = false;

static void prv_double_tap_timeout(void *context)
{
  s_double_tap_pending = false;
  s_double_tap_timer = NULL;
}

static void prv_accel_tap_handler(AccelAxisType axis, int32_t direction)
{
  APP_LOG(APP_LOG_LEVEL_INFO, "1 - Got tap, double? %d, direction: %d", s_double_tap_pending, direction);

  if (!s_double_tap_pending)
  {
    s_double_tap_pending = true;
    s_double_tap_timer = app_timer_register(DOUBLE_TAP_TIMEOUT_MS, prv_double_tap_timeout, NULL);
    return;
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "2 - Got tap, double? %d, direction: %d", s_double_tap_pending, direction);

  s_double_tap_pending = false;
  if (s_double_tap_timer)
  {
    app_timer_cancel(s_double_tap_timer);
    s_double_tap_timer = NULL;
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Got tap, Showing logo? %d", logo_is_showing());

  if (!logo_is_showing())
  {
    logo_show();
  }
  else
  {
    logo_hide();
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "Got tap, Done");
}

static void prv_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Set window background to dark blue
  window_set_background_color(s_window, DARK_BLUE);

  // Initialize the numbers screen
  numbers_screen_init(bounds);
  numbers_screen_add_children(window_layer);
}

static void prv_window_unload(Window *window)
{
  numbers_screen_deinit();
}

static void prv_init(void)
{
  srand((unsigned int)time(NULL));
  settings_init();
  s_window = window_create();
  logo_init();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = prv_window_load,
                                           .unload = prv_window_unload,
                                       });

  accel_tap_service_subscribe(prv_accel_tap_handler);

  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void)
{
  accel_tap_service_unsubscribe();
  if (s_double_tap_timer)
  {
    app_timer_cancel(s_double_tap_timer);
    s_double_tap_timer = NULL;
  }
  settings_deinit();
  logo_deinit();
  window_destroy(s_window);
}

int main(void)
{
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Lumon watchface initialized");

  app_event_loop();
  prv_deinit();
}
