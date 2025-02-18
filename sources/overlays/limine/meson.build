project('limine', 'c',
    version : '8.6.1',
    license : 'BSD-2-Clause',
    license_files : 'LICENSE',
    meson_version : '>=1.1.0',
)

limine_dep = declare_dependency(include_directories : '.')

executable('limine-install', 'limine.c',
    native : true,
    install : true
)

meson.override_dependency('limine', limine_dep)

