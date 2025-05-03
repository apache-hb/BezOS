project('rpmalloc', 'c')

src = files('rpmalloc/rpmalloc.c')
inc = include_directories('rpmalloc')

librpmalloc = static_library('rpmalloc', src,
    c_args : [ '-D_GNU_SOURCE=1', '-DENABLE_STATISTICS=1', '-DENABLE_ASSERTS=1', '-DENABLE_OVERRIDE=0', '-DENABLE_DECOMMIT=0', '-DENABLE_UNMAP=0' ],
    include_directories : inc,
    native : true,
)

rpmalloc_dep = declare_dependency(
    include_directories : inc,
    link_with : librpmalloc,
)

meson.override_dependency('rpmalloc', rpmalloc_dep, native : true)
