project('posix-headers', 'c')

inc = include_directories('include')

posix_headers_dep = declare_dependency(
    include_directories: inc,
)

meson.override_dependency('posix-headers', posix_headers_dep)

install_subdir('include', install_dir: get_option('includedir'))
