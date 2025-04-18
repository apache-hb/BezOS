project('sysapi', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++20',
        'c_std=gnu23',
        'cpp_rtti=false',
        'cpp_eh=none',
        'b_pie=true',
    ]
)

inc = include_directories('include')

sysapi_dep = declare_dependency(
    include_directories : inc,
)

add_project_arguments(
    '-Wno-c99-extensions',
    '-Wno-missing-designated-field-initializers',
    language : 'cpp'
)

if not meson.is_subproject()
    install_subdir('include/posix', install_dir : 'include', install_tag : 'headers')
    install_subdir('include/bezos', install_dir : 'include', install_tag : 'headers')
endif

if get_option('headers')
    subdir_done()
endif

common = subproject('common').get_variable('common_dep')

pkgconfig = import('pkgconfig')

cc = meson.get_compiler('c')

src = [
    'src/bezos/syscall_fs.c',
    'src/bezos/syscall_threads.c',
    'src/bezos/syscall_device.c',
    'src/bezos/syscall_clock.c',
    'src/bezos/syscall_vmem.c',
    'src/bezos/syscall_mutex.c',
    'src/bezos/syscall_process.c',
    'src/bezos/syscall_handle.c',
    'src/bezos/syscall_debug.c',
    'src/bezos/syscall.S',
]

inc = []
if not meson.is_subproject()
    sys_root = meson.get_external_property('sys_root')

    inc += include_directories(
        sys_root / 'llvm-libcxx/include/c++/v1',
        sys_root / 'llvm/lib/clang/21/include',
        'include/posix'
    )
endif

inc += include_directories('include')

libbezos = static_library('bezos-sysapi', src,
    include_directories : inc,
    install : true,
)

bezos_dep = declare_dependency(
    link_with : libbezos,
    include_directories : inc,
)

meson.override_dependency('bezos', bezos_dep)

src = [
    'src/rtld/rtld.cpp',
]

librtld = static_library('rtld', src,
    name_prefix : '',
    include_directories : [ inc ],
    dependencies : [ bezos_dep, common ],
    install : true,
)

rtld_dep = declare_dependency(
    link_with : librtld,
    include_directories : [ inc ],
)

meson.override_dependency('rtld', rtld_dep)

if meson.is_subproject()
    subdir_done()
endif

custom_target('crtstart',
    input : 'src/crt/start.cpp',
    output : 'crtstart.o',
    command : cc.cmd_array() + [ '-c', '-o', '@OUTPUT@', '@INPUT@', '-I' + meson.current_source_dir() / 'include', '--sysroot', sys_root ],
    install : true,
    install_dir : get_option('libdir'),
)

stb = dependency('stb')
rpmalloc = subproject('rpmalloc')

src = [
    rpmalloc.get_variable('sources'),
    'src/posix/private.cpp',

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
    'src/posix/wchar.cpp',
    'src/posix/time.cpp',
    'src/posix/dlfcn.cpp',
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
    include_directories : [ inc, rpmalloc.get_variable('includes') ],
    dependencies : [ bezos_dep, stb ],
    install : true,
)

posix_dep = declare_dependency(
    link_with : libposix,
    include_directories : [ inc ],
)

meson.override_dependency('sysapi', sysapi_dep)

pkgconfig.generate(libbezos,
    name : 'bezos-sysapi',
    description : 'BezOS native syscall API',
    version : meson.project_version(),
)

pkgconfig.generate(libposix,
    name : 'bezos-posix',
    description : 'BezOS POSIX API',
    version : meson.project_version(),
    requires : [ 'bezos-sysapi' ],
)
