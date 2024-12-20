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
    ],
    version : '0.0.1',
    license : 'GPL-3.0-only',
    license_files : 'LICENSE',
    meson_version : '>=1.1.0',
)

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

inc = include_directories('kernel/include')

common_args = [ '-fmacro-prefix-map=../=/', '-Wno-gnu-anonymous-struct', '-Wno-missing-field-initializers', '-Wno-missing-designated-field-initializers' ]

c_args = cc.get_supported_arguments([ '-Wno-language-extension-token' ] + common_args)
cpp_args = cpp.get_supported_arguments([ '-Wno-c99-designator' ] + common_args)

src = [
    'kernel/src/util/cpuid.cpp',
    'kernel/src/util/memory.cpp',
    'kernel/src/memory/physical.cpp',
    'kernel/src/memory/alloc.cpp',
]

limine = dependency('limine')

libkernel = static_library('kernel', src,
    include_directories : inc,
    c_args : c_args,
    cpp_args : cpp_args,
    dependencies : [ limine ],
)

kernel = declare_dependency(
    include_directories : inc,
    link_with : libkernel,
    dependencies : [ limine ],
)

if meson.is_cross_build()
    src = [
        'kernel/src/crt.c',
        'kernel/src/main.cpp',
        'kernel/src/check.cpp',
    ]

    executable('bezos', src,
        c_args : c_args,
        cpp_args : cpp_args,
        include_directories : 'kernel/include',
        dependencies : [ kernel ],
        link_depends : 'data/linker-x64.ld',
        install : true,
        link_args : [ '-T', meson.current_source_dir() / 'data/linker-x64.ld' ],
        link_with : libkernel,
        install_dir : get_option('prefix') / 'image' / 'boot'
    )

    install_data('data/limine.conf',
        install_dir : get_option('prefix') / 'image' / 'boot' / 'limine'
    )

    meson.add_install_script('data/image.sh')
else
    gtest = dependency('gtest_main')
    testkwargs = { 'protocol' : 'gtest', 'timeout': 5 }

    testcases = {
        'format': 'kernel/test/format.cpp',
        'memmap': [ 'kernel/test/memmap.cpp' ],
    }

    foreach name, source : testcases
        exe = executable('test-kernel-' + name.to_lower().replace(' ', '-'), source,
            include_directories : inc,
            c_args : c_args,
            cpp_args : cpp_args,
            link_with : libkernel,
            dependencies : [ gtest, kernel ],
        )

        test(name, exe,
            suite : 'kernel',
            kwargs : testkwargs
        )
    endforeach
endif
