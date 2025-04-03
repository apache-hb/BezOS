project('runtime', 'cpp',
    default_options : [
        'cpp_std=c++23',
    ]
)

sysapi = dependency('bezos-sysapi')

inc = include_directories('include')

runtime_dep = declare_dependency(
    include_directories : inc,
)

if meson.is_subproject()
    subdir_done()
endif
