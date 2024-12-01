project('clang', [ 'c', 'cpp' ],
    version : '19.1.4',
    license : 'Apache-2.0',
    meson_version : '>=1.1.0'
)

clang_intrin_headers = declare_dependency(
    include_directories : include_directories('.', is_system : true)
)

meson.override_dependency('clang-intrin-headers', clang_intrin_headers)
