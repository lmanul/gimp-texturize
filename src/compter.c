#include "config.h"
#include <stdio.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "render.h"
#include "texturize.h"

#include "plugin-intl.h"

// Counts the number of pixels that are already filled.
int count_filled_pixels(guchar **filled, int width_i, int height_i) {
  int x_i, y_i;
  int somme = 0;

  for (x_i = 0; x_i < width_i; x_i++) {
    for (y_i = 0; y_i < height_i; y_i++) {
      if (filled[x_i][y_i]) somme++;
    }
  }
  return somme;
}

// Finds the next pixel that needs to be filled.
int* pixel_to_fill(guchar **filled, int width_i, int height_i, int *resultat) {
  int x_i, y_i;

  for (y_i = 0; y_i < height_i; y_i++) {
    for (x_i = 0; x_i < width_i; x_i++) {
      if (!filled[x_i][y_i]) {
        resultat[0] = x_i;
        resultat[1] = y_i;
        return resultat;
      }
    }
  }
  return NULL;
}

// Quick and dirty implementation of x mod m assuming x isn't greater than
// 2 m.
gint modulo(gint x, gint m) {
  int v = (x - (m * (x / m)));
  return (v >= 0)? v : v + m;
}
