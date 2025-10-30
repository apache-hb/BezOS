project('sysapi-headers', 'c')

inc = include_directories('include')

sysapi_headers_dep = declare_dependency(
    include_directories: inc,
)

meson.override_dependency('sysapi-headers', sysapi_headers_dep)

install_subdir('include', install_dir: get_option('includedir'))
