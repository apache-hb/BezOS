project('system', 'cpp',
    default_options : [
        'cpp_std=c++20',
        'cpp_rtti=false',
        'cpp_eh=none',
    ]
)

flanterm = dependency('flanterm')
tlsf = dependency('tlsf')

link_args = [
    '-T', meson.current_source_dir() / 'data/bezos.ld',
    '-e', 'ClientStart',
    '-z', 'noexecstack',
]

cpp_args = [
    '-Wno-gnu-variable-sized-type-not-at-end',
]

src = [ 'init/init.cpp', ]

executable('init.elf', src,
    cpp_args : cpp_args,
    link_args : link_args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : [ flanterm, tlsf ],
)

src = [ 'shell/shell.cpp', ]

executable('shell.elf', src,
    cpp_args : cpp_args,
    link_args : link_args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : [ flanterm, tlsf ],
)

src = [ 'tty/tty.cpp', ]

executable('tty.elf', src,
    cpp_args : cpp_args,
    link_args : link_args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : [ flanterm, tlsf ],
)
