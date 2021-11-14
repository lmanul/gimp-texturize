project('texturize', ['c', 'cpp'])

gimp_dep = dependency('gimp-2.0')
# gimpwidgets_dep = dependency('gimpwidgets-2.0')
gtk_dep = dependency('gtk+-2.0')
glib_dep = dependency('glib-2.0')

graph_lib = library('graph', [
                     'src/compter.c',
                     'src/compter.h',
                     'src/graph.cpp',
                     'src/graphcut.cpp',
                     'src/maxflow.cpp',
                   ], dependencies: [glib_dep])

graph_dep = declare_dependency(link_with: graph_lib)

texturize_lib = library('texturize-lib', [
                         'src/compter.c',
                         'src/initialiser.c',
                         'src/interface.c',
                         'src/offset.c',
                         'src/render.c'],
                        dependencies: [gimp_dep, gtk_dep, graph_dep])

texturize_dep = declare_dependency(link_with: texturize_lib)

executable('texturize', ['extra_config.h', 'src/main.c'],
           dependencies: [gimp_dep, gtk_dep, texturize_dep])
