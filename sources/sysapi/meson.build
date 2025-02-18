project('sysapi', 'c')

src = [
    'src/syscall_fs.c',
    'src/syscall_threads.c',
    'src/syscall_device.c',
    'src/syscall_clock.c',
    'src/syscall_vmem.c',
    'src/syscall.S',
]

inc = include_directories('include')

sysapi_dep = declare_dependency(
    include_directories : inc,
)

libbezos = static_library('bezos-sysapi', src,
    include_directories : inc,
    build_by_default : false,
)

bezos_dep = declare_dependency(
    link_with : libbezos,
    include_directories : inc,
)

meson.override_dependency('sysapi', sysapi_dep)
meson.override_dependency('bezos-sysapi', bezos_dep)
