project('bezos', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++26',
        'c_std=c23',
        'warning_level=3',
        'b_pie=false',
        'b_staticpic=false',
        'b_ndebug=true',
        'cpp_eh=none',
        'cpp_rtti=false'
    ],
    version : '0.0.1',
    license : 'GPL-3.0-only',
    meson_version : '>=1.1.0',
)

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

limine = dependency('limine')
ultra = dependency('ultra')
common = subproject('kernel-common').get_variable('kernel_dep')

inc = include_directories('include')

common_args = [
    '-fmacro-prefix-map=../=/',

    '-Wno-gnu-anonymous-struct',
    '-Wno-gnu-case-range',
    '-Wno-gcc-compat',
    '-Wno-gnu-designator',
    '-Wno-gnu-conditional-omitted-operand',

    '-Wno-missing-field-initializers',
    '-Wno-missing-designated-field-initializers',
    '-Wno-zero-length-array',
    '-Wno-nested-anon-types',
    '-Wno-nullability-extension',

    '-fno-omit-frame-pointer',
]

c_args = cc.get_supported_arguments([ '-Wno-language-extension-token' ] + common_args)
cpp_args = cpp.get_supported_arguments([ '-Wno-c99-designator', '-Wno-c99-extensions' ] + common_args)

inc = [ inc ]
deps = [ common ]

compiler = find_program(cc.get_id())
objcopy = find_program('objcopy')

# Smp startup blob

smp_blob = custom_target('smp',
    input : 'src/smp.S',
    output : 'smp',
    depend_files : 'data/smp.ld',
    command : [
        compiler, '@INPUT@', '-fPIC',
        '-T', meson.current_source_dir() / 'data/smp.ld',
        '-I', meson.current_source_dir() / 'include',
        '-o', '@OUTPUT@',
        '-nostdlib', '-nostdlib++', '-ffreestanding',
    ],
)

smp = custom_target('smp.o',
    input : smp_blob,
    output : 'smp.o',
    command : [
        objcopy, '-I', 'binary', '-O', 'elf64-x86-64',
        '-B', 'i386:x86-64',
        '--set-section-flags', '.data.smp=alloc,load,readonly,data',
        '--rename-section', '.data=.rodata.smp',
        '@INPUT@', '@OUTPUT@'
    ],
)

# Kernel

src = [
    # C and C++ Runtime
    'src/crt_freestanding.cpp',
    'src/cxxrt.cpp',

    # Smp setup
    'src/smp.cpp',
    smp,

    # Architecture features
    'src/isr.cpp',
    'src/isr.S',

    'src/syscall.cpp',
    'src/syscall.S',

    'src/thread.cpp',

    'src/process/schedule.cpp',
    'src/process/schedule.S',
    'src/processor.cpp',

    'src/main.cpp',
]

kernel = static_library('kernel', src,
    include_directories : inc,
    c_args : c_args + [ '-mllvm', '-asm-macro-max-nesting-depth=256', '-D__BEZOS__=1' ],
    cpp_args : cpp_args + [ '-D__BEZOS__=1' ],
    dependencies : deps,
    override_options : [ 'b_coverage=false' ],
)

###
### Limine version of the kernel image
###

executable('bezos-limine.elf', 'src/boot/limine.cpp',
    c_args : c_args + [ '-D__BEZOS__=1' ],
    cpp_args : cpp_args + [ '-D__BEZOS__=1' ],
    include_directories : inc,
    link_with : kernel,
    dependencies : [ limine, deps ],
    link_depends : 'data/linker-limine.ld',
    link_args : [ '-T', meson.current_source_dir() / 'data/linker-limine.ld' ],
    install : true,
    override_options : [ 'b_coverage=false' ],
)

###
### Hyper version of the kernel image
###

executable('bezos-hyper.elf', 'src/boot/hyper.cpp',
    c_args : c_args + [ '-D__BEZOS__=1' ],
    cpp_args : cpp_args + [ '-D__BEZOS__=1' ],
    include_directories : inc,
    link_with : kernel,
    dependencies : [ ultra, deps ],
    link_depends : 'data/linker-hyper.ld',
    link_args : [ '-T', meson.current_source_dir() / 'data/linker-hyper.ld' ],
    install : true,
    override_options : [ 'b_coverage=false' ],
)
