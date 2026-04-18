#include "grid_motion.h"
#include "minute_transition.h"
#include <pebble.h>

#define DARK_BLUE GColorCobaltBlue
#define LIGHT_BLUE GColorCeleste

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
static GRect s_time_move_orig[4];
static GRect s_time_move_dest[4];
static char s_time_old_digits[4][2];
static char s_time_new_digits[4][2];
static const int s_special_frames_up = 6;
static const int s_special_frames_time_move = 10;
static const int s_special_frames_fade = 6;
static const int s_special_frames_down = 6;

static const GColor s_fade_colors[] = {DARK_BLUE,
                                       GColorBlueMoon,
                                       GColorPictonBlue,
                                       GColorVividCerulean,
                                       GColorTiffanyBlue,
                                       GColorCadetBlue,
                                       GColorCyan,
                                       GColorElectricBlue,
                                       LIGHT_BLUE};

static void prv_special_box_draw(Layer *layer, GContext *ctx)
{
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, DARK_BLUE);
  graphics_context_set_stroke_color(ctx, LIGHT_BLUE);
  graphics_fill_rect(ctx, bounds, 4, GCornersAll);
  graphics_draw_rect(ctx, bounds);
}

static void prv_end_special_animation(TextLayer *grid_layers[])
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
    layer_set_frame(text_layer_get_layer(grid_layers[index]), s_time_move_orig[j]);
    layer_set_hidden(text_layer_get_layer(grid_layers[index]), false);
    text_layer_set_text_color(grid_layers[index], LIGHT_BLUE);
  }
  grid_motion_reset_offsets_at_indices(s_time_positions, 4);
  s_special_active = false;
  s_special_state = 0;
  s_special_frame = 0;
}

static void prv_start_special_animation(const struct tm *tick_time, TextLayer *grid_layers[], char grid_buffer[][2])
{
  int day = tick_time->tm_mday;
  int month = tick_time->tm_mon + 1;
  int values[2] = {day, month};
  
  for (int i = 0; i < 2; i++)
  {
    snprintf(s_special_box_text[i], sizeof(s_special_box_text[i]), "%02d", values[i]);
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
    s_time_old_digits[j][0] = grid_buffer[index][0];
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
    s_time_move_orig[j] = layer_get_frame(text_layer_get_layer(grid_layers[index]));
    s_time_move_dest[j] = GRect(box_x + (j * digit_w), box_y, digit_w, s_special_box_dest[s_special_choice].size.h);
    text_layer_set_text(grid_layers[index], s_time_old_digits[j]);
    layer_set_hidden(text_layer_get_layer(grid_layers[index]), false);
  }
  s_special_active = true;
  s_special_state = 1;
  s_special_frame = 0;
}

void minute_transition_init(GRect bounds, int cell_height)
{
  const int side_padding = 4;
  const int bottom_padding = 4;
  const int box_width = (bounds.size.w / 2) - (side_padding * 2);
  const int box_height = cell_height - bottom_padding;

  for (int i = 0; i < 2; i++)
  {
    int box_x = (i * (bounds.size.w / 2)) + side_padding;
    s_special_box_bg_layers[i] = layer_create(GRect(box_x, bounds.size.h, box_width, box_height));
    layer_set_update_proc(s_special_box_bg_layers[i], prv_special_box_draw);
    layer_set_hidden(s_special_box_bg_layers[i], true);

    s_special_box_layers[i] = text_layer_create(GRect(box_x, bounds.size.h, box_width, box_height));
    text_layer_set_background_color(s_special_box_layers[i], GColorClear);
    text_layer_set_text_color(s_special_box_layers[i], LIGHT_BLUE);
    text_layer_set_text_alignment(s_special_box_layers[i], GTextAlignmentCenter);
    text_layer_set_font(s_special_box_layers[i], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    layer_set_hidden(text_layer_get_layer(s_special_box_layers[i]), true);

    s_special_box_start[i] = GRect(box_x, bounds.size.h, box_width, box_height);
    s_special_box_dest[i] = GRect(box_x, bounds.size.h - box_height - bottom_padding, box_width, box_height);
  }
}

void minute_transition_add_children(Layer *window_layer)
{
  for (int i = 0; i < 2; i++)
  {
    layer_add_child(window_layer, s_special_box_bg_layers[i]);
    layer_add_child(window_layer, text_layer_get_layer(s_special_box_layers[i]));
  }
}

void minute_transition_deinit(void)
{
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

void minute_transition_maybe_start(const struct tm *tick_time, TextLayer *grid_layers[], char grid_buffer[][2])
{
  int current_minute = tick_time->tm_min;
  if (s_special_last_minute >= 0 && current_minute != s_special_last_minute)
  {
    prv_start_special_animation(tick_time, grid_layers, grid_buffer);
  }
  s_special_last_minute = current_minute;
}

void minute_transition_update_animation(TextLayer *grid_layers[], const GRect grid_orig[30])
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
        layer_set_hidden(text_layer_get_layer(grid_layers[index]), false);
        text_layer_set_text_color(grid_layers[index], LIGHT_BLUE);
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
      int start_delay = 0; // all digits move together
      int frame = s_special_frame - start_delay;
      if (frame < 0)
      {
        layer_set_frame(text_layer_get_layer(grid_layers[index]), s_time_move_orig[j]);
        continue;
      }

      int travel_total = total - start_delay;
      if (travel_total < 1)
        travel_total = 1;
      if (frame > travel_total)
        frame = travel_total;

      int x_progress_num = frame * 14;
      int x_progress_den = travel_total * 10;
      if (x_progress_num > x_progress_den)
        x_progress_num = x_progress_den;

      int16_t x = s_time_move_orig[j].origin.x + ((s_time_move_dest[j].origin.x - s_time_move_orig[j].origin.x) * x_progress_num) / x_progress_den;
      int16_t y = s_time_move_orig[j].origin.y + ((s_time_move_dest[j].origin.y - s_time_move_orig[j].origin.y) * frame) / travel_total;
      int16_t w = s_time_move_orig[j].size.w + ((s_time_move_dest[j].size.w - s_time_move_orig[j].size.w) * x_progress_num) / x_progress_den;
      int16_t h = s_time_move_orig[j].size.h + ((s_time_move_dest[j].size.h - s_time_move_orig[j].size.h) * frame) / travel_total;

      GRect frame_rect = GRect(x, y, w, h);
      layer_set_frame(text_layer_get_layer(grid_layers[index]), frame_rect);
    }
    if (s_special_frame >= total)
    {
      for (int j = 0; j < 4; j++)
      {
        int index = s_time_positions[j];
        layer_set_hidden(text_layer_get_layer(grid_layers[index]), true);
      }
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
        layer_set_frame(text_layer_get_layer(grid_layers[index]), grid_orig[index]);
        text_layer_set_text(grid_layers[index], s_time_new_digits[j]);
        layer_set_hidden(text_layer_get_layer(grid_layers[index]), false);
        text_layer_set_text_color(grid_layers[index], s_fade_colors[0]);
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
      text_layer_set_text_color(grid_layers[index], s_fade_colors[color_index]);
    }
    if (s_special_frame >= s_special_frames_fade)
    {
      prv_end_special_animation(grid_layers);
    }
  }
}

bool minute_transition_active(void)
{
  return s_special_active;
}

bool minute_transition_should_display_old_digits(void)
{
  return s_special_active && s_special_state != 4;
}

bool minute_transition_should_display_new_digits(void)
{
  return s_special_active && s_special_state == 4;
}
