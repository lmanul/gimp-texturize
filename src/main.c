/* Always include this in all plug-ins */
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "render.h"

/* The name of my PDB procedure */
#define PLUG_IN_PROC "plug-in-texturize-c-texturize"
#define PLUG_IN_NAME_CAPITALIZED "Texturize"

#define DEFAULT_NEW_IMAGE_WIDTH 1000;
#define DEFAULT_NEW_IMAGE_HEIGHT 1000;
#define DEFAULT_OVERLAP 300;

/* Our custom class Texturize is derived from GimpPlugIn. */
struct _Texturize {
  GimpPlugIn parent_instance;
};

#define TEXTURIZE_TYPE (texturize_get_type())
G_DECLARE_FINAL_TYPE (Texturize, texturize, TEXTURIZE,, GimpPlugIn)

/* Declarations */
static GList          * texturize_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * texturize_create_procedure (GimpPlugIn           *plug_in,
                                                    const gchar          *name);
static GimpValueArray * texturize_run(GimpProcedure        *procedure,
                                      GimpRunMode           run_mode,
                                      GimpImage            *image,
                                      GimpDrawable        **drawables,
                                      GimpProcedureConfig  *config,
                                      gpointer              run_data);

G_DEFINE_TYPE (Texturize, texturize, GIMP_TYPE_PLUG_IN)

static void texturize_class_init (TexturizeClass *klass) {
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = texturize_query_procedures;
  plug_in_class->create_procedure = texturize_create_procedure;
}

static void texturize_init (Texturize *texturize) {
}

static GList * texturize_query_procedures (GimpPlugIn *plug_in) {
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure * texturize_create_procedure (GimpPlugIn  *plug_in, const gchar *name) {
  GimpProcedure *procedure = NULL;

  if (g_strcmp0 (name, PLUG_IN_PROC) == 0) {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            texturize_run, NULL, NULL);

      gimp_procedure_set_sensitivity_mask (procedure, GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, "Texturize...");
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Map/");

      gimp_procedure_set_documentation (procedure,
                                        "Texturize Plugin for the GIMP",
                                        "Creates seamless textures from existing images. "
                                        "",
                                        NULL);
      gimp_procedure_set_attribution (procedure, "Manu Cornet & Jean-Baptiste Rouquier", "", "2007");

      gimp_procedure_add_int_argument(procedure, "width_i", "New image _width", NULL, 1, 100000, 1000, G_PARAM_READWRITE);
      gimp_procedure_add_int_argument(procedure, "height_i", "New image _height", NULL, 1, 100000, 1000, G_PARAM_READWRITE);
      gimp_procedure_add_int_argument(procedure, "overlap", "O_verlap", "Higher: slower, more seamless but potentially more visible repetition", 5, 1000, 300, G_PARAM_READWRITE);
      gimp_procedure_add_boolean_argument(procedure, "tileable", "_Tileable", "Whether the new image should be tileable", 0, G_PARAM_READWRITE);
    }

  return procedure;
}

#define PLUG_IN_BINARY "texturize"

static GimpValueArray *
texturize_run (GimpProcedure        *procedure,
               GimpRunMode           run_mode,
               GimpImage            *image,
               GimpDrawable        **drawables,
               GimpProcedureConfig  *config,
               gpointer              run_data) {
  // GimpLayer     *parent   = NULL;
  // gint           position = 0;
  gint           n_drawables;

  gint     width_i = DEFAULT_NEW_IMAGE_WIDTH;
  gint     height_i = DEFAULT_NEW_IMAGE_HEIGHT;
  gint     overlap = DEFAULT_OVERLAP;
  gboolean tileable = 0;

  n_drawables = gimp_core_object_array_get_length ((GObject **) drawables);
  printf("Drawable %i\n\n", n_drawables);
  gimp_message ("Texturizing...");

  if (n_drawables > 1 || n_drawables == 0) {
    GError *error = NULL;

    g_set_error (&error, GIMP_PLUG_IN_ERROR, 0, "'%s' works only with a single image.", PLUG_IN_NAME_CAPITALIZED);

    return gimp_procedure_new_return_values(procedure, GIMP_PDB_CALLING_ERROR, error);
  } else if (n_drawables == 1) {
    GimpDrawable *drawable = drawables[0];

    if (!GIMP_IS_LAYER (drawable)) {
        GError *error = NULL;
        g_set_error(&error, GIMP_PLUG_IN_ERROR, 0,
                    "Procedure '%s' works with layers only.",
                    PLUG_IN_NAME_CAPITALIZED);
        return gimp_procedure_new_return_values(procedure, GIMP_PDB_CALLING_ERROR, error);
      }

    // parent = GIMP_LAYER(gimp_item_get_parent(GIMP_ITEM(drawable)));
    // position = gimp_image_get_item_position(image, GIMP_ITEM(drawable));
  }

  if (run_mode == GIMP_RUN_INTERACTIVE) {
    GtkWidget *dialog;

    gimp_ui_init(PLUG_IN_BINARY);
    dialog = gimp_procedure_dialog_new(procedure, GIMP_PROCEDURE_CONFIG (config), PLUG_IN_NAME_CAPITALIZED);
    gimp_procedure_dialog_fill(GIMP_PROCEDURE_DIALOG(dialog), NULL);

    if (!gimp_procedure_dialog_run(GIMP_PROCEDURE_DIALOG(dialog))) {
      return gimp_procedure_new_return_values(procedure, GIMP_PDB_CANCEL, NULL);
    }
  }

  g_object_get (config,
                "width_i", &width_i,
                "height_i", &height_i,
                "overlap", &overlap,
                "tileable", &tileable,
                NULL);
  GimpImage* new_image = render(drawables[0], width_i, height_i, overlap, tileable);
  gimp_display_new(new_image);
  return gimp_procedure_new_return_values(procedure, GIMP_PDB_SUCCESS, NULL);
}

/* Generate needed main() code */
GIMP_MAIN (TEXTURIZE_TYPE)


/*

#include "config.h"

#include <string.h>

#include "interface.h"
#include "render.h"
#include "texturize.h"

#include "plugin-intl.h"

//  Constants
#define PROCEDURE_NAME   "gimp_plugin_texturize"
#define DATA_KEY_VALS    "plug_in_texturize"
#define DATA_KEY_UI_VALS "plug_in_texturize_ui"
#define PARASITE_KEY     "plug-in-texturize-options"

//  Local function prototypes
static void query (void);
static void run(const gchar      *name,
                gint              nparams,
                const GimpParam  *param,
                gint             *nreturn_vals,
                GimpParam       **return_vals);

//  Local variables
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
  NULL,  // init_proc
  NULL,  // quit_proc
  query, // query_proc
  run,   // run_proc
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

  gimp_plugin_help_register("https://lmanul.github.io/gimp-texturize/", help_uri);

  gimp_install_procedure(
    PROCEDURE_NAME,
    "Blurb",
    "Help",
    "Manu Cornet <m@ma.nu>",
    "Jean-Baptiste Rouquier <Jean-Baptiste.Rouquier@ens-lyon.fr>",
    "2007",
    N_("Texturize..."),
    "RGB*, GRAY*, INDEXED*",
    GIMP_PLUGIN,
    G_N_ELEMENTS (args), 0,
    args, NULL);

  gimp_plugin_menu_register (PROCEDURE_NAME, "<Image>/Filters/Map/");
}

static void run(const gchar      *name,
                gint              n_params,
                const GimpParam  *param,
                gint             *nreturn_vals,
                GimpParam       **return_vals) {
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  gint32             image_ID;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  gint32 new_image_id = 0;

  *nreturn_vals = 1;
  *return_vals  = values;

  //  Initialize i18n support
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
  textdomain (GETTEXT_PACKAGE);

  run_mode = (GimpRunMode)param[0].data.d_int32;
  image_ID = param[1].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  //  Initialize with default values
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
      }
      break;

    case GIMP_RUN_INTERACTIVE:
      //  Possibly retrieve data
      gimp_get_data(DATA_KEY_VALS,    &vals);
      gimp_get_data(DATA_KEY_UI_VALS, &ui_vals);

      if (! dialog(image_ID, drawable,
                   &vals, &image_vals, &drawable_vals, &ui_vals)) {
        status = GIMP_PDB_CANCEL;
      }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      //  Possibly retrieve data
      gimp_get_data(DATA_KEY_VALS, &vals);
      break;

    default:
      break;
    }
  } else {
    status = GIMP_PDB_CALLING_ERROR;
  }

  if (status == GIMP_PDB_SUCCESS) {
    new_image_id = render(image_ID, drawable,
                          &vals, &image_vals, &drawable_vals);

    if (run_mode != GIMP_RUN_NONINTERACTIVE)
      gimp_displays_flush();

    if (run_mode == GIMP_RUN_INTERACTIVE) {
      gimp_set_data(DATA_KEY_VALS,    &vals,    sizeof (vals));
      gimp_set_data(DATA_KEY_UI_VALS, &ui_vals, sizeof (ui_vals));
    }

    gimp_drawable_flush(drawable);
    gimp_drawable_merge_shadow(drawable->drawable_id, TRUE);

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

*/