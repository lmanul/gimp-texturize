/* Always include this in all plug-ins */
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "render.h"

/* The name of my PDB procedure */
#define PLUG_IN_PROC "plug-in-texturize-c-texturize"
#define PLUG_IN_NAME_CAPITALIZED "Texturize"

#define DEFAULT_NEW_IMAGE_WIDTH 600;
#define DEFAULT_NEW_IMAGE_HEIGHT 600;
#define DEFAULT_OVERLAP 100;

/* Our custom class Texturize is derived from GimpPlugIn. */
struct _Texturize {
  GimpPlugIn parent_instance;
};

#define TEXTURIZE_TYPE (texturize_get_type())
G_DECLARE_FINAL_TYPE (Texturize, texturize, TEXTURIZE,, GimpPlugIn)

/* Declarations */
static GList          * texturize_query_procedures (GimpPlugIn *plug_in);
static GimpProcedure  * texturize_create_procedure (GimpPlugIn  *plug_in,
                                                    const gchar *name);
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

static GimpProcedure * texturize_create_procedure (GimpPlugIn  *plug_in,
    const gchar *name) {
  GimpProcedure *procedure = NULL;

  if (g_strcmp0 (name, PLUG_IN_PROC) == 0) {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            texturize_run, NULL, NULL);

      gimp_procedure_set_sensitivity_mask (procedure,
          GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, "Texturize...");
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Map/");

      gimp_procedure_set_documentation (procedure,
          "Texturize Plugin for the GIMP",
          "Creates seamless textures from existing images. "
          "",
          NULL);
      gimp_procedure_set_attribution (procedure,
          "Manu Cornet & Jean-Baptiste Rouquier", "", "2007");

      gimp_procedure_add_int_argument(procedure, "width_i",
          "New image _width", NULL, 1, 100000, 1000, G_PARAM_READWRITE);
      gimp_procedure_add_int_argument(procedure, "height_i",
          "New image _height", NULL, 1, 100000, 1000, G_PARAM_READWRITE);
      gimp_procedure_add_int_argument(procedure, "overlap", "O_verlap",
          "Higher: slower, more seamless but potentially more visible repetition",
          5, 1000, 300, G_PARAM_READWRITE);
      gimp_procedure_add_boolean_argument(procedure, "tileable", "_Tileable",
          "Whether the new image should be tileable", 0, G_PARAM_READWRITE);
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

  gint           n_drawables;

  gint     width_i = DEFAULT_NEW_IMAGE_WIDTH;
  gint     height_i = DEFAULT_NEW_IMAGE_HEIGHT;
  gint     overlap = DEFAULT_OVERLAP;
  gboolean tileable = 0;

  n_drawables = gimp_core_object_array_get_length ((GObject **) drawables);
  gimp_message ("Texturizing...");

  if (n_drawables > 1 || n_drawables == 0) {
    GError *error = NULL;

    g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
        "'%s' works only with a single image.", PLUG_IN_NAME_CAPITALIZED);

    return gimp_procedure_new_return_values(procedure, GIMP_PDB_CALLING_ERROR,
        error);
  } else if (n_drawables == 1) {
    GimpDrawable *drawable = drawables[0];

    if (!GIMP_IS_LAYER (drawable)) {
        GError *error = NULL;
        g_set_error(&error, GIMP_PLUG_IN_ERROR, 0,
                    "Procedure '%s' works with layers only.",
                    PLUG_IN_NAME_CAPITALIZED);
        return gimp_procedure_new_return_values(procedure,
            GIMP_PDB_CALLING_ERROR, error);
      }
  }

  if (run_mode == GIMP_RUN_INTERACTIVE) {
    GtkWidget *dialog;

    gimp_ui_init(PLUG_IN_BINARY);
    dialog = gimp_procedure_dialog_new(procedure, GIMP_PROCEDURE_CONFIG(config),
        PLUG_IN_NAME_CAPITALIZED);
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
  GimpImage* new_image = render(drawables[0], width_i, height_i, overlap,
      tileable);
  if (new_image == NULL) {
    return gimp_procedure_new_return_values(procedure, GIMP_PDB_EXECUTION_ERROR,
        NULL);
  } else {
    gimp_display_new(new_image);
    gimp_displays_flush();
    return gimp_procedure_new_return_values(procedure, GIMP_PDB_SUCCESS, NULL);
  }
}

GIMP_MAIN (TEXTURIZE_TYPE)
