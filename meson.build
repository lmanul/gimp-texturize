project('texturize', ['c', 'cpp'])

gimp_dep = dependency('gimp-2.0')
gtk_dep = dependency('gtk+-2.0')
glib_dep = dependency('glib-2.0')
libgimpui = meson.get_compiler('c').find_library('libgimpui-2.0')
libgimpwidgets = meson.get_compiler('c').find_library('libgimpwidgets-2.0')

executable('texturize', [
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
          ],
           dependencies: [
             gimp_dep,
             gtk_dep,
             libgimpui,
             libgimpwidgets,
           ])
