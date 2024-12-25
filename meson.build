project('bezos', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++26',
        'c_std=c23',
        'werror=true',
        'warning_level=3',
        'b_pie=false',
        'b_staticpic=false',
        'b_ndebug=true',
        'cpp_eh=none',
        'cpp_rtti=false'
    ],
    version : '0.0.1',
    license : 'GPL-3.0-only',
    license_files : 'LICENSE',
    meson_version : '>=1.1.0',
)

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

inc = include_directories('kernel/include')

common_args = [
    '-fmacro-prefix-map=../=/',
    '-Wno-gnu-anonymous-struct',
    '-Wno-missing-field-initializers',
    '-Wno-missing-designated-field-initializers',
    '-Wno-zero-length-array',
]

c_args = cc.get_supported_arguments([ '-Wno-language-extension-token' ] + common_args)
cpp_args = cpp.get_supported_arguments([ '-Wno-c99-designator', '-Wno-c99-extensions' ] + common_args)

limine = dependency('limine')

src = [
    'kernel/src/util/cpuid.cpp',
    'kernel/src/util/memory.cpp',
    'kernel/src/memory/layout.cpp',
    'kernel/src/memory/allocator.cpp',

    'kernel/src/crt.c',
    'kernel/src/cxxrt.cpp',
    'kernel/src/isr.cpp',
    'kernel/src/isr.S',
    'kernel/src/gdt.cpp',
    'kernel/src/main.cpp',
    'kernel/src/check.cpp',
    'kernel/src/apic.cpp',
    'kernel/src/uart.cpp',
    'kernel/src/acpi.cpp',
    'kernel/src/memory.cpp',
    'kernel/src/hypervisor.cpp',
]

executable('bezos', src,
    c_args : c_args + [ '-mllvm', '-asm-macro-max-nesting-depth=256' ],
    cpp_args : cpp_args,
    include_directories : inc,
    dependencies : [ limine ],
    link_depends : 'data/linker-x64.ld',
    link_args : [ '-T', meson.current_source_dir() / 'data/linker-x64.ld' ],
    install_dir : get_option('prefix') / 'image' / 'boot',
    install : true,
)

install_data('data/limine.conf',
    install_dir : get_option('prefix') / 'image' / 'boot' / 'limine'
)

meson.add_install_script('data/image.sh')

gtest = dependency('gtest_main', native : true)
testkwargs = { 'protocol' : 'gtest', 'timeout': 5 }

testcases = {
    'format': [ 'kernel/test/format.cpp', ],
    'memmap': [ 'kernel/test/memmap.cpp', 'kernel/src/memory/layout.cpp', 'kernel/src/util/memory.cpp', ],
    'string': [ 'kernel/test/string.cpp', ],
}

foreach name, source : testcases
    exe = executable('test-kernel-' + name.to_lower().replace(' ', '-'), source,
        include_directories : inc,
        c_args : c_args,
        cpp_args : cpp_args + [ '-std=c++26' ], # meson bug: native executables dont have default_options applied
        dependencies : [ gtest, limine ],
        native : true,
    )

    test(name, exe,
        suite : 'kernel',
        kwargs : testkwargs
    )
endforeach
