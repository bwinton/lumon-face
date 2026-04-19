#include "logo_animation.h"
#include "constants.h"

#define STEP 2621
#define TIMER_MS 20

#define NUM_ELLIPSES 3
#define NUM_POINTS 40 // Number of points for the polyline

static const struct
{
  int w;
  int h;
} s_ellipses[NUM_ELLIPSES] = {
    {180, 84},
    {142, 84},
    {70, 84}};

static GPoint s_center = {100, 84};

static AppTimer *s_logo_timer;
static int32_t s_t = 0;
static int8_t s_stage = 0; // 0-2 ellipses, 3 line1, 4 line2

static GPoint s_prev;
static GPoint s_ellipse_points[NUM_ELLIPSES][NUM_POINTS]; // Store points for each ellipse
static GPath *s_ellipse_paths[NUM_ELLIPSES];              // Store GPaths for each ellipse

// Line animation state
static GPoint s_line_start;
static GPoint s_line_end;
static GPoint s_line2_start;
static GPoint s_line2_end;

static int32_t s_line_progress = 0; // Line animation progress (0 to TRIG_MAX_ANGLE)

// ---------- 3px stroke helper ----------
static void draw_thick_line(GContext *ctx, GPoint a, GPoint b)
{
  graphics_context_set_stroke_color(ctx, LIGHT_BLUE);
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_antialiased(ctx, true);

  // central line
  graphics_draw_line(ctx, a, b);

  // normalize perpendicular (approx)
  APP_LOG(APP_LOG_LEVEL_DEBUG, "    Drawing line from (%d,%d) to (%d,%d)", a.x, a.y, b.x, b.y);

  graphics_draw_line(ctx,
                     GPoint(a.x, a.y),
                     GPoint(b.x, b.y));
}

// ---------- parametric ellipse ----------
static GPoint ellipse_point(int w, int h, int32_t t)
{
  float a = w / 2.0f;
  float b = h / 2.0f;

  float x = s_center.x + a * sin_lookup(t) / TRIG_MAX_RATIO;
  float y = s_center.y - b * cos_lookup(t) / TRIG_MAX_RATIO;

  return GPoint((int)x, (int)y);
}

// ---------- line interpolation ----------
static GPoint line_point(GPoint start, GPoint end, int32_t t)
{
  return GPoint(
      start.x + (end.x - start.x) * t / TRIG_MAX_ANGLE,
      start.y + (end.y - start.y) * t / TRIG_MAX_ANGLE);
}

// ---------- geometry helpers ----------
static GPoint ellipse_left_edge(int w)
{
  return GPoint(s_center.x - w / 2, s_center.y);
}

static GPoint ellipse_right_edge(int w)
{
  return GPoint(s_center.x + w / 2, s_center.y);
}

// Function to generate points for an ellipse
static void generate_ellipse_points(int w, int h, GPoint points[NUM_POINTS])
{
  for (int i = 0; i < NUM_POINTS; i++)
  {
    int32_t t = TRIG_MAX_ANGLE * i / NUM_POINTS;
    points[i] = ellipse_point(w, h, t);
  }
}

// Function to create a GPath from the ellipse points
static GPath *create_ellipse_path(GPoint points[NUM_POINTS])
{
  GPathInfo path_info = {
      .num_points = NUM_POINTS,
      .points = points};
  return gpath_create(&path_info); // Create the GPath from the points
}

// Function to draw the GPath for an ellipse
static void draw_ellipse_path(GContext *ctx, GPath *path)
{
  graphics_context_set_stroke_color(ctx, LIGHT_BLUE);
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_antialiased(ctx, true);

  gpath_draw_outline(ctx, path); // Draw the outline of the path
}

// ---------- Dirty Layer Function ----------
static void set_dirty(void *data)
{
  Layer *layer = (Layer *)data;
  layer_mark_dirty(layer); // Marks the layer as dirty to trigger a redraw
}

// ---------- Line Animation ----------
static void draw_line(GContext *ctx, GPoint start, GPoint end, int32_t t)
{
  GPoint current = line_point(start, end, t);
  draw_thick_line(ctx, start, current);
}

// ---------- animation step ----------
static void layer_update_proc(Layer *layer, GContext *ctx)
{
  int32_t next_t = s_t + STEP;

  if (s_logo_timer)
  {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "  stage: %d, angle: %d", s_stage, next_t);
  }

  if (s_stage < NUM_ELLIPSES)
  {
    // Generate the points for the current ellipse if not already done
    if (s_t == 0)
    {
      generate_ellipse_points(s_ellipses[s_stage].w, s_ellipses[s_stage].h, s_ellipse_points[s_stage]);
    }

    // Calculate the current point index based on the step
    int point_index = (next_t % TRIG_MAX_ANGLE) * NUM_POINTS / TRIG_MAX_ANGLE;

    // Draw the growing outline of the ellipse
    for (int i = 0; i <= point_index; i++) // Draw all points up to point_index
    {
      if (i > 0)
      {
        draw_thick_line(ctx, s_ellipse_points[s_stage][i - 1], s_ellipse_points[s_stage][i]);
      }
    }

    // If the entire ellipse has been drawn, move to the next stage
    if (point_index >= NUM_POINTS - 1)
    {
      // Create and store the GPath for this ellipse
      s_ellipse_paths[s_stage] = create_ellipse_path(s_ellipse_points[s_stage]);
      // First line: From 25% angle to 75% angle of the first ellipse
      s_line_start = s_ellipse_paths[0]->points[(int)(0.15 * NUM_POINTS)];
      s_line_end = s_ellipse_paths[0]->points[(int)(0.85 * NUM_POINTS)];
      s_line_end.x += 2;

      // Second line: From 32.5% angle to 62.5% angle of the first ellipse
      s_line2_start = s_ellipse_paths[0]->points[(int)(0.325 * NUM_POINTS)];
      s_line2_end = s_ellipse_paths[0]->points[(int)(0.675 * NUM_POINTS)];
      s_line2_end.x += 1;

      s_stage++; // Move to the next stage (ellipse)
      s_t = 0;
    }
    else
    {
      s_t = next_t;
    }
  }

  // Draw the GPaths for previously drawn ellipses
  for (int i = 0; i < 3; i++)
  {
    if (s_ellipse_paths[i])
    {
      draw_ellipse_path(ctx, s_ellipse_paths[i]);
    }
  }

  // Line drawing logic based on s_stage
  if (s_stage == NUM_ELLIPSES)
  {
    // Draw the growing first line (animate the line)
    draw_line(ctx, s_line_start, s_line_end, s_line_progress);

    // Increment the first line's animation progress
    if (s_line_progress < TRIG_MAX_ANGLE)
    {
      s_line_progress += STEP;
    }

    // If the first line is done, move to the next stage for the second line
    if (s_line_progress >= TRIG_MAX_ANGLE)
    {
      s_stage++;           // Move to the next stage (second line)
      s_line_progress = 0; // Reset second line progress
    }
  }

  // Second line: If we are in the stage for the second line
  if (s_stage == NUM_ELLIPSES + 1)
  {
    // Draw the growing first line (animate the line)
    draw_line(ctx, s_line_start, s_line_end, TRIG_MAX_ANGLE);

    // Draw the growing second line (animate the second line)
    draw_line(ctx, s_line2_start, s_line2_end, s_line_progress);

    // Increment the second line's animation progress
    if (s_line_progress < TRIG_MAX_ANGLE)
    {
      s_line_progress += STEP;
    }

    // Once the second line is done, move to the next stage (optional)
    if (s_line_progress >= TRIG_MAX_ANGLE)
    {
      s_stage++; // Move to the next stage (or stop if you're done)
    }
  }

  // Keep triggering a redraw if still animating
  if (s_stage < NUM_ELLIPSES + 2) // Added +2 since we are handling two lines
  {
    s_logo_timer = app_timer_register(TIMER_MS, set_dirty, layer);
  }
  else
  {
    s_logo_timer = NULL;
  }
}

// ---------- start ----------
void start_logo_animation(Layer *layer)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Logo animation starting");
  s_stage = 0;
  s_t = 0;
  s_line_progress = 0; // Reset line animation state
  for (int i = 0; i < 3; i++)
  {
    s_ellipse_paths[i] = NULL;
  }

  s_prev = ellipse_point(s_ellipses[0].w, s_ellipses[0].h, 0);

  if (s_logo_timer)
  {
    app_timer_cancel(s_logo_timer);
    s_logo_timer = NULL;
  }
  layer_set_update_proc(layer, layer_update_proc);
  s_logo_timer = app_timer_register(TIMER_MS, set_dirty, layer);
}

void stop_logo_animation()
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Logo animation stopping");
  s_stage = 0;
  s_t = 0;
  s_line_progress = 0;

  if (s_logo_timer)
  {
    app_timer_cancel(s_logo_timer);
    s_logo_timer = NULL;
  }
}