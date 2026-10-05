#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <glib.h>

#include "main.h"
#include "texturize.h"

#include "plugin-intl.h"

// Computes the distance between image_tab and patch_tab for the zone that's
//   been filled in:
// (x_min,y_min) -> (x_max,y_max) in image_tab
// (x_min,y_min)-posn -> (x_max,y_max) - posn in patch_tab
float difference(gint width_i, gint height_i, gint width_p, gint height_p,
                 guchar * image, guchar * patch,
                 gint posn_x, gint posn_y,
                 gint x_min, gint y_min, gint x_max, gint y_max,
                 gint channels, guchar *filled) {

  gint    somme = 0, zone=0;
  gint    x_i, y_i, k;
  guchar *image_ptr, *patch_ptr;

  gint x_p, y_p;
  gint x_i_start, x_p_start;
  gint xcount, ycount;
  gint iy, ix;
  guchar *image_ptr_x, *patch_ptr_x, *filled_ptr_x;
  gint image_add_y, patch_add_y;

  // source image edges is looping

  ycount = y_max - y_min;
  xcount = x_max - x_min;

  y_i = modulo(y_min, height_i);
  x_i_start = modulo(x_min, width_i);

  y_p = modulo(y_i - posn_y, height_p);
  x_p_start = modulo(x_i_start - posn_x, width_p);

  image_add_y = width_i * channels;
  patch_add_y = width_p * channels;
  image_ptr_x = image + y_i * image_add_y;
  patch_ptr_x = patch + y_p * patch_add_y;
  filled_ptr_x = filled + y_i * width_i;

  for (iy = 0; iy < ycount; iy++) {

    x_i = x_i_start;
    x_p = x_p_start;
    image_ptr = image_ptr_x + x_i * channels;
    patch_ptr = patch_ptr_x + x_p * channels;

    for (ix = 0; ix < xcount; ix++) {
      if (filled_ptr_x[x_i]) {
        for (k = 0 ; k < channels; k++) {
          somme += abs (*image_ptr - *patch_ptr);
          image_ptr++;
          patch_ptr++;
          zone++;
        }
      } else {
        image_ptr += channels;
        patch_ptr += channels;
      }

      if (++x_i >= width_i) { x_i = 0; image_ptr = image_ptr_x; }
      if (++x_p >= width_p) { x_p = 0; patch_ptr = patch_ptr_x; }
    }

    image_ptr_x += image_add_y;
    patch_ptr_x += patch_add_y;
    filled_ptr_x += width_i;

    if (++y_i >= height_i) { y_i = 0; image_ptr_x = image; filled_ptr_x = filled; }
    if (++y_p >= height_p) { y_p = 0; patch_ptr_x = patch; }
  }

  if (zone == 0) {g_message(_("Bug: Zone = 0")); exit(-1);}
  return (((float) somme) / ((float) zone));
}

void offset_optimal(gint    *resultat,
                    guchar  *image, guchar *patch,
                    gint     width_p, gint height_p, gint width_i, gint height_i,
                    gint     x_patch_posn_min, gint y_patch_posn_min, gint x_patch_posn_max, gint y_patch_posn_max,
                    gint     channels, guchar *filled,
                    gboolean tileable) {
  float best_difference = INFINITY;
  gint best_x = 0, best_y = 0;

  // The candidate positions are independent of each other, so they are shared
  // between threads. Each thread keeps its own best position, and they are
  // compared at the end.
  #pragma omp parallel
  {
    float thread_difference = INFINITY, tmp_difference;
    gint thread_x = 0, thread_y = 0;
    gint x_i, y_i;

    #pragma omp for collapse(2) schedule(static) nowait
    for (x_i = x_patch_posn_min; x_i < x_patch_posn_max; x_i++) {
      for (y_i = y_patch_posn_min; y_i < y_patch_posn_max; y_i++) {

        if (tileable) {
          tmp_difference = difference (
            width_i, height_i, width_p, height_p, image, patch,
            x_i, y_i,
            MAX (0, x_i), MAX (0, y_i),
            x_i + width_p, y_i + height_p,
            channels, filled);
        } else {
          tmp_difference = difference (
            width_i, height_i, width_p, height_p, image, patch,
            x_i, y_i,
            MAX (0,x_i), MAX (0,y_i),
            MIN (x_i + width_p, width_i), MIN (y_i + height_p, height_i),
            channels, filled);
        }

        if (tmp_difference < thread_difference) {
          thread_difference = tmp_difference;
          thread_x = x_i; thread_y = y_i;
        }
      }
    }

    // When several positions are equally good, keep the first one in the
    // order of the loops above, whichever thread found it. This way the
    // result doesn't depend on the number of threads.
    #pragma omp critical
    {
      if (thread_difference < best_difference
          || (thread_difference == best_difference
              && thread_difference < INFINITY
              && (thread_x < best_x
                  || (thread_x == best_x && thread_y < best_y)))) {
        best_difference = thread_difference;
        best_x = thread_x; best_y = thread_y;
      }
    }
  }

  if (best_difference < INFINITY) {
    resultat[0] = best_x; resultat[1] = best_y;
  }
  return;
}
