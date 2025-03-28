#ifndef __RENDER_H__
#define __RENDER_H__

/*  Public functions  */

GimpImage* render(GimpDrawable *drawable, gint new_width, gint new_height, gint overlap, gboolean tileable);

#endif /* __RENDER_H__ */
