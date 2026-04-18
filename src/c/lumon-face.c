#include <pebble.h>
#include "grid_motion.h"
#include "minute_transition.h"
#include "logo_screen.h"
#include "numbers_screen.h"
#include "constants.h"

// Animation timing
#define ANIMATION_TICK 150

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context);
static void prv_click_config_provider(void *context);

static Window *s_window;

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (!logo_is_showing()) {
        logo_show();
    } else {
        logo_hide();
    }
}

static void prv_click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
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

  // Set up button handling
  window_set_click_config_provider(s_window, prv_click_config_provider);
}

static void prv_window_unload(Window *window)
{
  numbers_screen_deinit();
}

static void prv_init(void)
{
  srand((unsigned int)time(NULL));
  s_window = window_create();
  logo_init();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = prv_window_load,
                                           .unload = prv_window_unload,
                                       });

  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void)
{
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
