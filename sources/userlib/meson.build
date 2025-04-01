project('userlib', 'cpp')

sysapi = dependency('bezos-sysapi')

inc = include_directories('include')

install_subdir('include/userapi', install_dir : 'include', install_tag : 'headers')

userapi_dep = declare_dependency(
    include_directories : inc,
)
