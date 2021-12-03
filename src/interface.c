#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimpwidgets/gimpwidgets.h>

#include "main.h"
#include "interface.h"
#include "texturize.h"

#include "plugin-intl.h"


/*  Constants  */

#define SCALE_WIDTH        180
#define SPIN_BUTTON_WIDTH   75
#define RANDOM_SEED_WIDTH  100


/*  Local function prototypes  */

/*  Local variables  */

static PlugInUIVals *ui_state = NULL;


/*  Public functions  */

gboolean dialog (gint32              image_ID,
                 GimpDrawable       *drawable,
                 PlugInVals         *vals,
                 PlugInImageVals    *image_vals,
                 PlugInDrawableVals *drawable_vals,
                 PlugInUIVals       *ui_vals) {
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *tileable_checkbox;
  GtkObject *adj;
  gint row;
  gboolean   run = FALSE;

  image_vals->width_p  = gimp_image_width(image_ID);
  image_vals->height_p = gimp_image_height(image_ID);
  // Here are the default values of the dialog box
  if (vals->width_i < image_vals->width_p
      || vals->height_i < image_vals->height_p) {
    vals->width_i  = 2 * image_vals->width_p;
    vals->height_i = 2 * image_vals->height_p;
  }

  vals->overlap = 100;
  vals->make_tileable = FALSE;

  ui_state = ui_vals;

  gimp_ui_init (PLUGIN_NAME, TRUE);

  dlg = gimp_dialog_new(
    _("Texturize Plug-in for GIMP"),
    PLUGIN_NAME,
    NULL, GTK_DIALOG_MODAL,
    gimp_standard_help_func, "plug-in-template",
    _("_Cancel"), GTK_RESPONSE_CANCEL,
    _("_OK"), GTK_RESPONSE_OK,
    NULL);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_vbox);

  // Size of the new image ?
  frame = gimp_frame_new (_("Please set the size of the new image\nand the maximum overlap between patches."));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  row = 0;

  // Width of the new image?
  adj = gimp_scale_entry_new(GTK_TABLE (table), 0, row++,_("Width :"),
                             SCALE_WIDTH, SPIN_BUTTON_WIDTH,
                             vals->width_i,
                             image_vals->width_p,
                             20 * image_vals->width_p,
                             1, 10, 0, TRUE, 0, 0,
                             _("Set the new image's width"), NULL);

  g_signal_connect(adj, "value_changed",
                   G_CALLBACK (gimp_int_adjustment_update),
                   &vals->width_i);

  // Height of the new image?
  adj = gimp_scale_entry_new(GTK_TABLE (table), 0, row++,_("Height :"),
                             SCALE_WIDTH, SPIN_BUTTON_WIDTH,
                             vals->height_i,
                             image_vals->height_p,
                             20*image_vals->height_p,
                             1, 10, 0, TRUE, 0, 0,
                             _("Set the new image's height"), NULL);

  g_signal_connect(adj, "value_changed",
                   G_CALLBACK (gimp_int_adjustment_update),
                   &vals->height_i);

  // Patch overlap?
  adj = gimp_scale_entry_new(GTK_TABLE (table), 0, row++,
                             _("Overlap (pixels) :"),
                             SCALE_WIDTH, SPIN_BUTTON_WIDTH,
                             vals->overlap,
                             MIN(25, MIN(image_vals->width_p - 1 ,image_vals->height_p - 1)),
                             MIN(image_vals->width_p, image_vals->height_p),
                             5, 10, 0, TRUE, 0, 0,
                             _("Set the overlap between patches (larger values make better "
                               "but longer texturizing "
                               "and tend to make periodic results)"),
                             NULL);

  g_signal_connect(adj, "value_changed",
                   G_CALLBACK (gimp_int_adjustment_update),
                   &vals->overlap);


  // Tileable texture?
  tileable_checkbox = gtk_check_button_new_with_mnemonic(_("_Tileable"));
  gtk_box_pack_start(GTK_BOX (main_vbox), tileable_checkbox, FALSE, FALSE, 0);
  gtk_widget_show(tileable_checkbox);
  gimp_help_set_help_data(tileable_checkbox,
                          _("Selects if to create a tileable texture"),
                          NULL);

  g_signal_connect(GTK_WIDGET (tileable_checkbox), "toggled",
                   G_CALLBACK (gimp_toggle_button_update),
                   &vals->make_tileable);

  // Show the main containers.
  gtk_widget_show(main_vbox);
  gtk_widget_show(dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}
