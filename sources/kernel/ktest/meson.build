src = [
    'src/ktest.cpp',
    'src/ktest_main.cpp',
    '../src/boot/limine.cpp',
]

subprocess = dependency('cpp-subprocess', native : true)
limine_install = find_program('limine-install', native : true)
argparse = dependency('argparse', native : true)

ktest_inc = [ inc, include_directories('include') ]
ktest_ld = meson.project_source_root() / 'data/test/linker/linker-test.ld'

libktest = static_library('ktest', src,
    include_directories : ktest_inc,
    c_args : kernel_c_args,
    cpp_args : kernel_cpp_args,
    dependencies : [ deps, limine ],
    link_with : [ kernel ],
    override_options : kernel_kwargs['override_options']
)

ktest_runner = executable('ktest-runner', 'runner.cpp',
    dependencies : [ argparse, subprocess ],
    native : true,
)

testcases = {
    'fail': {
        'sources': files('test/fail.cpp'),
        'should_fail': true,
    },
    'simple': {
        'sources': files('test/simple.cpp'),
    },
}

foreach name, setup : testcases
    id = name.replace(' ', '-')
    kwargs = setup + {
        'c_args': setup.get('c_args', []) + kernel_c_args,
        'cpp_args': setup.get('cpp_args', []) + kernel_cpp_args,
        'include_directories': setup.get('include_directories', []) + ktest_inc,
        'link_with': setup.get('link_with', []) + [ libktest, kernel ],
        'dependencies': setup.get('dependencies', []) + [ deps, limine ],
        'override_options': kernel_kwargs['override_options'],
        'link_depends': setup.get('link_depends', []) + [ ktest_ld ],
        'link_args': setup.get('link_args', []) + [ '-T', ktest_ld ],
    }

    # Filter out 'should_fail' from kwargs passed to executable()
    # TODO: surely meson has a way to remove keys from a map?
    exe_kwargs = {}
    foreach k, v : kwargs
        if k != 'should_fail'
            exe_kwargs += { k: v }
        endif
    endforeach

    image = executable('virt-test-image-' + id + '.elf',
        kwargs : exe_kwargs,
    )

    test('virt test ' + name, ktest_runner,
        args : [
            '--kernel', image.full_path(),
            '--limine-install', limine_install.full_path(),
        ],
        env : {
            'MESON_PREFIX': get_option('prefix'),
        },
        depends : [ image, ktest_runner, limine_install ],
        suite : 'virt',
        should_fail : setup.get('should_fail', false),
    )
endforeach
