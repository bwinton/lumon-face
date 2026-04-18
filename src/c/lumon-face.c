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

static Layer *s_special_box_bg_layers[2];
static TextLayer *s_special_box_layers[2];
static char s_special_box_text[2][8];
static GRect s_special_box_start[2];
static GRect s_special_box_dest[2];
static bool s_special_active = false;
static int s_special_state = 0;
static int s_special_frame = 0;
static int s_special_choice = 0;
static int s_special_last_minute = -1;
static int s_cell_width = 0;
static int s_cell_height = 0;
static GRect s_time_move_orig[4];
static GRect s_time_move_dest[4];
static char s_time_old_digits[4][2];
static char s_time_new_digits[4][2];
static bool s_time_hidden = false;
static const int s_special_frames_up = 6;
static const int s_special_frames_time_move = 10;
static const int s_special_frames_fade = 6;
static const int s_special_frames_down = 6;

// Dark blue background, Light blue text
#define DARK_BLUE GColorCobaltBlue
#define LIGHT_BLUE GColorCeleste

static const GColor s_fade_colors[] = {DARK_BLUE,
                                       GColorBlueMoon,
                                       GColorPictonBlue,
                                       GColorVividCerulean,
                                       GColorTiffanyBlue,
                                       GColorCadetBlue,
                                       GColorCyan,
                                       GColorElectricBlue,
                                       LIGHT_BLUE};

#define ANIMATION_TICK 150

static void prv_special_box_draw(Layer *layer, GContext *ctx)
{
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, DARK_BLUE);
  graphics_context_set_stroke_color(ctx, LIGHT_BLUE);
  graphics_fill_rect(ctx, bounds, 4, GCornersAll);
  graphics_draw_rect(ctx, bounds);
}

static void prv_end_special_animation(void)
{
  for (int i = 0; i < 2; i++)
  {
    if (s_special_box_bg_layers[i])
    {
      layer_set_hidden(s_special_box_bg_layers[i], true);
    }
    if (s_special_box_layers[i])
    {
      layer_set_hidden(text_layer_get_layer(s_special_box_layers[i]), true);
      text_layer_set_text(s_special_box_layers[i], "");
    }
  }
  for (int j = 0; j < 4; j++)
  {
    int index = s_time_positions[j];
    layer_set_frame(text_layer_get_layer(s_grid_layers[index]), s_grid_orig[index]);
    layer_set_hidden(text_layer_get_layer(s_grid_layers[index]), false);
    text_layer_set_text_color(s_grid_layers[index], LIGHT_BLUE);
    s_move_offset_x[index] = 0;
    s_move_offset_y[index] = 0;
  }
  s_time_hidden = false;
  s_special_active = false;
  s_special_state = 0;
  s_special_frame = 0;
}

static void prv_start_special_animation(const struct tm *tick_time)
{
  int label_base = (rand() % 5) + 1;
  for (int i = 0; i < 2; i++)
  {
    snprintf(s_special_box_text[i], sizeof(s_special_box_text[i]), "%02d", label_base + i);
    text_layer_set_text(s_special_box_layers[i], s_special_box_text[i]);
    layer_set_hidden(s_special_box_bg_layers[i], false);
    layer_set_hidden(text_layer_get_layer(s_special_box_layers[i]), false);
    layer_set_frame(s_special_box_bg_layers[i], s_special_box_start[i]);
    layer_set_frame(text_layer_get_layer(s_special_box_layers[i]), s_special_box_start[i]);
  }
  s_special_choice = rand() % 2;
  int box_x = s_special_box_dest[s_special_choice].origin.x;
  int box_y = s_special_box_dest[s_special_choice].origin.y;
  int box_w = s_special_box_dest[s_special_choice].size.w;
  int digit_w = box_w / 4;
  int hour = tick_time->tm_hour;
  int minute = tick_time->tm_min;
  for (int j = 0; j < 4; j++)
  {
    int index = s_time_positions[j];
    s_time_old_digits[j][0] = s_grid_buffer[index][0];
    s_time_old_digits[j][1] = '\0';
    int digit_value;
    if (j == 0)
      digit_value = hour / 10;
    else if (j == 1)
      digit_value = hour % 10;
    else if (j == 2)
      digit_value = minute / 10;
    else
      digit_value = minute % 10;
    s_time_new_digits[j][0] = '0' + digit_value;
    s_time_new_digits[j][1] = '\0';
    s_time_move_orig[j] = layer_get_frame(text_layer_get_layer(s_grid_layers[index]));
    s_time_move_dest[j] = GRect(box_x + (j * digit_w), box_y, digit_w, s_special_box_dest[s_special_choice].size.h);
    text_layer_set_text(s_grid_layers[index], s_time_old_digits[j]);
    layer_set_hidden(text_layer_get_layer(s_grid_layers[index]), false);
  }
  s_time_hidden = false;
  s_special_active = true;
  s_special_state = 1;
  s_special_frame = 0;
}

static void prv_update_special_animation(void)
{
  if (!s_special_active)
    return;

  if (s_special_state == 1)
  {
    s_special_frame++;
    int total = s_special_frames_up;
    for (int i = 0; i < 2; i++)
    {
      int16_t y = s_special_box_start[i].origin.y + ((s_special_box_dest[i].origin.y - s_special_box_start[i].origin.y) * s_special_frame) / total;
      GRect frame = GRect(s_special_box_start[i].origin.x, y, s_special_box_start[i].size.w, s_special_box_start[i].size.h);
      layer_set_frame(s_special_box_bg_layers[i], frame);
      layer_set_frame(text_layer_get_layer(s_special_box_layers[i]), frame);
    }
    if (s_special_frame >= total)
    {
      s_special_state = 2;
      s_special_frame = 0;
      for (int j = 0; j < 4; j++)
      {
        int index = s_time_positions[j];
        layer_set_hidden(text_layer_get_layer(s_grid_layers[index]), false);
        text_layer_set_text_color(s_grid_layers[index], LIGHT_BLUE);
      }
    }
  }
  else if (s_special_state == 2)
  {
    s_special_frame++;
    int total = s_special_frames_time_move;
    for (int j = 0; j < 4; j++)
    {
      int index = s_time_positions[j];
      int16_t x = s_time_move_orig[j].origin.x + ((s_time_move_dest[j].origin.x - s_time_move_orig[j].origin.x) * s_special_frame) / total;
      int16_t y = s_time_move_orig[j].origin.y + ((s_time_move_dest[j].origin.y - s_time_move_orig[j].origin.y) * s_special_frame) / total;
      int16_t w = s_time_move_orig[j].size.w + ((s_time_move_dest[j].size.w - s_time_move_orig[j].size.w) * s_special_frame) / total;
      int16_t h = s_time_move_orig[j].size.h + ((s_time_move_dest[j].size.h - s_time_move_orig[j].size.h) * s_special_frame) / total;
      GRect frame = GRect(x, y, w, h);
      layer_set_frame(text_layer_get_layer(s_grid_layers[index]), frame);
    }
    if (s_special_frame >= total)
    {
      for (int j = 0; j < 4; j++)
      {
        int index = s_time_positions[j];
        layer_set_hidden(text_layer_get_layer(s_grid_layers[index]), true);
      }
      s_time_hidden = true;
      s_special_state = 3;
      s_special_frame = 0;
    }
  }
  else if (s_special_state == 3)
  {
    s_special_frame++;
    int total = s_special_frames_down;
    for (int i = 0; i < 2; i++)
    {
      int16_t y = s_special_box_dest[i].origin.y + ((s_special_box_start[i].origin.y - s_special_box_dest[i].origin.y) * s_special_frame) / total;
      GRect frame = GRect(s_special_box_dest[i].origin.x, y, s_special_box_dest[i].size.w, s_special_box_dest[i].size.h);
      layer_set_frame(s_special_box_bg_layers[i], frame);
      layer_set_frame(text_layer_get_layer(s_special_box_layers[i]), frame);
    }
    if (s_special_frame >= total)
    {
      s_special_state = 4;
      s_special_frame = 0;
      for (int j = 0; j < 4; j++)
      {
        int index = s_time_positions[j];
        layer_set_frame(text_layer_get_layer(s_grid_layers[index]), s_grid_orig[index]);
        text_layer_set_text(s_grid_layers[index], s_time_new_digits[j]);
        layer_set_hidden(text_layer_get_layer(s_grid_layers[index]), false);
        text_layer_set_text_color(s_grid_layers[index], s_fade_colors[0]);
      }
    }
  }
  else if (s_special_state == 4)
  {
    s_special_frame++;
    int step = s_special_frame;
    if (step > s_special_frames_fade)
      step = s_special_frames_fade;
    int color_index = (step * (int)(sizeof(s_fade_colors) / sizeof(s_fade_colors[0]) - 1)) / s_special_frames_fade;
    for (int j = 0; j < 4; j++)
    {
      int index = s_time_positions[j];
      text_layer_set_text_color(s_grid_layers[index], s_fade_colors[color_index]);
    }
    if (s_special_frame >= s_special_frames_fade)
    {
      prv_end_special_animation();
    }
  }
}

static void prv_update_time(void)
{
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  int current_minute = tick_time->tm_min;
  if (s_special_last_minute >= 0 && current_minute != s_special_last_minute)
  {
    prv_start_special_animation(tick_time);
  }
  s_special_last_minute = current_minute;

  // Format time as HHMM (no colon)
  strftime(s_time_text, sizeof(s_time_text), "%H%M", tick_time);

  // Update only the time digits; keep the other numbers unchanged
  for (int i = 0; i < 4; i++)
  {
    int pos = s_time_positions[i];
    if (s_special_active && s_special_state != 4)
    {
      s_grid_buffer[pos][0] = s_time_old_digits[i][0];
    }
    else if (s_special_active && s_special_state == 4)
    {
      s_grid_buffer[pos][0] = s_time_new_digits[i][0];
    }
    else
    {
      s_grid_buffer[pos][0] = s_time_text[i];
    }
    s_grid_buffer[pos][1] = '\0';
    text_layer_set_text(s_grid_layers[pos], s_grid_buffer[pos]);
  }

  // Animate every number around its center point without altering text
  for (int i = 0; i < 30; i++)
  {
    bool is_time_digit = false;
    for (int j = 0; j < 4; j++)
    {
      if (i == s_time_positions[j])
      {
        is_time_digit = true;
        break;
      }
    }
    if (s_special_active && s_special_state >= 1 && is_time_digit)
    {
      continue;
    }

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

  prv_update_special_animation();
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
  s_cell_width = cell_width;
  s_cell_height = cell_height;

  // Prepare minute-change overlay boxes offscreen at the bottom.
  // They are created here but added after the grid so they draw on top.
  for (int i = 0; i < 2; i++)
  {
    s_special_box_bg_layers[i] = layer_create(GRect(i * (bounds.size.w / 2), bounds.size.h, bounds.size.w / 2, cell_height));
    layer_set_update_proc(s_special_box_bg_layers[i], prv_special_box_draw);
    layer_set_hidden(s_special_box_bg_layers[i], true);

    s_special_box_layers[i] = text_layer_create(GRect(i * (bounds.size.w / 2), bounds.size.h, bounds.size.w / 2, cell_height));
    text_layer_set_background_color(s_special_box_layers[i], GColorClear);
    text_layer_set_text_color(s_special_box_layers[i], LIGHT_BLUE);
    text_layer_set_text_alignment(s_special_box_layers[i], GTextAlignmentCenter);
    text_layer_set_font(s_special_box_layers[i], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    layer_set_hidden(text_layer_get_layer(s_special_box_layers[i]), true);

    s_special_box_start[i] = GRect(i * (bounds.size.w / 2), bounds.size.h, bounds.size.w / 2, cell_height);
    s_special_box_dest[i] = GRect(i * (bounds.size.w / 2), bounds.size.h - cell_height - 8, bounds.size.w / 2, cell_height);
  }

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

  // Add the special overlay boxes after the grid so they render on top.
  for (int i = 0; i < 2; i++)
  {
    layer_add_child(window_layer, s_special_box_bg_layers[i]);
    layer_add_child(window_layer, text_layer_get_layer(s_special_box_layers[i]));
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
  for (int i = 0; i < 2; i++)
  {
    if (s_special_box_layers[i])
    {
      text_layer_destroy(s_special_box_layers[i]);
      s_special_box_layers[i] = NULL;
    }
    if (s_special_box_bg_layers[i])
    {
      layer_destroy(s_special_box_bg_layers[i]);
      s_special_box_bg_layers[i] = NULL;
    }
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
