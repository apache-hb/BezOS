project('sysapi', [ 'c', 'cpp' ])

src = [
    'src/bezos/syscall_fs.c',
    'src/bezos/syscall_threads.c',
    'src/bezos/syscall_device.c',
    'src/bezos/syscall_clock.c',
    'src/bezos/syscall_vmem.c',
    'src/bezos/syscall.S',
]

inc = include_directories('include')

sysapi_dep = declare_dependency(
    include_directories : inc,
)

inc = include_directories('include', 'include/posix')

libbezos = static_library('bezos-sysapi', src,
    include_directories : inc,
    install : true,
)

bezos_dep = declare_dependency(
    link_with : libbezos,
    include_directories : inc,
)

src = [
    'src/posix/fcntl.cpp',
    'src/posix/stdio.cpp',
    'src/posix/errno.cpp',
    'src/posix/string.cpp',
    'src/posix/ctype.cpp',
]

libposix = static_library('bezos-posix', src,
    include_directories : inc,
    install : true,
)

meson.override_dependency('bezos', bezos_dep)
meson.override_dependency('sysapi', sysapi_dep)

install_subdir('include/posix', install_dir : 'include')
install_subdir('include/bezos', install_dir : 'include')
