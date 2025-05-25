project('relacy', 'cpp')

relacy_dep = declare_dependency(
    include_directories : include_directories('.', is_system : true)
)

meson.override_dependency('relacy', relacy_dep, native : true)
