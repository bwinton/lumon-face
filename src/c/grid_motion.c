#include "grid_motion.h"
#include "constants.h"
#include <pebble.h>

// Motion limits
#define MAX_OFFSET 5

static TextLayer *s_grid_layers[GRID_COUNT];
static GRect s_grid_orig[GRID_COUNT];
static int8_t s_move_dx[GRID_COUNT];
static int8_t s_move_dy[GRID_COUNT];
static int8_t s_move_offset_x[GRID_COUNT];
static int8_t s_move_offset_y[GRID_COUNT];
static bool s_time_position[GRID_COUNT];

void grid_motion_init(TextLayer *grid_layers[], const GRect grid_orig[30], const int time_positions[4], int time_positions_count)
{
    for (int i = 0; i < GRID_COUNT; i++)
    {
        s_grid_layers[i] = grid_layers[i];
        s_grid_orig[i] = grid_orig[i];
        s_move_offset_x[i] = 0;
        s_move_offset_y[i] = 0;
        do
        {
            s_move_dx[i] = (rand() % 3) - 1;
            s_move_dy[i] = (rand() % 3) - 1;
        } while (s_move_dx[i] == 0 && s_move_dy[i] == 0);
        s_time_position[i] = false;
    }

    for (int i = 0; i < time_positions_count; i++)
    {
        int index = time_positions[i];
        if (index >= 0 && index < GRID_COUNT)
        {
            s_time_position[index] = true;
        }
    }
}

void grid_motion_update(bool skip_time_digits)
{
    const int8_t max_offset = MAX_OFFSET;

    for (int i = 0; i < GRID_COUNT; i++)
    {
        if (skip_time_digits && s_time_position[i])
        {
            continue;
        }

        int8_t next_x = s_move_offset_x[i] + s_move_dx[i];
        int8_t next_y = s_move_offset_y[i] + s_move_dy[i];
        if (next_x > max_offset || next_x < -max_offset || next_y > max_offset || next_y < -max_offset)
        {
            do
            {
                s_move_dx[i] = (rand() % 3) - 1;
                s_move_dy[i] = (rand() % 3) - 1;
            } while (s_move_dx[i] == 0 && s_move_dy[i] == 0);

            next_x = s_move_offset_x[i] + s_move_dx[i];
            next_y = s_move_offset_y[i] + s_move_dy[i];
            if (next_x > max_offset || next_x < -max_offset || next_y > max_offset || next_y < -max_offset)
            {
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
}

void grid_motion_reset_offsets_at_indices(const int indices[], int count)
{
    for (int i = 0; i < count; i++)
    {
        int index = indices[i];
        if (index >= 0 && index < GRID_COUNT)
        {
            s_move_offset_x[index] = 0;
            s_move_offset_y[index] = 0;
        }
    }
}

void grid_motion_deinit(void)
{
    (void)s_grid_layers;
    (void)s_grid_orig;
    (void)s_move_dx;
    (void)s_move_dy;
    (void)s_move_offset_x;
    (void)s_move_offset_y;
    (void)s_time_position;
}
