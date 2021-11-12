#ifndef __MAIN_H__
#define __MAIN_H__


typedef struct {
  gint     width_i;
  gint     height_i;
  gint     overlap;
  gboolean make_tileable;
} PlugInVals;

typedef struct {
  gint32 image_id;
  gint width_p;
  gint height_p;
} PlugInImageVals;

typedef struct {
  gint32    drawable_id;
} PlugInDrawableVals;

typedef struct {
  gboolean  chain_active;
} PlugInUIVals;


/*  Default values  */

extern const PlugInVals         default_vals;
extern const PlugInImageVals    default_image_vals;
extern const PlugInDrawableVals default_drawable_vals;
extern const PlugInUIVals       default_ui_vals;


#endif /* __MAIN_H__ */
