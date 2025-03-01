project('tlsf', 'c')

sources = files('tlsf.c')
includes = include_directories('.')

libtlsf = static_library('tlsf', sources, include_directories: includes)
tlsf_dep = declare_dependency(link_with: libtlsf, include_directories: includes)

meson.override_dependency('tlsf', tlsf_dep)
