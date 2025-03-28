#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gegl.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "plugin-intl.h"

#include "main.h"
#include "texturize.h"

// Allocates enough memory for a 2-dimensional table of guchars and
// initializes all elements to zero.
guchar** init_guchar_tab_2d (gint x, gint y) {
  guchar** tab;
  tab = (guchar**) malloc (x * sizeof (guchar*));

  for (gint i = 0; i < x; i++) {
    tab[i] = (guchar*) malloc (y * sizeof (guchar));
  }

  for (gint i = 0; i < x; i++) {
    for (gint j = 0; j < y; j++) {
      tab[i][j] = 0;
    }
  }
  return tab;
}

void debug_print_guchar_buffer(gpointer buffer, int width, int height) {
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      printf("%i ", ((guchar*)buffer)[row * width + col]);
    }
  }
}

GimpImage* render(GimpDrawable *drawable, gint width_i, gint height_i,
    gint overlap, gboolean tileable) {
  gint width_p = gimp_drawable_get_width(drawable);
  gint height_p = gimp_drawable_get_height(drawable);
  GimpImageBaseType image_type = GIMP_RGB;
  GimpImageType     drawable_type = GIMP_RGB_IMAGE;
  printf("Patch width %i\n", width_p);
  printf("Image width %i\n", width_i);

  gimp_progress_init(_("Texturizing image..."));
  gimp_progress_update(0);
  float progress = 0; // Progress bar displayed during computation.

  GeglRectangle rect_image = { 0, 0, width_i, height_i };
  GeglRectangle rect_patch = { 0, 0, width_p, height_p };
  const Babl* format;
  gint channels = gimp_drawable_get_bpp(drawable);  // 3 for RGB, 1 for grayscale

  if (width_i == width_p && height_i == height_p) {
    g_message(_("New image size and original image size are the same."));
    return NULL;
  } else if (width_i <= width_p || height_i <= height_p) {
    g_message(_("New image size is too small."));
    return NULL;
  }

  // Figure out the type of the new image according to the original image
  switch (gimp_drawable_type(drawable)) {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      image_type    = GIMP_RGB;
      drawable_type = GIMP_RGB_IMAGE;
      format = babl_format("RGB u8");
      break;
    case GIMP_GRAY_IMAGE:
    case GIMP_GRAYA_IMAGE:
      image_type    = GIMP_GRAY;
      drawable_type = GIMP_GRAY_IMAGE;
      format = babl_format("Y u8");
      break;
    case GIMP_INDEXED_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      g_message (_("Sorry, the Texturize plugin only supports RGB and "
        "grayscale images. Please convert your image to RGB first."));
      return NULL;
    }

    if (gimp_drawable_has_alpha(drawable)) {
      g_message (_("Sorry, the Texturize plugin doesn't support images with "
        "an alpha (i.e. transparency) channel yet. Please convert your image "
        "first."));
      return NULL;
    }

  ////////////////////////////                  ///////////////////////////
  ////////////////////////////      Overlap     ///////////////////////////
  ////////////////////////////                  ///////////////////////////

  // WARNING: our conventions here aren't necessarily intuitive. Given the way
  // that we detect the next pixel to fill, offsets are always negative values
  // (we paste the patch a little above and to the left). However, {x,y}_off_*
  // are positive values, and x_off_max < x_off_min.

  // Heuristic values, to refine when we get more experience.
  int x_off_min, y_off_min; // Max and min values of the offset, i.e. the vector
  int x_off_max, y_off_max; // subtracted from cur_posn to get patch_posn.
  x_off_min = MIN(overlap, width_p - 1);
  y_off_min = MIN(overlap, height_p - 1);
  x_off_max = CLAMP(20, x_off_min/3, width_p -1);  // We know that x_off_min/5 < width_p -1
  y_off_max = CLAMP(20, y_off_min/3, height_p - 1);  // We know that y_off_min/5 < height_p-1

  // Keeps track of which pixels have been filled.
  guchar** filled = init_guchar_tab_2d (rect_image.width, rect_image.height);
  // 0 iff the pixel isn't filled
  // 1 if the pixel is filled and without any cuts
  // 3 if there is an upwards cut
  // 5 if there is a cut towards the left
  // 7 if both
  // I.e. the weak bit is "filled?", the previous bit is "upwards_cut?".

  // These are for storing the pixels we have discarded along the cuts.
  guchar* coupe_h_here;  // pixel (x,y) of the patch to which belongs the pixel
  // on the left (we will thus not use the first column of this array).

  guchar* coupe_h_west;  // Pixel to the left of the patch to which belongs the
  // pixel (x,y) (same for the first column).

  guchar* coupe_v_here;  // pixel (x,y) of the patch to which belongs the pixel
  // to the top (we will thus not use the first row of this array).

  guchar* coupe_v_north; // Pixel to the top of the patch to which belongs the
  // pixel (x,y) (same for the first row).

  int cur_posn[2];          // The position of the pixel to be filled.
  int patch_posn[2];        // Where we'll paste the patch to fill this pixel.
  // GeglBuffer* buffer_out;

  // The initial image data, renamed to "patch".
  guchar* patch =
      g_new(guchar, rect_patch.width * rect_patch.height * channels);
  GeglBuffer* buffer_in = gimp_drawable_get_buffer(drawable);
  // patch = destination
  gegl_buffer_get(buffer_in, &rect_patch, 1.0, format, patch,
      GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  debug_print_guchar_buffer(patch, width_p, height_p);

  // Create a new image with only one layer.
  GimpImage* new_image = gimp_image_new(width_i, height_i, image_type);
  GimpLayer* new_layer = gimp_layer_new(new_image, _("Texture"),
      rect_image.width, rect_image.height, drawable_type, 100,
      GIMP_LAYER_MODE_NORMAL);
  GeglBuffer* dest_buffer = gimp_drawable_get_buffer(GIMP_DRAWABLE(new_layer));

  // The destination image.
  guchar* image =
    g_new(guchar, rect_image.width * rect_image.height * channels);

  // Paste a first patch at position (0,0) of the out image.
  // TODO: is there a shortcut for this low-level operation?
  for (int row = 0; row < height_p; row++) {
    // Copy row by row
    memcpy(image + row * width_i * channels, patch + row * width_p * channels, channels * width_p);
  }

  // And declare we have already filled in the corresponding pixels.
  for (int x_i = 0; x_i < width_p; x_i++) {
    for (int y_i = 0; y_i < height_p; y_i++) {
      filled[x_i][y_i] = 1;
    }
  }

  coupe_h_here  = g_new(guchar, rect_image.width * rect_image.height * channels);
  coupe_h_west  = g_new(guchar, rect_image.width * rect_image.height * channels);
  coupe_v_here  = g_new(guchar, rect_image.width * rect_image.height * channels);
  coupe_v_north = g_new(guchar, rect_image.width * rect_image.height * channels);
  // For security, initialize everything to 0.
  for (int k = 0; k < width_i * height_i * channels; k++) {
    coupe_h_here[k] = coupe_h_west[k] = coupe_v_here[k] = coupe_v_north[k] = 0;
  }

  gimp_progress_update(1.0);

  gegl_buffer_set(dest_buffer, &rect_image, 0, format, image, GEGL_AUTO_ROWSTRIDE);
  // Necessary for the drawable to update
  g_object_unref(dest_buffer);

  gimp_drawable_update(GIMP_DRAWABLE(new_layer),
      rect_patch.x, rect_patch.y, rect_patch.width, rect_patch.height);
  gimp_image_insert_layer(new_image, new_layer, NULL /* parent */, 0);
  gimp_displays_flush();

  g_free(coupe_h_here);
  g_free(coupe_h_west);
  g_free(coupe_v_here);
  g_free(coupe_v_north);
  g_free(patch);
  g_free(filled);

  return new_image;
}
