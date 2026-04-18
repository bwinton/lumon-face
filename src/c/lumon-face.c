#include <pebble.h>
#include "grid_motion.h"
#include "minute_transition.h"
#include "logo_screen.h"
#include "constants.h"

// Grid configuration
#define GRID_START_Y 8

// Animation timing
#define ANIMATION_TICK 150

static void prv_update_time(void);
static void prv_animation_timer_callback(void *context);
static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context);
static void prv_click_config_provider(void *context);

static Window *s_window;
static TextLayer *s_grid_layers[GRID_COUNT]; // 6x5 grid = 30 numbers
static GRect s_grid_orig[GRID_COUNT];
static char s_time_text[5] = "0000"; // HHMM without colon
static char s_grid_buffer[GRID_COUNT][2];    // Buffers for all grid positions
static int s_animation_offset = 0;
static AppTimer *s_animation_timer;
static int s_cell_width = 0;
static int s_cell_height = 0;
static bool s_showing_logo = false;

static void prv_update_time(void)
{
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  minute_transition_maybe_start(tick_time, s_grid_layers, s_grid_buffer);

  // Format time as HHMM (no colon)
  strftime(s_time_text, sizeof(s_time_text), "%H%M", tick_time);

  // Update only the time digits when the transition is not active.
  for (int i = 0; i < 4; i++)
  {
    int pos = TIME_POSITIONS[i];
    if (minute_transition_active())
    {
      continue;
    }
    s_grid_buffer[pos][0] = s_time_text[i];
    s_grid_buffer[pos][1] = '\0';
    text_layer_set_text(s_grid_layers[pos], s_grid_buffer[pos]);
  }

  // Animate every number around its center point without altering text.
  grid_motion_update(minute_transition_active());

  minute_transition_update_animation(s_grid_layers, s_grid_orig);
  s_animation_offset = (s_animation_offset + 1) % 70;
}

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

static void prv_animation_timer_callback(void *context)
{
  prv_update_time();
  s_animation_timer = app_timer_register(ANIMATION_TICK, prv_animation_timer_callback, NULL);
}

static void prv_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Set window background to dark blue
  window_set_background_color(s_window, DARK_BLUE);

  // Create a 6x5 grid of numbers, spaced to fill the screen with top padding
  int cols = GRID_COLS;
  int rows = GRID_ROWS;
  int start_x = 0;
  int start_y = GRID_START_Y;
  int cell_width = bounds.size.w / cols;
  int cell_height = (bounds.size.h - start_y) / rows;
  s_cell_width = cell_width;
  s_cell_height = cell_height;

  minute_transition_init(bounds, cell_height);

  int index = 0;
  for (int row = 0; row < rows; row++)
  {
    for (int col = 0; col < cols; col++)
    {
      int x = start_x + (col * cell_width);
      int y = start_y + (row * cell_height);

      // Check if this is a time position
      bool is_time_position = false;
      for (int i = 0; i < 4; i++)
      {
        if (index == TIME_POSITIONS[i])
        {
          is_time_position = true;
          break;
        }
      }

      // Adjust y position for time layers to align with smaller numbers
      int layer_y = y;
      if (is_time_position)
      {
        layer_y = y - 9; // Offset to vertically center with smaller fonts
      }

      s_grid_layers[index] = text_layer_create(GRect(x, layer_y, cell_width, cell_height));
      s_grid_orig[index] = GRect(x, layer_y, cell_width, cell_height);
      text_layer_set_background_color(s_grid_layers[index], GColorClear);
      text_layer_set_text_color(s_grid_layers[index], LIGHT_BLUE);
      text_layer_set_text_alignment(s_grid_layers[index], GTextAlignmentCenter);

      // Set font based on whether it's a time position
      if (is_time_position)
      {
        text_layer_set_font(s_grid_layers[index], fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
      }
      else
      {
        text_layer_set_font(s_grid_layers[index], fonts_get_system_font(FONT_KEY_GOTHIC_18));
      }
      int digit_val = ((index * 6) + 3) % 10;
      s_grid_buffer[index][0] = '0' + digit_val;
      s_grid_buffer[index][1] = '\0';
      text_layer_set_text(s_grid_layers[index], s_grid_buffer[index]);

      layer_add_child(window_layer, text_layer_get_layer(s_grid_layers[index]));
      index++;
    }
  }

  grid_motion_init(s_grid_layers, s_grid_orig, TIME_POSITIONS, 4);
  minute_transition_add_children(window_layer);

  // Set up button handling
  window_set_click_config_provider(s_window, prv_click_config_provider);

  // Update time immediately
  prv_update_time();
}

static void prv_window_unload(Window *window)
{
  for (int i = 0; i < GRID_COUNT; i++)
  {
    text_layer_destroy(s_grid_layers[i]);
  }
  grid_motion_deinit();
  minute_transition_deinit();
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

  // Start the fast animation timer
  s_animation_timer = app_timer_register(ANIMATION_TICK, prv_animation_timer_callback, NULL);

  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void)
{
  if (s_animation_timer)
  {
    app_timer_cancel(s_animation_timer);
    s_animation_timer = NULL;
  }
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
