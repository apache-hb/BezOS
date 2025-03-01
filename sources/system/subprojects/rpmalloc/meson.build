project('rpmalloc', 'c')

src = [ 'rpmalloc/rpmalloc.c' ]

librpmalloc = static_library('rpmalloc', src)
rpmalloc_dep = declare_dependency(link_with: librpmalloc, include_directories: 'rpmalloc')

meson.override_dependency('rpmalloc', rpmalloc_dep)
