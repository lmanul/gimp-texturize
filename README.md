# GIMP Plug-In Texturize

This plugin takes a small image and creates a texture out of it.  The idea is to
put several copies (patches) of the small image on the big canvas. The copies
aren't complete: we cut the border so that the transition is invisible, and the
texture seems natural.

# Examples

https://lmanul.github.io/gimp-texturize/examples.html

# Dependencies

If you're using a Debian-based system, install the build dependencies

    sudo apt build-dep gimp-texturize meson

Under a different OS, install
- `gimp` (you probably already have that)
- `libgimp3.0-dev` (libraries for developping with GIMP)
- `gettext` (an internationalization tool).
- `meson` (the build tool we use)

# Installation

To build and install the plugin:

    ./compile

Then (close and) reopen the GIMP. Texturize will be in the Filters->Map menu.

This plugin is based on the article "Graphcut Textures: Image and Video Synthesis Using
Graph Cuts" by Vivek Kwatra, Arno Schödl, Irfan Essa, Greg Turk and Aaron
Bobick, available from www.cc.gatech.edu/cpl/projects/graphcuttextures.


Copyright (C) 2004-2025

* Manu Cornet            <m@ma.nu>
* Jean-Baptiste Rouquier <firstname.lastname@ens-lyon.org>
