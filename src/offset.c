#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "render.h"
#include "texturize.h"

#include "plugin-intl.h"

// Computes the distance between image_tab and patch_tab for the zone that's
//   been filled in:
// (x_min,y_min) -> (x_max,y_max) in image_tab
// (x_min,y_min)-posn -> (x_max,y_max) - posn in patch_tab
float
difference (gint width_i, gint height_i, gint width_p, gint height_p,
            guchar * image, guchar * patch,
            gint posn_x, gint posn_y,
            gint x_min, gint y_min, gint x_max, gint y_max,
            gint channels, guchar ** filled) {
  gint    somme = 0, zone=0, x_i, y_i, k;
  guchar *image_ptr, *patch_ptr;
  gint    real_y_i, real_x_i;

  for (y_i = y_min; y_i < y_max; y_i++) {
    real_y_i = modulo (y_i, height_i);

    image_ptr = image + (real_y_i * width_i
			 + modulo (x_min, width_i)) * channels;
    patch_ptr = patch + ((real_y_i - posn_y) * width_p
			 + modulo (x_min, width_i) - posn_x) * channels;

    for (x_i = x_min; x_i < x_max; x_i++) {
      real_x_i = modulo (x_i, width_i);

      if (filled[real_x_i][real_y_i]) {
        for (k = 0 ; k < channels; k++) {
          somme += abs (*image_ptr++ - *patch_ptr++);
          zone ++;
        }
      } else {
        image_ptr += channels;
        patch_ptr += channels;
      }
    }
  }
  if (zone == 0) {printf("Bug: Zone = 0\n"); exit(-1);}
  return (((float) somme) / ((float) zone));
}

void
offset_optimal (gint    *resultat,
                guchar  *image, guchar *patch,
                gint     width_p, gint height_p, gint width_i, gint height_i,
                gint     x_patch_posn_min, gint y_patch_posn_min, gint x_patch_posn_max, gint y_patch_posn_max,
                gint     channels, guchar **filled,
                gboolean tileable) {
  gint x_i, y_i;
  float best_difference = INFINITY, tmp_difference;

  for (x_i = x_patch_posn_min; x_i < x_patch_posn_max; x_i++) {
    for (y_i = y_patch_posn_min; y_i < y_patch_posn_max; y_i++) {
      if (tileable) { /* Move this test outside the loops? */
        tmp_difference = difference (
          width_i, height_i,
          width_p, height_p,
          image, patch,
          x_i, y_i,
          MAX (0, x_i), MAX (0, y_i), x_i + width_p, y_i + height_p,
          channels, filled);
      } else {
        tmp_difference = difference (
          width_i, height_i,
          width_p, height_p,
          image, patch,
          x_i, y_i,
          MAX (0,x_i), MAX (0,y_i), MIN (x_i + width_p, width_i), MIN (y_i + height_p, height_i),
          channels, filled);
      }
      if (tmp_difference < best_difference) {
        best_difference = tmp_difference;
        resultat[0] = x_i; resultat[1] = y_i;
      }
    }
  }
  return;
}
