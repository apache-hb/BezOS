project('tlsf', 'c')

src = files('tlsf.c')
inc = include_directories('.')

tlsf_lib = static_library('tlsf', src, include_directories: inc)
tlsf_dep = declare_dependency(link_with: tlsf_lib, include_directories: inc)

meson.override_dependency('tlsf', tlsf_dep)
