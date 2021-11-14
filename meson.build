project('texturize', ['c', 'cpp'])

gimp_dep = dependency('gimp-2.0')
# gimpwidgets_dep = dependency('gimpwidgets-2.0')
gtk_dep = dependency('gtk+-2.0')

texturize_lib = library('texturize-lib', [
         'src/compter.c',
         'src/graph.cpp',
         'src/graphcut.cpp',
         'src/initialiser.c',
         'src/interface.c',
         'src/maxflow.cpp',
         'src/offset.c',
         'src/render.c'],
        dependencies: [gimp_dep, gtk_dep])

texturize_dep = declare_dependency(link_with: texturize_lib)

executable('texturize', ['extra_config.h', 'src/main.c'],
          dependencies: [gimp_dep, gtk_dep, texturize_dep])
