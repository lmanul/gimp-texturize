project(
  'texturize',
  ['c', 'cpp'],
  meson_version: '>= 0.51.0',
)

i18n = import('i18n')

gettext_package = 'gimp-texturize'

gimp_dep = dependency('gimp-3.0')
gtk_dep = dependency('gtk+-3.0')
glib_dep = dependency('glib-2.0')
# TODO: Remove:
gimpui_dep = dependency('gimpui-3.0')

prefix = get_option('prefix')
# Once we bump meson version to 0.58, we will be able to use positional argument
# https://mesonbuild.com/Release-notes-for-0-58-0.html#depget_variablevarname
gimp_libdir = gimp_dep.get_variable(
  pkgconfig: 'gimplibdir',
  pkgconfig_define: ['prefix', prefix],
)
texturize_plugin_dir = gimp_libdir / 'plug-ins' / 'texturize'

datadir = prefix / get_option('datadir')
localedir = prefix / get_option('localedir')

configure_include_dir = include_directories('.')
conf_data = configuration_data()
conf_data.set10(
  'ENABLE_NLS',
  true,
  description: 'always defined to indicate that i18n is enabled',
)
conf_data.set_quoted(
  'GETTEXT_PACKAGE',
  gettext_package,
  description: 'The gettext translation domain.',
)
conf_data.set10(
  'HAVE_BIND_TEXTDOMAIN_CODESET',
  meson.get_compiler('c').has_function('bind_textdomain_codeset'),
  description: 'Define to 1 if you have the “bind_textdomain_codeset” function.',
)
conf_data.set_quoted(
  'PLUGIN_NAME',
  meson.project_name(),
  description: 'Plug-In name',
)
conf_data.set_quoted(
  'DATADIR',
  datadir,
)
conf_data.set_quoted(
  'LOCALEDIR',
  datadir,
)

configure_file(
  output: 'config.h',
  configuration: conf_data,
)

sources = [
#  'src/compter.c',
#  'src/compter.h',
#  'src/graph.cpp',
#  'src/graphcut.cpp',
#  'src/initialize.c',
#  'src/interface.c',
#  'src/maxflow.cpp',
  'src/main.c',
#  'src/offset.c',
#  'src/render.c'
  'src/render_minimal.c'
]

executable(
  'texturize',
  sources,
  dependencies: [
    gimp_dep,
    gtk_dep,
    gimpui_dep,
  ],
  include_directories: configure_include_dir,
  install: true,
  install_dir: texturize_plugin_dir,
)

install_subdir(
  'help',
  install_dir: datadir / 'gimp-texturize',
)

subdir('po')
