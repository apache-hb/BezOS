project('ultra', 'c')

inc = include_directories('.')

ultra_dep = declare_dependency(
    include_directories : inc,
)

meson.override_dependency('ultra', ultra_dep)
