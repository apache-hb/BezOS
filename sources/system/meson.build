project('system', 'cpp',
    default_options : [
        'cpp_std=c++20',
        'cpp_rtti=false',
        'cpp_eh=none',
    ]
)

add_project_arguments(
    '-ffile-prefix-map=' + meson.current_source_dir() + '=/',
    '-ffile-prefix-map=../../../=/',
    language : [ 'c', 'cpp' ]
)

cpp = meson.get_compiler('cpp')
flanterm = dependency('flanterm')
tlsf = dependency('tlsf')
sys_root = meson.get_external_property('sys_root')
rtld = cpp.find_library('rtld', dirs : sys_root / 'sysapi/lib', static : true)

link_args = [
    '-T', meson.current_source_dir() / 'data/bezos.ld',
    '-e', 'ClientStart',
    '-z', 'noexecstack',
]

cpp_args = [
    '-Wno-gnu-variable-sized-type-not-at-end',
]

libcommon = static_library('common', 'common/common.cpp',
    cpp_args : cpp_args,
)

src = [ 'shell/test.cpp', ]

executable('test.elf', src,
    cpp_args : cpp_args,
    link_args : link_args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : [ flanterm, tlsf ],
    link_depends : 'data/bezos.ld',
    link_with : [ libcommon ],
)

src = [ 'init/init.cpp', ]

executable('init.elf', src,
    cpp_args : cpp_args,
    link_args : link_args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : [ flanterm, tlsf, rtld ],
    link_depends : 'data/bezos.ld',
    link_with : [ libcommon ],
)

src = [ 'shell/shell.cpp', ]

executable('shell.elf', src,
    cpp_args : cpp_args,
    link_args : link_args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : [ flanterm, tlsf ],
    link_depends : 'data/bezos.ld',
    link_with : [ libcommon ],
)

src = [ 'tty/tty.cpp', ]

executable('tty.elf', src,
    cpp_args : cpp_args,
    link_args : link_args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : [ flanterm, tlsf ],
    link_depends : 'data/bezos.ld',
    link_with : [ libcommon ],
)
