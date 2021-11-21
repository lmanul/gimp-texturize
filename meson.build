project(
  'texturize',
  ['c', 'cpp'],
  meson_version: '>= 0.51.0',
)

gimp_dep = dependency('gimp-2.0')
gtk_dep = dependency('gtk+-2.0')
glib_dep = dependency('glib-2.0')
libgimpui = meson.get_compiler('c').find_library('libgimpui-2.0')
libgimpwidgets = meson.get_compiler('c').find_library('libgimpwidgets-2.0')

prefix = get_option('prefix')
# Once we bump meson version to 0.58, we will be able to use positional argument
# https://mesonbuild.com/Release-notes-for-0-58-0.html#depget_variablevarname
gimp_libdir = gimp_dep.get_variable(
  pkgconfig: 'gimplibdir',
  pkgconfig_define: ['prefix', prefix],
)
texturize_plugin_dir = gimp_libdir / 'plug-ins' / 'texturize'

sources = [
  'extra_config.h',
  'src/compter.c',
  'src/compter.h',
  'src/graph.cpp',
  'src/graphcut.cpp',
  'src/initialiser.c',
  'src/interface.c',
  'src/maxflow.cpp',
  'src/main.c',
  'src/offset.c',
  'src/render.c'
]

executable(
  'texturize',
  sources,
  dependencies: [
    gimp_dep,
    gtk_dep,
    libgimpui,
    libgimpwidgets,
  ],
  install: true,
  install_dir: texturize_plugin_dir,
)
