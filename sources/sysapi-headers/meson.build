project('sysapi-headers', 'c', version: '1.0.0', license: 'Apache-2.0')

inc = include_directories('include')

sysapi_headers_dep = declare_dependency(
    include_directories: inc,
)

meson.override_dependency('sysapi-headers', sysapi_headers_dep)

install_subdir('include', install_dir: get_option('includedir'))
