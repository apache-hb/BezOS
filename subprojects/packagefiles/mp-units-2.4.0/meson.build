project('mp-units', 'cpp')

inc = include_directories(
    'src/core/include',
    'src/systems/include',
)

dep = declare_dependency(
    include_directories : inc
)

meson.override_dependency('mp-units', dep)
