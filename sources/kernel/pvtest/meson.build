
capstone = dependency('capstone', native : true)
rpmalloc = dependency('rpmalloc', native : true)

pvtest_deps = [ capstone, sysapi, common, rpmalloc ]

pvtest_src = files(
    'src/pvtest.cpp',
    'src/machine.cpp',
    'src/cpu.cpp',
    'src/memory.cpp',
    'src/int.S',
)

pvtest_inc = include_directories('include')

libpvtest = static_library('pvtest', pvtest_src,
    c_args : [ '-Wno-missing-designated-field-initializers' ],
    cpp_args : [ '-Wno-missing-designated-field-initializers', '-Wno-c99-designator' ],
    include_directories : [ pvtest_inc, inc ],
    dependencies : pvtest_deps,
    native : true,
    pic : true,
)

pvtest_dep = declare_dependency(
    include_directories : [ pvtest_inc, inc ],
    link_with : [ libpvtest ],
    dependencies : pvtest_deps,
)

pvtest_testcases = {
    'basic': {
        'sources': files('test/basic.cpp')
    },
    'asm': {
        'sources': files('test/asm.cpp'),
        'c_args': [ ],
        'cpp_args': [ ],
        'link_args' : [ ]
    },
}

foreach name, setup : pvtest_testcases
    exe = executable('pvtest-' + name.strip().replace(' ', '-'), setup['sources'],
        c_args : setup.get('c_args', [ '-fsanitize=nullability' ]),
        cpp_args : setup.get('cpp_args', [ '-fsanitize=nullability' ]),
        link_args : setup.get('link_args', [ '-fsanitize=nullability' ]),
        dependencies : setup.get('dependencies', []) + [ pvtest_dep, gtest_main ],
        native : true,
        pie : true,
    )

    test('pvtest ' + name.strip(), exe,
        suite : 'pvtest',
        protocol : 'gtest',
        timeout : 5,
    )
endforeach
