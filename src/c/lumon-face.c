#include <pebble.h>

static Window *s_window;
static TextLayer *s_grid_layers[35]; // 7x5 grid = 35 numbers
static char s_time_text[5] = "0000"; // HHMM without colon
static char s_grid_buffer[35][2];    // Buffers for all grid positions
static int s_animation_offset = 0;

// Dark blue background, Light blue text
// The background colour should be 0x07123f. The text colour should be 0x8ab6d6.
#define DARK_BLUE GColorMidnightGreen
#define LIGHT_BLUE GColorElectricBlue

static void prv_update_time(void)
{
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  // Format time as HHMM (no colon)
  strftime(s_time_text, sizeof(s_time_text), "%H%M", tick_time);

  // Initialize all grid positions with animated numbers
  for (int i = 0; i < 35; i++)
  {
    int digit_val = (i + s_animation_offset) % 10;
    snprintf(s_grid_buffer[i], sizeof(s_grid_buffer[i]), "%d", digit_val);
  }

  // Place time digits in the middle positions (row 2, columns 2-5)
  // With 7 columns (0-6), middle 4 are: columns 2, 3, 4, 5
  // Row 2 positions: 2*7 + 2 = 16, 2*7 + 3 = 17, 2*7 + 4 = 18, 2*7 + 5 = 19
  int time_positions[4] = {16, 17, 18, 19};
  for (int i = 0; i < 4; i++)
  {
    s_grid_buffer[time_positions[i]][0] = s_time_text[i];
    s_grid_buffer[time_positions[i]][1] = '\0';
  }

  // Update all grid layers
  for (int i = 0; i < 35; i++)
  {
    text_layer_set_text(s_grid_layers[i], s_grid_buffer[i]);
  }

  s_animation_offset = (s_animation_offset + 1) % 10;
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  prv_update_time();
}

static void prv_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Set window background to dark blue
  window_set_background_color(s_window, DARK_BLUE);

  // Create a 7x5 grid of numbers, spaced to fill the entire screen
  int cols = 7;
  int rows = 5;
  int cell_width = bounds.size.w / cols;
  int cell_height = bounds.size.h / rows;
  int start_x = 0;
  int start_y = 0;

  // Time positions for reference
  int time_positions[4] = {16, 17, 18, 19};

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
        if (index == time_positions[i])
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

      layer_add_child(window_layer, text_layer_get_layer(s_grid_layers[index]));
      index++;
    }
  }

  // Update time immediately
  prv_update_time();
}

static void prv_window_unload(Window *window)
{
  for (int i = 0; i < 35; i++)
  {
    text_layer_destroy(s_grid_layers[i]);
  }
}

static void prv_init(void)
{
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = prv_window_load,
                                           .unload = prv_window_unload,
                                       });

  // Subscribe to second ticks for animation movement
  tick_timer_service_subscribe(SECOND_UNIT, prv_tick_handler);

  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void)
{
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void)
{
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Lumon watchface initialized");

  app_event_loop();
  prv_deinit();
}
