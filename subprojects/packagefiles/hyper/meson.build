project('hyper', 'c')

inc = include_directories('.')

hyper_dep = declare_dependency(
    include_directories : inc,
)

meson.override_dependency('hyper', hyper_dep)
