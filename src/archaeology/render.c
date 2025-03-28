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