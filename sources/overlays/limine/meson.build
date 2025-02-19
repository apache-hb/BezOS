project('limine', 'c',
    version : '8.6.1',
    license : 'BSD-2-Clause',
    license_files : 'LICENSE',
    meson_version : '>=1.1.0',
)

limine_dep = declare_dependency(include_directories : '.')

meson.override_dependency('limine', limine_dep)

if not meson.is_subproject()
    executable('limine-install', 'limine.c', install : true)

    data = [
        'BOOTX64.EFI',
        'BOOTIA32.EFI',
        'limine-bios-cd.bin',
        'limine-bios-pxe.bin',
        'limine-bios.sys',
        'limine-uefi-cd.bin'
    ]
    install_data(data, install_dir : get_option('prefix') / 'share', install_tag : 'bin')
endif
