
capstone = dependency('capstone', native : true)
rpmalloc = dependency('rpmalloc', native : true)

pvtest_deps = [ capstone, sysapi, common, rpmalloc ]

pvtest_src = files(
    'src/pvtest.cpp',
    'src/machine.cpp',
    'src/cpu.cpp',
    'src/memory.cpp',
)

pvtest_inc = include_directories('include')

libpvtest = library('pvtest', pvtest_src,
    c_args : [ '-Wno-missing-designated-field-initializers' ],
    cpp_args : [ '-Wno-missing-designated-field-initializers', '-Wno-c99-designator' ],
    include_directories : [ pvtest_inc, inc ],
    dependencies : pvtest_deps,
    native : true,
)

pvtest_dep = declare_dependency(
    include_directories : [ pvtest_inc, inc ],
    link_with : [ libpvtest ],
    dependencies : pvtest_deps,
)

pvtest_testcases = {
    'basic': {
        'sources': files('test/basic.cpp')
    }
}

foreach name, setup : pvtest_testcases
    exe = executable('pvtest-' + name.strip().replace(' ', '-'), setup['sources'],
        c_args : [ '-fsanitize=nullability' ],
        cpp_args : [ '-fsanitize=nullability' ],
        link_args : [ '-fsanitize=nullability' ],
        dependencies : setup.get('dependencies', []) + [ pvtest_dep, gtest_main ],
        native : true,
    )

    test('pvtest ' + name.strip(), exe,
        suite : 'pvtest',
        protocol : 'gtest',
        timeout : 5,
    )
endforeach
