#ifndef __RENDER_H__
#define __RENDER_H__

/*  Public functions  */

GimpImage* render(GimpDrawable *drawable, gint new_width, gint new_height, gint overlap, gboolean tileable);

// gint32 render(gint32     image_ID,
//               GimpDrawable       *drawable,
//               PlugInVals         *vals,
//               PlugInImageVals    *image_vals,
//               PlugInDrawableVals *drawable_vals);

#endif /* __RENDER_H__ */
