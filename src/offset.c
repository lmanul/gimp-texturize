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

  gint    somme = 0, zone=0;
  gint    x_i, y_i, k;
  guchar *image_ptr, *patch_ptr;

  gint x_p, y_p;
  gint x_i_start, x_p_start;
  gint xcount, ycount;
  gint iy, ix;
  guchar *image_ptr_x, *patch_ptr_x;
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

  for(iy = 0; iy < ycount; iy++) {

    x_i = x_i_start;
    x_p = x_p_start;
    image_ptr = image_ptr_x + x_i * channels;
    patch_ptr = patch_ptr_x + x_p * channels;

    for (ix = 0; ix < xcount; ix++) {
      if (filled[x_i][y_i]) {
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

    if (++y_i >= height_i) { y_i = 0; image_ptr_x = image; }
    if (++y_p >= height_p) { y_p = 0; patch_ptr_x = patch; }
  }

  if (zone == 0) {g_message(_("Bug: Zone = 0")); exit(-1);}
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

  if (tileable) {
    for (x_i = x_patch_posn_min; x_i < x_patch_posn_max; x_i++) {
      for (y_i = y_patch_posn_min; y_i < y_patch_posn_max; y_i++) {
        
        tmp_difference = difference (
          width_i, height_i, width_p, height_p, image, patch,
          x_i, y_i,
          MAX (0, x_i), MAX (0, y_i),
          x_i + width_p, y_i + height_p,
          channels, filled);
        
        if (tmp_difference < best_difference) {
          best_difference = tmp_difference;
          resultat[0] = x_i; resultat[1] = y_i;
        }
      }
    }
  } else {
    for (x_i = x_patch_posn_min; x_i < x_patch_posn_max; x_i++) {
      for (y_i = y_patch_posn_min; y_i < y_patch_posn_max; y_i++) {
        
        tmp_difference = difference (
          width_i, height_i, width_p, height_p, image, patch,
          x_i, y_i,
          MAX (0,x_i), MAX (0,y_i),
          MIN (x_i + width_p, width_i), MIN (y_i + height_p, height_i),
          channels, filled);
        
        if (tmp_difference < best_difference) {
          best_difference = tmp_difference;
          resultat[0] = x_i; resultat[1] = y_i;
        }
      }
    }
  }
  return;
}
