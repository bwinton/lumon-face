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
      int16_t x = s_time_move_orig[j].origin.x + ((s_time_move_dest[j].origin.x - s_time_move_orig[j].origin.x) * s_special_frame) / total;
      int16_t y = s_time_move_orig[j].origin.y + ((s_time_move_dest[j].origin.y - s_time_move_orig[j].origin.y) * s_special_frame) / total;
      int16_t w = s_time_move_orig[j].size.w + ((s_time_move_dest[j].size.w - s_time_move_orig[j].size.w) * s_special_frame) / total;
      int16_t h = s_time_move_orig[j].size.h + ((s_time_move_dest[j].size.h - s_time_move_orig[j].size.h) * s_special_frame) / total;
      GRect frame = GRect(x, y, w, h);
      layer_set_frame(text_layer_get_layer(grid_layers[index]), frame);
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
