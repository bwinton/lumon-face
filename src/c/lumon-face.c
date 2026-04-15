#include <pebble.h>

static Window *s_window;
static TextLayer *s_grid_layers[30]; // 6x5 grid = 30 numbers
static GRect s_grid_orig[30];
static char s_time_text[5] = "0000"; // HHMM without colon
static char s_grid_buffer[30][2];    // Buffers for all grid positions
static int s_animation_offset = 0;
static AppTimer *s_animation_timer;
static int8_t s_move_dx[30];
static int8_t s_move_dy[30];
static int8_t s_move_offset_x[30];
static int8_t s_move_offset_y[30];
static const int s_time_positions[4] = {13, 14, 15, 16};

// Dark blue background, Light blue text
// The background colour should be 0x07123f. The text colour should be 0x8ab6d6.
#define DARK_BLUE GColorCobaltBlue
#define LIGHT_BLUE GColorCeleste
// #define LIGHT_BLUE GColorElectricBlue

#define ANIMATION_TICK 150

static void prv_update_time(void)
{
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  // Format time as HHMM (no colon)
  strftime(s_time_text, sizeof(s_time_text), "%H%M", tick_time);

  // Update only the time digits; keep the other numbers unchanged
  for (int i = 0; i < 4; i++)
  {
    int pos = s_time_positions[i];
    s_grid_buffer[pos][0] = s_time_text[i];
    s_grid_buffer[pos][1] = '\0';
    text_layer_set_text(s_grid_layers[pos], s_grid_buffer[pos]);
  }

  // Animate every number around its center point without altering text
  for (int i = 0; i < 30; i++)
  {
    const int8_t max_offset = 5;
    int8_t next_x = s_move_offset_x[i] + s_move_dx[i];
    int8_t next_y = s_move_offset_y[i] + s_move_dy[i];
    if (next_x > max_offset || next_x < -max_offset || next_y > max_offset || next_y < -max_offset)
    {
      // Choose a new random direction until it keeps the cell inside bounds.
      do
      {
        s_move_dx[i] = (rand() % 3) - 1;
        s_move_dy[i] = (rand() % 3) - 1;
      } while (s_move_dx[i] == 0 && s_move_dy[i] == 0);

      next_x = s_move_offset_x[i] + s_move_dx[i];
      next_y = s_move_offset_y[i] + s_move_dy[i];
      if (next_x > max_offset || next_x < -max_offset || next_y > max_offset || next_y < -max_offset)
      {
        // If the direction is invalid, reverse one axis to stay in bounds.
        if (next_x > max_offset)
          s_move_dx[i] = -1;
        else if (next_x < -max_offset)
          s_move_dx[i] = 1;
        if (next_y > max_offset)
          s_move_dy[i] = -1;
        else if (next_y < -max_offset)
          s_move_dy[i] = 1;

        next_x = s_move_offset_x[i] + s_move_dx[i];
        next_y = s_move_offset_y[i] + s_move_dy[i];
      }
    }

    s_move_offset_x[i] = next_x;
    s_move_offset_y[i] = next_y;
    GRect moved_frame = GRect(s_grid_orig[i].origin.x + s_move_offset_x[i],
                              s_grid_orig[i].origin.y + s_move_offset_y[i],
                              s_grid_orig[i].size.w,
                              s_grid_orig[i].size.h);
    layer_set_frame(text_layer_get_layer(s_grid_layers[i]), moved_frame);
  }

  s_animation_offset = (s_animation_offset + 1) % 70;
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
  int cols = 6;
  int rows = 5;
  int start_x = 0;
  int start_y = 8;
  int cell_width = bounds.size.w / cols;
  int cell_height = (bounds.size.h - start_y) / rows;

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
        if (index == s_time_positions[i])
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
      s_move_offset_x[index] = 0;
      s_move_offset_y[index] = 0;
      do
      {
        s_move_dx[index] = (rand() % 3) - 1;
        s_move_dy[index] = (rand() % 3) - 1;
      } while (s_move_dx[index] == 0 && s_move_dy[index] == 0);
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

  // Update time immediately
  prv_update_time();
}

static void prv_window_unload(Window *window)
{
  for (int i = 0; i < 30; i++)
  {
    text_layer_destroy(s_grid_layers[i]);
  }
}

static void prv_init(void)
{
  srand((unsigned int)time(NULL));
  s_window = window_create();
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
  window_destroy(s_window);
}

int main(void)
{
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Lumon watchface initialized");

  app_event_loop();
  prv_deinit();
}
