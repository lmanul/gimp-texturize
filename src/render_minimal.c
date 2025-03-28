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

// Dumbed down version of this function just to show a minimal example.
GimpImage* render(GimpDrawable *drawable, gint unused_width_i,
    gint unused_height_i, gint overlap, gboolean tileable) {
  gint width_p = gimp_drawable_get_width(drawable);
  gint height_p = gimp_drawable_get_height(drawable);
  GimpImageBaseType image_type = GIMP_RGB;
  GimpImageType     drawable_type = GIMP_RGB_IMAGE;

  gimp_progress_init(_("Texturizing image..."));
  gimp_progress_update(0.0);

  GeglRectangle rect = { 0, 0, width_p, height_p };
  const Babl* format = babl_format("RGB u8");
  gint channels = gimp_drawable_get_bpp(drawable);

  // The initial image data, renamed to "patch".
  gpointer patch =
      g_new(guchar, rect.width * rect.height * channels);
  GeglBuffer* buffer_in = gimp_drawable_get_buffer(drawable);
  // patch = destination
  gegl_buffer_get(buffer_in, &rect, 1.0, format, patch,
      GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  // Create a new image with only one layer.
  GimpImage* new_image = gimp_image_new(width_p, height_p, image_type);
  GimpLayer* new_layer = gimp_layer_new(new_image, _("Texture"),
      rect.width, rect.height, drawable_type, 100.0,
      GIMP_LAYER_MODE_NORMAL);
  GeglBuffer* dest_buffer = gimp_drawable_get_buffer(GIMP_DRAWABLE(new_layer));
  gegl_buffer_set(dest_buffer, &rect, 0, format, patch, GEGL_AUTO_ROWSTRIDE);

  // Not sure if any/all of these are necessary.
  gimp_drawable_update(GIMP_DRAWABLE(new_layer),
      rect.x, rect.y, rect.width, rect.height);
  gimp_image_insert_layer(new_image, new_layer, NULL /* parent */, 0);
  gimp_displays_flush();

  g_free(patch);

  return new_image;
}
