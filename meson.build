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

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

src = [
    'src/crt.c',
    'src/kernel.cpp',
    'src/check.cpp',
]

common_args = [ '-fmacro-prefix-map=../=/' ]

c_args = cc.get_supported_arguments([ '-Wno-language-extension-token' ] + common_args)
cpp_args = cpp.get_supported_arguments([ '-Wno-c99-designator', '-Wno-missing-designated-field-initializers' ] + common_args)

executable('bezos', src,
    c_args : c_args,
    cpp_args : cpp_args,
    include_directories : 'include',
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
