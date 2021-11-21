#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "interface.h"
#include "render.h"
#include "texturize.h"

#include "plugin-intl.h"

/*  Constants  */
#define PROCEDURE_NAME   "gimp_plugin_texturize"
#define DATA_KEY_VALS    "plug_in_texturize"
#define DATA_KEY_UI_VALS "plug_in_texturize_ui"
#define PARASITE_KEY     "plug-in-texturize-options"

/*  Local function prototypes  */
static void query (void);
static void run(const gchar      *name,
                gint              nparams,
                const GimpParam  *param,
                gint             *nreturn_vals,
                GimpParam       **return_vals);

/*  Local variables  */
const PlugInVals default_vals = {
  0,      // width_i
  0,      // height_i
  5,      // overlap
  FALSE,  // tileable
};

const PlugInImageVals default_image_vals = {
  0,        // image_id
  500,      // width_p
  500       // height_p
};

const PlugInDrawableVals default_drawable_vals = {
  0
};

const PlugInUIVals default_ui_vals = {
  TRUE
};

static PlugInVals         vals;
static PlugInImageVals    image_vals;
static PlugInDrawableVals drawable_vals;
static PlugInUIVals       ui_vals;


GimpPlugInInfo PLUG_IN_INFO = {
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void query(void) {
  gchar *help_path;
  gchar *help_uri;

  static GimpParamDef args[] = {
    { GIMP_PDB_INT32,    "run_mode",   "Interactive, non-interactive"    },
    { GIMP_PDB_IMAGE,    "image",      "Input image"                     },
    { GIMP_PDB_DRAWABLE, "drawable",   "Input drawable"                  },
    { GIMP_PDB_INT32,    "width_i",    "New image width"                 },
    { GIMP_PDB_INT32,    "height_i",   "New image height"                },
    { GIMP_PDB_INT32,    "overlap",    "Overlap"                         },
    { GIMP_PDB_INT32,    "tileable",   "Tileable"                        },
  };

  gimp_plugin_domain_register(PLUGIN_NAME, LOCALEDIR);

  help_path = g_build_filename(DATADIR, "help", NULL);
  help_uri = g_filename_to_uri(help_path, NULL, NULL);
  g_free (help_path);

  gimp_plugin_help_register("http://www.manucornet.net/Informatique/Texturize.php", help_uri);

  gimp_install_procedure (
    PROCEDURE_NAME,
    "Blurb",
    "Help",
    "Emmanuel Cornet <Manu@manucornet.net>",
    "Jean-Baptiste Rouquier <Jean-Baptiste.Rouquier@ens-lyon.fr>",
    "2007",
    N_("Texturize..."),
    "RGB*, GRAY*, INDEXED*",
    GIMP_PLUGIN,
    G_N_ELEMENTS (args), 0,
    args, NULL);

  gimp_plugin_menu_register (PROCEDURE_NAME, "<Image>/Filters/Map/");
}

static void run (const gchar      *name,
                 gint              n_params,
                 const GimpParam  *param,
                 gint             *nreturn_vals,
                 GimpParam       **return_vals) {
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  gint32             image_ID;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  gint32 new_image_id=0;

  *nreturn_vals = 1;
  *return_vals  = values;

  /*  Initialize i18n support  */
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
  textdomain (GETTEXT_PACKAGE);

  run_mode = (GimpRunMode)param[0].data.d_int32;
  image_ID = param[1].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  Initialize with default values  */
  vals          = default_vals;
  image_vals    = default_image_vals;
  drawable_vals = default_drawable_vals;
  ui_vals       = default_ui_vals;

  image_vals.width_p  = gimp_image_width(image_ID);
  image_vals.height_p = gimp_image_height(image_ID);

  if (strcmp (name, PROCEDURE_NAME) == 0) {
    switch (run_mode) {
    case GIMP_RUN_NONINTERACTIVE:
      if (n_params != 7) {
        status = GIMP_PDB_CALLING_ERROR;
      } else {
        vals.width_i       = param[3].data.d_int32;
        vals.height_i      = param[4].data.d_int32;
        vals.overlap       = param[5].data.d_int32;
        vals.make_tileable = param[6].data.d_int32;

        // vals.random_seed = param[7].data.d_int32;
        // if (vals.random_seed)
        //   vals.seed = g_random_int ();
      }
      break;

    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (DATA_KEY_VALS,    &vals);
      gimp_get_data (DATA_KEY_UI_VALS, &ui_vals);

      if (! dialog(image_ID, drawable,
                   &vals, &image_vals, &drawable_vals, &ui_vals)) {
        status = GIMP_PDB_CANCEL;
      }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (DATA_KEY_VALS, &vals);

      // if (vals.random_seed)
      //   vals.seed = g_random_int ();
      break;

    default:
      break;
    }
  } else {
    status = GIMP_PDB_CALLING_ERROR;
  }

  if (status == GIMP_PDB_SUCCESS) {
    new_image_id = render (image_ID, drawable,
                           &vals, &image_vals, &drawable_vals);

    if (run_mode != GIMP_RUN_NONINTERACTIVE)
      gimp_displays_flush ();

    if (run_mode == GIMP_RUN_INTERACTIVE) {
      gimp_set_data (DATA_KEY_VALS,    &vals,    sizeof (vals));
      gimp_set_data (DATA_KEY_UI_VALS, &ui_vals, sizeof (ui_vals));
    }

    gimp_drawable_detach (drawable);

    // If new_image_id = -1, there has been an error (indexed colors ?).
    if (new_image_id != -1)
      gimp_display_new(new_image_id);
    else {
      g_message(_("There was a problem when opening the new image."));
    }
 }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}
