project('sysapi', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++20',
        'cpp_rtti=false',
        'cpp_eh=none',
    ]
)

src = [
    'src/bezos/syscall_fs.c',
    'src/bezos/syscall_threads.c',
    'src/bezos/syscall_device.c',
    'src/bezos/syscall_clock.c',
    'src/bezos/syscall_vmem.c',
    'src/bezos/syscall_process.c',
    'src/bezos/syscall_handle.c',
    'src/bezos/syscall_debug.c',
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
    install : not meson.is_subproject(),
    build_by_default : not meson.is_subproject(),
    install_dir : get_option('libdir'),
)

libbezos = static_library('bezos-sysapi', src,
    include_directories : inc,
    install : not meson.is_subproject(),
    build_by_default : not meson.is_subproject(),
)

bezos_dep = declare_dependency(
    link_with : libbezos,
    include_directories : inc,
)

src = [
    'src/posix/assert.cpp',
    'src/posix/locale.cpp',
    'src/posix/dirent.cpp',
    'src/posix/fcntl.cpp',
    'src/posix/stdio.cpp',
    'src/posix/errno.cpp',
    'src/posix/math.cpp',
    'src/posix/string.cpp',
    'src/posix/setjmp.cpp',
    'src/posix/strings.cpp',
    'src/posix/signal.cpp',
    'src/posix/ctype.cpp',
    'src/posix/unistd.cpp',
    'src/posix/termios.cpp',
    'src/posix/stdlib.cpp',
    'src/posix/time.cpp',
    'src/posix/stropts.cpp',
    'src/posix/sys_mman.cpp',
    'src/posix/sys_stat.cpp',
    'src/posix/sys_time.cpp',
    'src/posix/sys_times.cpp',
    'src/posix/sys_utsname.cpp',
    'src/posix/sys_wait.cpp',
    'src/posix/sys_resource.cpp',
]

libposix = static_library('bezos-posix', src,
    include_directories : inc,
    dependencies : [ bezos_dep ],
    install : not meson.is_subproject(),
    build_by_default : not meson.is_subproject(),
)

meson.override_dependency('bezos', bezos_dep)
meson.override_dependency('sysapi', sysapi_dep)

install_subdir('include/posix', install_dir : 'include')
install_subdir('include/bezos', install_dir : 'include')
