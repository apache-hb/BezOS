project('rpmalloc', 'c')

src = files('rpmalloc/rpmalloc.c')
inc = include_directories('rpmalloc')

librpmalloc = library('rpmalloc', src,
    c_args : [ '-D_GNU_SOURCE=1' ],
    include_directories : inc,
    native : true,
)

rpmalloc_dep = declare_dependency(
    include_directories : inc,
    link_with : librpmalloc,
)

meson.override_dependency('rpmalloc', rpmalloc_dep, native : true)
