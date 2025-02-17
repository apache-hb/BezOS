project('system', 'cpp',
    default_options : [
        'cpp_rtti=false',
        'cpp_eh=none',
    ]
)

shared = subproject('shared')
sysapi = shared.get_variable('sysapi')

args = [
    '-e', 'ClientStart',
    '-z', 'noexecstack',
]

src = [
    'init/init.cpp',
]


executable('init.elf', src,
    link_args : args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : sysapi,
)

src = [
    'shell/shell.cpp',
]

executable('shell.elf', src,
    link_args : args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : sysapi,
)
