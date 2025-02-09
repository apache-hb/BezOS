project('limine', 'c',
    version : '8.4.1',
    license : 'BSD-2-Clause',
    license_files : 'LICENSE',
    meson_version : '>=1.1.0',
)

limine_dep = declare_dependency(include_directories : '.')

meson.override_dependency('limine', limine_dep)

install_data('BOOTX64.EFI', 'BOOTIA32.EFI',
    install_dir : get_option('prefix') / 'limine' / 'image' / 'EFI' / 'BOOT'
)

install_data('limine-bios.sys', 'limine-bios-cd.bin', 'limine-uefi-cd.bin',
    install_dir : get_option('prefix') / 'limine' / 'image' / 'boot' / 'limine'
)

install_data('limine-bios-pxe.bin', 'limine-bios.sys', 'BOOTX64.EFI', 'BOOTIA32.EFI',
    install_dir : get_option('prefix') / 'pxe'
)

limine_tool = executable('limine', 'limine.c', native : true)
