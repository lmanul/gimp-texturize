// Performance test harness for the Texturize plugin. Built and run by
// scripts/perftest, which links it against the plugin's own object files so
// that the code being timed is exactly what "compile" produced.
//
// Usage: perftest --list
//        perftest PART PATCH_SIZE IMAGE_SIZE OVERLAP
//
// To add a part: write a function with the same signature as part_big_loop
// below and add it to the "parts" table at the bottom of this file. A part
// times only the code it is interested in and returns the elapsed seconds.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gegl.h>
#include <glib.h>

#include "texturize.h"

#define CHANNELS 3

typedef struct {
  int patch_size;  // Width and height of the source patch.
  int image_size;  // Width and height of the generated image.
  int overlap;
} Config;

typedef struct {
  const char* name;
  // Returns the elapsed time in seconds, and a checksum of whatever the part
  // produced so that two builds can be checked for identical output.
  double (*run)(const Config* config, guint64* checksum);
} Part;

////////////////////////////////////////////////////////////////////////////////
// Helpers.

// libgimp's version needs a running GIMP, so we replace it.
gboolean gimp_progress_update(gdouble percentage) {
  return TRUE;
}

static double now(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec + t.tv_nsec / 1e9;
}

// FNV-1a.
static guint64 checksum_of(const guchar* data, int length) {
  guint64 hash = 1469598103934665603ull;
  for (int k = 0; k < length; k++) {
    hash ^= data[k];
    hash *= 1099511628211ull;
  }
  return hash;
}

// A deterministic texture: blurred pseudo-random noise.
static guchar* make_patch(int size) {
  int length = size * size * CHANNELS;
  guchar* noise = g_new(guchar, length);
  guchar* patch = g_new(guchar, length);
  unsigned seed = 12345;

  for (int k = 0; k < length; k++) {
    seed = seed * 1103515245u + 12345u;
    noise[k] = seed >> 24;
  }
  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      for (int c = 0; c < CHANNELS; c++) {
        int sum = 0;
        for (int dy = -2; dy <= 2; dy++) {
          for (int dx = -2; dx <= 2; dx++) {
            int ny = (y + dy + size) % size;
            int nx = (x + dx + size) % size;
            sum += noise[(ny * size + nx) * CHANNELS + c];
          }
        }
        patch[(y * size + x) * CHANNELS + c] = sum / 25;
      }
    }
  }
  g_free(noise);
  return patch;
}

// Whether the plugin stores "filled" as one flat row-major buffer
// (filled[y * width + x]) or as a table of columns (filled[x][y]). Looking at
// the plugin's own prototype lets this harness time either version.
#define FILLED_IS_FLAT _Generic(&count_filled_pixels, \
    int (*)(guchar*, int, int): 1, \
    default: 0)

// Allocates "filled" with the top left patch_size x patch_size square marked
// as filled, as render() does. Never freed: the process exits after one part.
static void* make_filled(int image_size, int patch_size) {
  if (FILLED_IS_FLAT) {
    guchar* filled = g_new0(guchar, image_size * image_size);
    for (int y = 0; y < patch_size; y++) {
      memset(filled + y * image_size, 1, patch_size);
    }
    return filled;
  } else {
    guchar** filled = g_new(guchar*, image_size);
    for (int x = 0; x < image_size; x++) {
      filled[x] = g_new0(guchar, image_size);
      if (x < patch_size) memset(filled[x], 1, patch_size);
    }
    return filled;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Part: the big loop (render.c), i.e. everything but reading the source
// drawable and writing the new image.

// Not declared in any header. "filled" is declared as void* here so that this
// file compiles whichever way the plugin stores it.
void the_big_loop(
    guchar* image, guchar* patch,
    int width_i, int height_i, int width_p, int height_p,
    GeglRectangle rect_image, GeglRectangle rect_patch,
    void* filled,
    gboolean tileable,
    guchar* coupe_h_here, guchar* coupe_h_west,
    guchar* coupe_v_here, guchar* coupe_v_north,
    int x_off_min, int y_off_min,
    int x_off_max, int y_off_max,
    int channels);

static double run_big_loop(const Config* config, gboolean tileable,
                           guint64* checksum) {
  int size_p = config->patch_size;
  int size_i = config->image_size;
  int image_length = size_i * size_i * CHANNELS;

  guchar* patch = make_patch(size_p);
  guchar* image = g_new0(guchar, image_length);
  void* filled = make_filled(size_i, size_p);
  // The cuts aren't used by the plugin yet, but it still expects the buffers.
  guchar* coupe_h_here  = g_new0(guchar, image_length);
  guchar* coupe_h_west  = g_new0(guchar, image_length);
  guchar* coupe_v_here  = g_new0(guchar, image_length);
  guchar* coupe_v_north = g_new0(guchar, image_length);

  // Paste a first patch at position (0,0), as render() does.
  for (int row = 0; row < size_p; row++) {
    memcpy(image + row * size_i * CHANNELS, patch + row * size_p * CHANNELS,
           size_p * CHANNELS);
  }

  // Same heuristics as render().
  int off_min = MIN(config->overlap, size_p - 1);
  int off_max = CLAMP(20, off_min / 3, size_p - 1);
  GeglRectangle rect_image = { 0, 0, size_i, size_i };
  GeglRectangle rect_patch = { 0, 0, size_p, size_p };

  double start = now();
  the_big_loop(image, patch, size_i, size_i, size_p, size_p,
               rect_image, rect_patch, filled, tileable,
               coupe_h_here, coupe_h_west, coupe_v_here, coupe_v_north,
               off_min, off_min, off_max, off_max, CHANNELS);
  double elapsed = now() - start;

  *checksum = checksum_of(image, image_length);
  return elapsed;
}

static double part_big_loop(const Config* config, guint64* checksum) {
  return run_big_loop(config, FALSE, checksum);
}

static double part_big_loop_tileable(const Config* config, guint64* checksum) {
  return run_big_loop(config, TRUE, checksum);
}

////////////////////////////////////////////////////////////////////////////////
// The list of parts.

static const Part parts[] = {
  { "big_loop",          part_big_loop },
  { "big_loop_tileable", part_big_loop_tileable },
};

int main(int argc, char** argv) {
  if (argc == 2 && strcmp(argv[1], "--list") == 0) {
    for (guint k = 0; k < G_N_ELEMENTS(parts); k++) {
      printf("%s\n", parts[k].name);
    }
    return 0;
  }
  if (argc != 5) {
    fprintf(stderr, "Usage: %s --list\n"
                    "       %s PART PATCH_SIZE IMAGE_SIZE OVERLAP\n",
            argv[0], argv[0]);
    return 2;
  }

  Config config = { atoi(argv[2]), atoi(argv[3]), atoi(argv[4]) };
  if (config.patch_size < 2 || config.image_size <= config.patch_size) {
    fprintf(stderr, "The image must be bigger than the patch.\n");
    return 2;
  }

  for (guint k = 0; k < G_N_ELEMENTS(parts); k++) {
    if (strcmp(argv[1], parts[k].name) == 0) {
      guint64 checksum = 0;
      double elapsed = parts[k].run(&config, &checksum);
      printf("%.3f %016" G_GINT64_MODIFIER "x\n", elapsed, checksum);
      return 0;
    }
  }
  fprintf(stderr, "Unknown part: %s\n", argv[1]);
  return 2;
}
