#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimp/gimpimage_pdb.h>
#include "plugin-intl.h"

#include "main.h"
#include "render.h"
#include "texturize.h"

/*  Public functions  */

gint32
render (gint32        image_ID,
	GimpDrawable       *drawable,
	PlugInVals         *vals,
	PlugInImageVals    *image_vals,
	PlugInDrawableVals *drawable_vals)
{

/////////////////////                               ////////////////////
/////////////////////   Déclarations de variables   ////////////////////
/////////////////////                               ////////////////////

  gint32 new_image_id = 0;
  gint32 new_layer_id = 0;
  GimpDrawable *    new_drawable;
  GimpImageBaseType image_type = GIMP_RGB;
  GimpImageType     drawable_type = GIMP_RGB_IMAGE;
  gint32            drawable_id = drawable->drawable_id;

  GimpPixelRgn rgn_in, rgn_out;
  gint width_i, height_i, width_p, height_p;
  gint channels; // 3 pour RVB, 1 pour grayscale

  gint k, x_i, y_i; // Moult compteurs

  guchar * patch; // Stockage de l'image originale
  guchar * image; // Zone qui permet de transférer l'image actuelle dans un tableau 3d

  // Pour stocker les pixels qu'on a jetés le long des coupes.
  guchar * coupe_h_here;  // pixel (x,y) du patch auquel appartient le
                          // pixel de gauche (on n'utilisera donc pas la
                          // 1ere colonne de ce tableau)

  guchar * coupe_h_west;  // pixel de gauche du patch auquel appartient
                          // le pixel (x,y) (id pr la 1ere colonne)
  guchar * coupe_v_here;  // pixel (x,y) du patch auquel appartient le
                          // pixel d'au dessus (on n'utilisera pas la
                          // 1ere ligne de ce tableau)
  guchar * coupe_v_north; // pixel d'au dessus du patch auquel appartient
			  // le pixel (x,y) (id pr la 1ere ligne)

  guchar ** rempli; // Pour savoir quels sont les pixels déjà remplis
  //0 ssi le pixel n'est pas rempli
  //1 si le pixel est rempli et sans coupes
  //3 s'il y a une coupe vers le haut
  //5 si vers la gauche
  //7 si les deux
  //C'est-à-dire que le bit de poids faible est "rempli?", le précédent est "coupe_haut?".

  int cur_posn[2];          // La position du pixel à remplir
  int patch_posn[2];        // La position où l'on va coller le patch pour remplir ce pixel
  int x_off_min, y_off_min; // Valeurs max et min de l'offset,
  int x_off_max, y_off_max; // ie vecteur retranché à cur_posn pour obtenir patch_posn

  float progress; // barre de progression affichée pendant le traitement
  gimp_progress_init ("Texturizing image...");

///////////////////////                           //////////////////////
///////////////////////   Dimensions de l'image   //////////////////////
///////////////////////                           //////////////////////

  width_i  = image_vals->width_i;
  height_i = image_vals->height_i;
  width_p  = image_vals->width_p;
  height_p = image_vals->height_p;
  channels = gimp_drawable_bpp (drawable->drawable_id);

  //g_warning ("Tileable : %i\n", vals->make_tileable);

  /* On détermine le type de l'image et on choisit celui de la
   * nouvelle image en conséquence */
  switch (gimp_drawable_type (drawable_id)) {
  case GIMP_RGB_IMAGE:
  case GIMP_RGBA_IMAGE:
    image_type    = GIMP_RGB;
    drawable_type = GIMP_RGB_IMAGE;
    break;
  case GIMP_GRAY_IMAGE:
  case GIMP_GRAYA_IMAGE:
    image_type    = GIMP_GRAY;
    drawable_type = GIMP_GRAY_IMAGE;
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

////////////////////////////                  ///////////////////////////
////////////////////////////   Recouvrement   ///////////////////////////
////////////////////////////                  ///////////////////////////

  /* ATTENTION : ici les conventions ne sont pas forcément intuitives.
   * Avec la façon dont on détecte à chaque fois le prochain pixel à
   * remplir, les offsets sont toujours des valeurs négatives (on pose
   * le patch vers le haut et la gauche). En revanche, {x,y}_off_* sont
   * positifs, avec x_off_max < x_off_min. */


  // Valeurs heuristiques à affiner quand on aura de l'expérience.
  x_off_min = MIN (vals->overlap, width_p - 1);
  y_off_min = MIN (vals->overlap, height_p - 1);
  x_off_max = CLAMP (20, x_off_min/3, width_p -1); /* On sait x_off_min/5 < width_p -1 */
  y_off_max = CLAMP (20, y_off_min/3, height_p - 1); /* On sait y_off_min/5 < height_p-1 */

//////////////////                                     /////////////////
//////////////////   Nouvelle image, initialisations   /////////////////
//////////////////                                     /////////////////

  // On crée une nouvelle image avec un seul calque
  new_image_id = gimp_image_new (width_i,height_i,image_type);
  new_layer_id = gimp_layer_new (new_image_id, "Texture",
				 width_i, height_i,
				 drawable_type, 100, GIMP_NORMAL_MODE);
  gimp_image_add_layer (new_image_id, new_layer_id, 0);
  new_drawable = gimp_drawable_get (new_layer_id);

  // On initialise les régions de destination et de départ
  gimp_pixel_rgn_init (&rgn_out, new_drawable, 0, 0, width_i, height_i, TRUE, TRUE);
  gimp_pixel_rgn_init (&rgn_in, drawable, 0, 0, width_p, height_p, FALSE, FALSE);

  // On alloue de la mémoire pour tout le monde
  patch = g_new (guchar,width_p * height_p * channels);
  image = g_new (guchar,width_i * height_i * channels);
  rempli = init_guchar_tab_2d (width_i, height_i);

  coupe_h_here  = g_new (guchar, width_i * height_i * channels);
  coupe_h_west  = g_new (guchar, width_i * height_i * channels);
  coupe_v_here  = g_new (guchar, width_i * height_i * channels);
  coupe_v_north = g_new (guchar, width_i * height_i * channels);

  // Par sécurité, on initialise à 0.
  for (k = 0; k < width_i * height_i * channels; k++)
    coupe_h_here[k] = coupe_h_west[k] = coupe_v_here[k] = coupe_v_north[k] = 0;

//////////////////                                    /////////////////
//////////////////   Nettoyage de la nouvelle image   /////////////////
//////////////////                                    /////////////////


  // On récupère l'image de départ dans la variable patch
  gimp_pixel_rgn_get_rect (&rgn_in, patch, 0, 0, width_p, height_p);

 // Puis on colle un premier patch en (0,0) de l'image d'arrivée
  gimp_pixel_rgn_set_rect (&rgn_out, patch, 0, 0, width_p, height_p);

  // On dit qu'on a déjà rempli les pixels correspondants
  for (x_i = 0; x_i < width_p; x_i++) {
    for (y_i = 0; y_i < height_p; y_i++) rempli[x_i][y_i] = 1;
  }

  // On récupère toute l'image courante dans image
  gimp_pixel_rgn_get_rect (&rgn_out, image, 0, 0, width_i, height_i);


/////////////////////////                      ////////////////////////
/////////////////////////   La grande boucle   ////////////////////////
/////////////////////////                      ////////////////////////


  // La position courante : (0,0)
  cur_posn[0] = 0; cur_posn[1] = 0;

  while (compter_remplis (rempli,width_i,height_i) < (width_i * height_i)) {
    /* On met à jour la position courante : c'est le prochain pixel
     * à remplir. */
    if (pixel_a_remplir (rempli, width_i, height_i, cur_posn) == NULL) {
      g_message (_("There was a problem when filling the new image."));
      exit(-1);
    };

    offset_optimal (patch_posn,
		    image, patch,
		    width_p, height_p, width_i, height_i,
		    cur_posn[0] - x_off_min,
		    cur_posn[1] - y_off_min,
		    cur_posn[0] - x_off_max,
		    cur_posn[1] - y_off_max,
		    channels,
		    rempli,
		    vals->make_tileable);

    decoupe_graphe (patch_posn,
		    width_i, height_i, width_p, height_p,
		    channels,
		    rempli,
		    image,
		    patch,
		    coupe_h_here, coupe_h_west, coupe_v_here, coupe_v_north,
		    vals->make_tileable,
		    FALSE);

    // On affiche la progression du traitement
    progress = ((float) compter_remplis (rempli, width_i, height_i)) / ((float)(width_i * height_i));
    gimp_progress_update(progress);
  }


//////////////////////                             /////////////////////
//////////////////////   Derniers coups de balai   /////////////////////
//////////////////////                             /////////////////////


/*
  //Pour voir où passent les coupes
  guchar * image_coupes;
  image_coupes = g_new(guchar, width_i*height_i*channels);
  for (k=0;k<width_i*height_i*channels;k++) image_coupes[k] = 255;

  for(x_i=1; x_i<width_i; x_i++){
    for(y_i=1; y_i<height_i; y_i++){
      guchar r = rempli[x_i][y_i];
      if (HAS_CUT_NORTH(r) || HAS_CUT_WEST(r)){
        for (k=0; k<channels; k++)
          image_coupes[(y_i*width_i +x_i)*channels +k] = 0;
      }
//      if (HAS_CUT_WEST(r))
//        image_coupes[(y_i*width_i +x_i)*channels +channels-1] = 255;
    }
  }
*/


  gimp_pixel_rgn_set_rect (&rgn_out, image, 0, 0, width_i, height_i);

  gimp_drawable_flush (new_drawable);
  gimp_drawable_merge_shadow (new_drawable->drawable_id, TRUE);
  gimp_drawable_update (new_drawable->drawable_id, 0, 0, width_i, height_i);
  gimp_drawable_detach (new_drawable);
  gimp_displays_flush ();

  g_free (patch);
  g_free (coupe_h_here);
  g_free (coupe_h_west);
  g_free (coupe_v_here);
  g_free (coupe_v_north);

  /* Enfin, on retourne l'identifiant de la nouvelle image pour
   * que la fonction main puisse ouvrir une nouvelle fenêtre et
   * l'afficher */
  return new_image_id;
}
