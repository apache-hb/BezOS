project('system', 'cpp',
    default_options : [
        'cpp_std=c++20',
        'cpp_rtti=false',
        'cpp_eh=none',
    ]
)

flanterm = dependency('flanterm')
tlsf = dependency('tlsf')

args = [
    '-e', 'ClientStart',
    '-z', 'noexecstack',
]

src = [ 'init/init.cpp', ]

executable('init.elf', src,
    link_args : args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : [ flanterm, tlsf ],
)

src = [ 'shell/shell.cpp', ]

executable('shell.elf', src,
    link_args : args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : [ flanterm, tlsf ],
)
