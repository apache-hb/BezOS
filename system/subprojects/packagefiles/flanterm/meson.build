project('flanterm', 'c')

src = [
    'flanterm.c',
    'backends/fb.c',
]

inc = include_directories('.')

flanterm_lib = static_library('flanterm', src,
    include_directories : inc,
)

flanterm_dep = declare_dependency(
    link_with : flanterm_lib,
    include_directories : inc,
)

meson.override_dependency('flanterm', flanterm_dep)
