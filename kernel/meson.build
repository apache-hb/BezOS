kernel_lib = static_library('kernel', 'kernel.c',
    include_directories : include_directories('.')
)

kernel = declare_dependency(
    link_with : kernel_lib,
    include_directories : include_directories('.')
)
