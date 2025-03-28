
//////////////////                                    /////////////////
//////////////////    Cleaning up of the new image    /////////////////
//////////////////                                    /////////////////

  // Retrieve the initial image into the patch.
  // patch = destination buffer, rgn_in = source
  gimp_pixel_rgn_get_rect (&rgn_in, patch, 0, 0, width_p, height_p);

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


}