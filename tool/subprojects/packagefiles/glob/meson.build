project('glob', 'cpp')

src = [ 'source/glob.cpp' ]
inc = include_directories('include')

libglob = library('glob', src, include_directories: inc)
glob_dep = declare_dependency(link_with: libglob, include_directories: inc)

meson.override_dependency('glob', glob_dep)
