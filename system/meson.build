project('system', 'cpp',
    default_options : [
        'cpp_rtti=false',
        'cpp_eh=none',
    ]
)

flanterm = dependency('flanterm')
bezos = subproject('shared').get_variable('bezos_dep')

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
    dependencies : [ bezos, flanterm ],
)

src = [
    'shell/shell.cpp',
]

executable('shell.elf', src,
    link_args : args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : bezos,
)
