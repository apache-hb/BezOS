
capstone = dependency('capstone', native : true)
rpmalloc = dependency('rpmalloc', native : true)

pvtest_deps = [ capstone, sysapi, common, rpmalloc ]

pvtest_src = files(
    'src/pvtest.cpp',
    'src/machine.cpp',
    'src/cpu.cpp',
    'src/memory.cpp',
    'src/idt.cpp',
    'src/int.S',
)

pvtest_inc = include_directories('include')

pvtest_args = [
    '-Wno-missing-designated-field-initializers',
    '-Wno-c99-designator',
    '-Wno-c99-extensions',
    '-Wno-gnu-designator',
    '-Wno-gcc-compat',
    '-Wno-nullability-extension',
]

libpvtest = static_library('pvtest', pvtest_src,
    c_args : [ '-Wno-missing-designated-field-initializers' ],
    cpp_args : pvtest_args,
    include_directories : [ pvtest_inc, inc ],
    dependencies : pvtest_deps,
    native : true,
    pic : true,
)

pvtest = declare_dependency(
    include_directories : [ pvtest_inc, inc ],
    link_with : [ libpvtest ],
    dependencies : pvtest_deps,
)

subdir('test')
