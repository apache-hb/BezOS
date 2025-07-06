src = [
    'test_image.cpp',
    '../src/boot/limine.cpp',
]

subprocess = dependency('cpp-subprocess', native : true)
limine_install = find_program('limine-install', native : true)
argparse = dependency('argparse', native : true)

libtest_image = static_library('test_image', src,
    include_directories : inc,
    c_args : kernel_c_args,
    cpp_args : kernel_cpp_args,
    dependencies : [ deps, limine ],
    link_with : [ kernel ],
    override_options : kernel_kwargs['override_options']
)

ktest_ld = meson.project_source_root() / 'data/test/linker/linker-test.ld'

ktest_start_image = executable('start.elf', 'start.cpp',
    include_directories : inc,
    c_args : kernel_c_args,
    cpp_args : kernel_cpp_args,
    link_with : [ libtest_image, kernel ],
    dependencies : [ limine, deps ],
    override_options : kernel_kwargs['override_options'],
    link_depends : ktest_ld,
    link_args : [ '-T', ktest_ld ],
)

libvirt = dependency('libvirt', native : true)

ktest_runner = executable('ktest_runner', 'runner.cpp',
    dependencies : [ libvirt, argparse, subprocess ],
    native : true,
)

test('kernel start', ktest_runner,
    args : [
        '--kernel', ktest_start_image.full_path(),
        '--limine-install', limine_install.full_path(),
    ],
    env : {
        'MESON_PREFIX': get_option('prefix'),
    },
    depends : [ ktest_start_image, limine_install ],
    suite : 'virt'
)
