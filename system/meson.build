project('system', 'cpp',
    default_options : [
        'cpp_rtti=false',
        'cpp_eh=none',
    ]
)

src = [
    'init/init.cpp',
]

shared = subproject('shared')

args = [
    '-e', 'ClientStart',
    '-z', 'noexecstack',
]

executable('init.elf', src,
    link_args : args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : shared.get_variable('sysapi'),
)
