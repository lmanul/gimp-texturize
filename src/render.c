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

//  Public functions

GimpImage* render(GimpDrawable *drawable, gint width_i, gint height_i, gint overlap, gboolean tileable) {
  gint width_p = gimp_drawable_get_width(drawable);
  gint height_p = gimp_drawable_get_height(drawable);
  printf("Patch width %i\n", width_p);
  printf("Image width %i\n", width_i);

  float progress; // Progress bar displayed during computation.
  gimp_progress_init("Texturizing image...");

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

  guchar** filled; // To keep track of which pixels have been filled.
  // 0 iff the pixel isn't filled
  // 1 if the pixel is filled and without any cuts
  // 3 if there is an upwards cut
  // 5 if there is a cut towards the left
  // 7 if both
  // I.e. the weak bit is "filled?", the previous bit is "upwards_cut?".


  GimpImage* new_image = gimp_image_new(width_i, height_i, GIMP_RGB);
  return new_image;
}


/*
gint32 render(gint32        image_ID,
              GimpDrawable       *drawable,
              PlugInVals         *vals,
              PlugInImageVals    *image_vals,
              PlugInDrawableVals *drawable_vals) {

/////////////////////                               ////////////////////
/////////////////////      Variable declaration     ////////////////////
/////////////////////                               ////////////////////

  gint32 new_image_id = 0;
  gint32 new_layer_id = 0;
  GimpDrawable*     new_drawable;
  GimpImageBaseType image_type = GIMP_RGB;
  GimpImageType     drawable_type = GIMP_RGB_IMAGE;
  gint32            drawable_id = drawable->drawable_id;

  GimpPixelRgn rgn_in, rgn_out;

  gint k, x_i, y_i; // Many counters

  guchar* patch; // To store the original image
  guchar* image; // Buffer to store the current image in a 3d array

  // These are for storing the pixels we have discarded along the cuts.
  guchar* coupe_h_here;  // pixel (x,y) of the patch to which belongs the pixel
                         // on the left (we will thus not use the first
                         // column of this array).

  guchar* coupe_h_west;  // Pixel to the left of the patch to which belongs the
                         // pixel (x,y) (same for the first column).

  guchar* coupe_v_here;  // pixel (x,y) of the patch to which belongs the pixel
                         // to the top (we will thus not use the first
                         // line of this array).

  guchar* coupe_v_north; // Pixel to the top of the patch to which belongs the
                         // pixel (x,y) (same for the first line).


  int cur_posn[2];          // The position of the pixel to be filled.
  int patch_posn[2];        // Where we'll paste the patch to fill this pixel.

///////////////////////                           //////////////////////
///////////////////////      Image dimensions     //////////////////////
///////////////////////                           //////////////////////



  // Figure out the type of the new image according to the original image
  switch (gimp_drawable_type (drawable_id)) {
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
    g_message (_("Sorry, the Texturize plugin only supports RGB and grayscale images. "
		 "Please convert your image to RGB mode first."));
    return -1;
  }

  if (gimp_drawable_has_alpha (drawable_id)) {
    g_message (_("Sorry, the Texturize plugin doesn't support images"
		 " with an alpha (ie transparency) channel yet."
		 " Please flatten your image first."));
    return -1;
  }


//////////////////                                     /////////////////
//////////////////      New image, initializations     /////////////////
//////////////////                                     /////////////////

  // Create a new image with only one layer.
  new_image_id = gimp_image_new(rect_image.width, rect_image.height, image_type);
  new_layer_id = gimp_layer_new(new_image_id, "Texture",
                                rect_image.width, rect_image.height,
                                drawable_type, 100, GIMP_NORMAL_MODE);
  gimp_image_insert_layer(new_image_id, new_layer_id, 0, -1);
  new_drawable = gimp_drawable_get(new_layer_id);

  // Initialize in and out regions.
  gimp_pixel_rgn_init(&rgn_out, new_drawable, rect_image.x, rect_image.y, rect_image.width, rect_image.height, TRUE, TRUE);
  gimp_pixel_rgn_init(&rgn_in, drawable, rect_patch.x, rect_patch.y, rect_patch.width, rect_patch.height, FALSE, FALSE);

  // Allocate some memory for everyone.
  patch = g_new(guchar, rect_patch.width * rect_patch.height * channels);
  image = g_new(guchar, rect_image.width * rect_image.height * channels);
  filled = init_guchar_tab_2d (rect_image.width, rect_image.height);

  coupe_h_here  = g_new(guchar, rect_image.width * rect_image.height * channels);
  coupe_h_west  = g_new(guchar, rect_image.width * rect_image.height * channels);
  coupe_v_here  = g_new(guchar, rect_image.width * rect_image.height * channels);
  coupe_v_north = g_new(guchar, rect_image.width * rect_image.height * channels);

  // For security, initialize everything to 0.
  for (k = 0; k < width_i * height_i * channels; k++)
    coupe_h_here[k] = coupe_h_west[k] = coupe_v_here[k] = coupe_v_north[k] = 0;

//////////////////                                    /////////////////
//////////////////    Cleaning up of the new image    /////////////////
//////////////////                                    /////////////////

  // Retrieve the initial image into the patch.
  // patch = destination buffer, rgn_in = source
  gimp_pixel_rgn_get_rect (&rgn_in, patch, 0, 0, width_p, height_p);

  // Then paste a first patch at position (0,0) of the out image.
  // patch = source buffer, rgn_out = destination
  gimp_pixel_rgn_set_rect (&rgn_out, patch, 0, 0, width_p, height_p);

  // And declare we have already filled in the corresponding pixels.
  for (x_i = 0; x_i < width_p; x_i++) {
    for (y_i = 0; y_i < height_p; y_i++) filled[x_i][y_i] = 1;
  }

  // Retrieve all of the current image into image.
  // image = destination buffer, rgn_out = source
  gimp_pixel_rgn_get_rect (&rgn_out, image, 0, 0, rect_image.width, rect_image.height);


/////////////////////////                      ////////////////////////
/////////////////////////     The big loop     ////////////////////////
/////////////////////////                      ////////////////////////


  // The current position : (0,0)
  cur_posn[0] = 0; cur_posn[1] = 0;
  int count = count_filled_pixels (filled, rect_image.width, rect_image.height);
  int count_max = rect_image.width * rect_image.height;

  while (count < count_max) {

    // Update the current position: it's the next pixel to fill.
    if (pixel_to_fill (filled, width_i, height_i, cur_posn) == NULL) {
      g_message (_("There was a problem when filling the new image."));
      exit(-1);
    };

    offset_optimal(patch_posn,
                   image, patch,
                   rect_patch.width, rect_patch.height, rect_image.width, rect_image.height,
                   cur_posn[0] - x_off_min,
                   cur_posn[1] - y_off_min,
                   cur_posn[0] - x_off_max,
                   cur_posn[1] - y_off_max,
                   channels,
                   filled,
                   vals->make_tileable);

    cut_graph(patch_posn,
              rect_image.width, rect_image.height, rect_patch.width, rect_patch.height,
              channels,
              filled,
              image,
              patch,
              coupe_h_here, coupe_h_west, coupe_v_here, coupe_v_north,
              vals->make_tileable,
              FALSE);

    // Display progress to the user.
    count = count_filled_pixels (filled, rect_image.width, rect_image.height);
    progress = ((float) count) / ((float) count_max);
    progress = (progress > 1.0f)? 1.0f : ((progress < 0.0f)? 0.0f : progress);
    gimp_progress_update(progress);
  }


//////////////////////                             /////////////////////
//////////////////////        Last clean up        /////////////////////
//////////////////////                             /////////////////////

  // image = source buffer, rgn_out = destination
  gimp_pixel_rgn_set_rect(&rgn_out, image, rect_image.x, rect_image.y, rect_image.width, rect_image.height);

  gimp_drawable_flush(new_drawable);
  gimp_drawable_merge_shadow (new_drawable->drawable_id, TRUE);
  gimp_drawable_update(new_drawable->drawable_id, rect_image.x, rect_image.y, rect_image.width, rect_image.height);
  gimp_displays_flush();

  g_free(patch);
  g_free(coupe_h_here);
  g_free(coupe_h_west);
  g_free(coupe_v_here);
  g_free(coupe_v_north);

  // Finally return the ID of the new image, for the main function to display it
  return new_image_id;
}
*/