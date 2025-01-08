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
    '-Wno-nested-anon-types'
]

c_args = cc.get_supported_arguments([ '-Wno-language-extension-token' ] + common_args)
cpp_args = cpp.get_supported_arguments([ '-Wno-c99-designator', '-Wno-c99-extensions' ] + common_args)

limine = dependency('limine')

# Smp startup blob

compiler = find_program(cc.get_id())
objcopy = find_program('objcopy')

smp_blob = custom_target('smp',
    input : 'kernel/src/smp.S',
    output : 'smp',
    depend_files : 'data/smp.ld',
    command : [
        compiler, '@INPUT@',
        '-T', meson.current_source_dir() / 'data/smp.ld',
        '-I', meson.current_source_dir() / 'kernel/include',
        '-o', '@OUTPUT@',
        '-nostdlib', '-nostdlib++', '-ffreestanding'
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
    # Utilities
    'kernel/src/util/cpuid.cpp',
    'kernel/src/util/memory.cpp',
    'kernel/src/util/format.cpp',

    # Memory management
    'kernel/src/memory/layout.cpp',
    'kernel/src/memory/allocator.cpp',
    'kernel/src/memory/bitmap.cpp',
    'kernel/src/memory/page_allocator.cpp',
    'kernel/src/memory/virtual_allocator.cpp',

    # C and C++ Runtime
    'kernel/src/crt.cpp',
    'kernel/src/cxxrt.cpp',
    'kernel/src/crt_freestanding.cpp',

    # Architecture features
    'kernel/src/isr.cpp',
    'kernel/src/isr.S',
    'kernel/src/gdt.cpp',
    'kernel/src/pat.cpp',
    'kernel/src/check.cpp',
    'kernel/src/apic.cpp',
    'kernel/src/uart.cpp',
    'kernel/src/hypervisor.cpp',
    'kernel/src/smbios.cpp',
    'kernel/src/pci.cpp',
    'kernel/src/delay.cpp',

    # ACPI
    'kernel/src/acpi/acpi.cpp',
    'kernel/src/acpi/aml.cpp',

    # HID
    'kernel/src/hid/ps2.cpp',

    # Terminal
    'kernel/src/canvas.cpp',
    'kernel/src/terminal/terminal.cpp',
    'kernel/src/terminal/buffered.cpp',

    # Wrappers
    'kernel/src/arch/cr.cpp',

    # Smp setup
    'kernel/src/smp.cpp',
    smp,

    # Kernel components
    'kernel/src/memory.cpp',
    'kernel/src/main.cpp',
]

kernel = static_library('kernel', src,
    include_directories : inc,
    c_args : c_args + [ '-mllvm', '-asm-macro-max-nesting-depth=256' ],
    cpp_args : cpp_args,
    override_options : [ 'b_coverage=false' ],
)

executable('bezos', 'kernel/src/boot/limine.cpp',
    c_args : c_args,
    cpp_args : cpp_args,
    include_directories : inc,
    link_with : kernel,
    dependencies : [ limine ],
    link_depends : 'data/linker-x64.ld',
    link_args : [ '-T', meson.current_source_dir() / 'data/linker-x64.ld' ],
    install_dir : get_option('prefix') / 'image' / 'boot',
    install : true,
    override_options : [ 'b_coverage=false' ],
)

install_data('data/limine-iso.conf',
    install_dir : get_option('prefix') / 'image' / 'boot' / 'limine',
    rename : 'limine.conf',
)

install_data('data/limine-pxe.conf',
    install_dir : get_option('prefix') / 'pxe',
    rename : 'limine.conf',
)

meson.add_install_script('data/image.sh')

# Unit tests

gtest = dependency('gtest', native : true)
gmock = dependency('gmock', native : true)
gtest_main = dependency('gtest_main', native : true)
testkwargs = { 'protocol' : 'gtest', 'timeout': 5 }

# meson bug: native executables dont have all default_options applied
test_cpp_args = cpp_args + [ '-std=c++26', '-DUTIL_TESTING=1' ]

src = [
    'kernel/test/test_shim.cpp',
    'kernel/test/ports.cpp',
    'kernel/src/delay.cpp',
]

test_shim = static_library('test_shim', src,
    include_directories : inc,
    c_args : c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest ],
    native : true,
)

# Test cases

testcases = {
    'static vector': [ 'kernel/test/vector.cpp', ],
    'format': [ 'kernel/test/format.cpp', ],
    'memmap': [ 'kernel/test/memmap.cpp', 'kernel/src/memory/layout.cpp', 'kernel/src/util/memory.cpp', ],
    'string': [ 'kernel/test/string.cpp', ],
    'memory range': [ 'kernel/test/memory_range.cpp' ],
    'pat': [ 'kernel/test/page_attribute_table.cpp', 'kernel/src/pat.cpp', 'kernel/src/util/cpuid.cpp', ],
    'paging': [ 'kernel/test/paging.cpp', ],
    'pmm': [ 'kernel/test/pmm.cpp', 'kernel/src/memory/bitmap.cpp', ],
    'arch cr': [ 'kernel/test/arch/cr.cpp', 'kernel/src/arch/cr.cpp', ],
    'arch gdt': [ 'kernel/test/arch/gdt.cpp', 'kernel/src/gdt.cpp', ],
    'framebuffer': [ 'kernel/test/display.cpp', 'kernel/src/canvas.cpp', ],
    'crt': [ 'kernel/test/crt.cpp', 'kernel/src/crt.cpp', ],
    'page allocator': [
        'kernel/test/page_allocator.cpp',
        'kernel/src/memory/page_allocator.cpp',
        'kernel/src/memory/bitmap.cpp',
        'kernel/src/memory/layout.cpp',
    ],
    'serial': [ 'kernel/test/serial.cpp', 'kernel/src/uart.cpp', ],
}

foreach name, source : testcases
    exe = executable('test-kernel-' + name.to_lower().replace(' ', '-'), source,
        include_directories : inc,
        c_args : c_args,
        cpp_args : test_cpp_args,
        dependencies : [ gtest_main, gmock ],
        override_options : [ 'b_sanitize=address' ],
        native : true,
        link_with : test_shim,
    )

    test(name, exe,
        suite : 'kernel',
        kwargs : testkwargs
    )
endforeach

# AML tests

testcases = [
    'B450M-MORTAR'
]

acpixtract = find_program('data/test/acpi/dump.sh')

src = [
    'kernel/test/acpi/aml.cpp',
    'kernel/src/acpi/aml.cpp',
]

exe = executable('test-aml', src,
    include_directories : inc,
    c_args : c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest_main, gmock ],
    override_options : [ 'b_sanitize=address' ],
    native : true,
    link_with : test_shim,
)

foreach name : testcases
    bin = custom_target(name + '-bin',
        input : 'data/test/acpi/' + name + '.txt',
        output : name + '.bin',
        command : [ acpixtract, '@INPUT@', '@OUTPUT@', '@PRIVATE_DIR@' ],
    )

    test(name, exe,
        suite : 'acpi',
        kwargs : testkwargs,
        depends : bin,
        env : { 'AML_BLOB': bin.full_path() }
    )
endforeach
