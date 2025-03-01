project('system', 'cpp',
    default_options : [
        'cpp_rtti=false',
        'cpp_eh=none',
    ]
)

flanterm = dependency('flanterm')
rpmalloc = dependency('rpmalloc')

args = [
    '-e', 'ClientStart',
    '-z', 'noexecstack',
]

src = [ 'init/init.cpp', ]

executable('init.elf', src,
    link_args : args,
    install : true,
    install_dir : get_option('prefix'),
    dependencies : [ flanterm, rpmalloc ],
)
