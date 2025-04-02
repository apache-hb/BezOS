project('runtime', 'cpp',
    default_options : [
        'cpp_std=c++23',
    ]
)

sysapi = dependency('bezos-sysapi')

inc = include_directories('include')

install_subdir('include/bezos-runtime', install_dir : 'include', install_tag : 'headers')

runtime_dep = declare_dependency(
    include_directories : inc,
)
