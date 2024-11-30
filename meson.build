project('bezos', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=gnu++26',
        'c_std=gnu23',
        'werror=true',
        'warning_level=3',
        'b_pie=false',
        'b_staticpic=false',
        'debug=false',
        'cpp_eh=none',
        'cpp_rtti=false'
    ]
)

executable('bezos', 'src/kernel.c',
    c_args : '-Wno-language-extension-token',
    dependencies : [ dependency('limine'), dependency('clang-intrin-headers') ],
    link_depends : 'data/linker-x64.ld',
    install : true,
    link_args : [ '-T', meson.current_source_dir() / 'data/linker-x64.ld' ],
    install_dir : get_option('prefix') / 'image' / 'boot'
)

install_data('data/limine.conf',
    install_dir : get_option('prefix') / 'image' / 'boot' / 'limine'
)

meson.add_install_script('data/image.sh')
