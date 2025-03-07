project('sysapi', [ 'c', 'cpp' ])

src = [
    'src/bezos/syscall_fs.c',
    'src/bezos/syscall_threads.c',
    'src/bezos/syscall_device.c',
    'src/bezos/syscall_clock.c',
    'src/bezos/syscall_vmem.c',
    'src/bezos/syscall_process.c',
    'src/bezos/syscall.S',
]

inc = include_directories('include')

sysapi_dep = declare_dependency(
    include_directories : inc,
)

inc = include_directories('include', 'include/posix')

cc = meson.get_compiler('c')

custom_target('crtstart',
    input : 'src/crt/start.cpp',
    output : 'crtstart.o',
    command : cc.cmd_array() + [ '-c', '-o', '@OUTPUT@', '@INPUT@', '-I' + meson.current_source_dir() / 'include' ],
    install : true,
    build_by_default : true,
    install_dir : get_option('libdir'),
)

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
    'src/posix/unistd.cpp',
    'src/posix/termios.cpp',
    'src/posix/stdlib.cpp',
    'src/posix/sys_mman.cpp',
]

libposix = static_library('bezos-posix', src,
    include_directories : inc,
    dependencies : [ bezos_dep ],
    install : true,
)

meson.override_dependency('bezos', bezos_dep)
meson.override_dependency('sysapi', sysapi_dep)

install_subdir('include/posix', install_dir : 'include')
install_subdir('include/bezos', install_dir : 'include')
