project('sysapi', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++20',
        'c_std=gnu23',
        'cpp_rtti=false',
        'cpp_eh=none',
        'b_pie=true',
    ],
    version : '0.1.0',
)

inc = include_directories('include')

sysapi_dep = declare_dependency(
    include_directories : inc,
)

add_project_arguments(
    '-ffile-prefix-map=' + meson.current_source_dir() + '=/',
    '-ffile-prefix-map=../../../=/',
    language : [ 'c', 'cpp' ]
)

add_project_arguments(
    '-Wno-c99-extensions',
    '-Wno-missing-designated-field-initializers',
    language : 'cpp'
)

install_subdir('include/posix', install_dir : 'include', install_tag : 'headers')
install_subdir('include/bezos', install_dir : 'include', install_tag : 'headers')
install_subdir('include/rtld', install_dir : 'include', install_tag : 'headers')

if get_option('headers') or meson.is_subproject()
    subdir_done()
endif

pkgconfig = import('pkgconfig')

common = subproject('common').get_variable('common_dep')

cc = meson.get_compiler('c')
sys_root = meson.get_external_property('sys_root')

src = [
    'src/bezos/syscall_clock.c',
    'src/bezos/syscall_debug.c',
    'src/bezos/syscall_device.c',
    'src/bezos/syscall_handle.c',
    'src/bezos/syscall_mutex.c',
    'src/bezos/syscall_node.c',
    'src/bezos/syscall_process.c',
    'src/bezos/syscall_thread.c',
    'src/bezos/syscall_vmem.c',
    'src/bezos/syscall.S',
]

inc = include_directories(
    sys_root / 'llvm-libcxx/include/c++/v1',
    sys_root / 'llvm/lib/clang/21/include',
    'include/posix',
    'include'
)

libbezos = static_library('bezos-sysapi', src,
    include_directories : inc,
    install : true,
)

bezos_dep = declare_dependency(
    link_with : libbezos,
    include_directories : inc,
)

meson.override_dependency('bezos', bezos_dep)

stb = dependency('stb')

src = [
    'src/rtld/rtld.cpp',
]

librtld = static_library('rtld', src,
    name_prefix : '',
    include_directories : [ inc ],
    dependencies : [ bezos_dep, common, stb ],
    install : true,
)

rtld_dep = declare_dependency(
    link_with : librtld,
    include_directories : [ inc ],
)

meson.override_dependency('rtld', rtld_dep)

custom_target('crtstart',
    input : 'src/crt/start.cpp',
    output : 'crtstart.o',
    command : cc.cmd_array() + [ '-c', '-o', '@OUTPUT@', '@INPUT@', '-I' + meson.current_source_dir() / 'include', '--sysroot', sys_root ],
    install : true,
    install_dir : get_option('libdir'),
)

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

c_args = [
    # rpmalloc defines
    '-DENABLE_STATISTICS=1',
    '-DENABLE_ASSERTS=1',
    '-DENABLE_OVERRIDE=0',
]

libposix = static_library('bezos-posix', src,
    include_directories : [ inc, rpmalloc.get_variable('includes') ],
    dependencies : [ bezos_dep, stb ],
    c_args : c_args,
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

doxygen = find_program('doxygen', required : get_option('docs'))
if doxygen.found()
    source_dir = meson.project_source_root()

    doxy_cdata = configuration_data()
    doxy_cdata.set('VERSION', meson.project_version())
    doxy_cdata.set('SOURCES', source_dir)
    doxy_cdata.set('ROOT_SOURCE_DIR', source_dir)
    doxy_cdata.set('INCLUDE_PATHS', source_dir / 'include')
    doxy_cdata.set('CLANG_DATABASE', meson.global_build_root() / 'compile_commands.json')

    doxy_cfg = configure_file(
        input : 'data/Doxyfile.in',
        output : 'Doxyfile',
        configuration : doxy_cdata
    )

    custom_target('docs',
        input : doxy_cfg,
        output : 'html',
        command: [ doxygen, doxy_cfg ],
        install : true,
        install_dir : get_option('datadir') / 'doc',
    )
endif
