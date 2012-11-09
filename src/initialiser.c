#include <stdlib.h>

#include <libgimp/gimp.h>
#include "texturize.h"

// Allocates enough memory for a 2-dimensional table of guchars and
// initializes all elements to zero.
guchar **
init_guchar_tab_2d (gint x, gint y) {
  guchar ** tab;
  gint i, j;
  tab = (guchar**) malloc (x * sizeof (guchar*));

  for (i = 0; i < x; i++) {
    tab[i] = (guchar*) malloc (y * sizeof (guchar));
  }

  for (i = 0; i < x; i++) {
    for (j = 0; j < y; j++) tab[i][j] = 0;
  }
  return tab;
}
