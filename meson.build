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

enable_aml = false

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

inc = include_directories(
    'kernel/include',
    'kernel/subprojects/tlsf/include',
    'kernel/subprojects/concurrentqueue',
)

common_args = [
    '-fmacro-prefix-map=../=/',
    '-Wno-gnu-anonymous-struct',
    '-Wno-missing-field-initializers',
    '-Wno-missing-designated-field-initializers',
    '-Wno-zero-length-array',
    '-Wno-nested-anon-types',
    '-Wno-gnu-case-range',
    '-fno-omit-frame-pointer',
    '-Wno-nullability-extension',
    '-Wno-gcc-compat',
]

c_args = cc.get_supported_arguments([ '-Wno-language-extension-token' ] + common_args)
cpp_args = cpp.get_supported_arguments([ '-Wno-c99-designator', '-Wno-c99-extensions' ] + common_args)

limine = dependency('limine')
ultra = dependency('ultra')
mp_units = dependency('mp-units')
absl = subproject('abseil-cpp')

inc = [ inc, absl.get_variable('absl_include_dir') ]
deps = [ mp_units ]

compiler = find_program(cc.get_id())
objcopy = find_program('objcopy')

# Init program

init_blob = custom_target('init',
    input : [ 'system/init/init.S', 'system/init/init.cpp' ],
    output : 'init.elf',
    command : [
        compiler, '@INPUT@',
        '-ffreestanding', '-nostdlib', '-nostdlib++',
        '-fPIE', '-fno-pic', '-static', '-fno-rtti', '-fno-exceptions',
        '-z', 'noexecstack',
        '-e', 'ClientStart',
        '-o', '@OUTPUT@',
    ]
)

init = custom_target('init.o',
    input : init_blob,
    output : 'init.o',
    command : [
        objcopy, '-I', 'binary', '-O', 'elf64-x86-64',
        '--set-section-flags', '.data.init=alloc,load,readonly,data',
        '--rename-section', '.data=.rodata.init',
        '@INPUT@', '@OUTPUT@'
    ],
)

# Smp startup blob

smp_blob = custom_target('smp',
    input : 'kernel/src/smp.S',
    output : 'smp',
    depend_files : 'data/smp.ld',
    command : [
        compiler, '@INPUT@', '-fPIC',
        '-T', meson.current_source_dir() / 'data/smp.ld',
        '-I', meson.current_source_dir() / 'kernel/include',
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
    # TLSF allocator
    'kernel/subprojects/tlsf/tlsf.c',

    # Utilities
    'kernel/src/util/cpuid.cpp',
    'kernel/src/util/memory.cpp',
    'kernel/src/util/format.cpp',
    'kernel/src/util/bits.cpp',
    'kernel/src/util/uuid.cpp',

    # Memory management
    'kernel/src/memory/layout.cpp',
    'kernel/src/memory/allocator.cpp',
    'kernel/src/memory/page_allocator.cpp',
    'kernel/src/memory/virtual_allocator.cpp',
    'kernel/src/memory/page_tables.cpp',

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
    'kernel/src/delay.cpp',
    'kernel/src/cmos.cpp',
    'kernel/src/pit.cpp',
    'kernel/src/hpet.cpp',

    'kernel/src/thread.cpp',
    'kernel/src/user.S',
    'kernel/src/schedule.cpp',

    # PCI
    'kernel/src/pci/pci.cpp',
    'kernel/src/pci/config.cpp',

    # ACPI
    'kernel/src/acpi/acpi.cpp',
    'kernel/src/acpi/format.cpp',

    # HID
    'kernel/src/hid/ps2.cpp',

    # Terminal
    'kernel/src/canvas.cpp',
    'kernel/src/terminal/terminal.cpp',
    'kernel/src/terminal/buffered.cpp',

    # Wrappers
    'kernel/src/arch/cr.cpp',
    'kernel/src/boot.cpp',

    # VFS
    'kernel/src/fs/path.cpp',
    'kernel/src/fs/vfs.cpp',
    'kernel/src/fs/ramfs.cpp',
    'kernel/src/fs/tarfs.cpp',

    # VFS rewrite
    'kernel/src/fs2/vfs.cpp',
    'kernel/src/fs2/path.cpp',
    'kernel/src/fs2/node.cpp',
    'kernel/src/fs2/handle.cpp',

    # Smp setup
    'kernel/src/smp.cpp',
    smp,

    # Device drivers
    'kernel/src/drivers/block/driver.cpp',
    'kernel/src/drivers/block/virtio_blk.cpp',
    'kernel/src/drivers/block/ramblk.cpp',

    # FS drviers
    'kernel/src/drivers/fs/driver.cpp',

    # Process
    'kernel/src/syscall.cpp',
    'kernel/src/processor.cpp',
    'kernel/src/launch.cpp',

    # Embedded init
    init,

    # Kernel components
    'kernel/src/log.cpp',
    'kernel/src/memory.cpp',
    'kernel/src/main.cpp',
]

if enable_aml
    src += [ 'kernel/src/acpi/aml.cpp', ]
endif

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

executable('bezos-limine', 'kernel/src/boot/limine.cpp',
    c_args : c_args + [ '-D__BEZOS__=1' ],
    cpp_args : cpp_args + [ '-D__BEZOS__=1' ],
    include_directories : inc,
    link_with : kernel,
    dependencies : [ limine, deps ],
    link_depends : 'data/linker-limine.ld',
    link_args : [ '-T', meson.current_source_dir() / 'data/linker-limine.ld' ],
    install_dir : get_option('prefix') / 'limine' / 'image' / 'boot',
    install : true,
    override_options : [ 'b_coverage=false' ],
)

install_data('data/limine.conf',
    install_dir : get_option('prefix') / 'limine' / 'image' / 'boot' / 'limine',
)

install_data('data/limine.conf',
    install_dir : get_option('prefix') / 'pxe',
)

###
### Hyper version of the kernel image
###

executable('bezos-hyper', 'kernel/src/boot/hyper.cpp',
    c_args : c_args + [ '-D__BEZOS__=1' ],
    cpp_args : cpp_args + [ '-D__BEZOS__=1' ],
    include_directories : inc,
    link_with : kernel,
    dependencies : [ ultra, deps ],
    link_depends : 'data/linker-hyper.ld',
    link_args : [ '-T', meson.current_source_dir() / 'data/linker-hyper.ld' ],
    install_dir : get_option('prefix') / 'hyper' / 'image' / 'boot',
    install : true,
    override_options : [ 'b_coverage=false' ],
)

install_data('data/hyper/hyper-iso-boot',
    install_dir : get_option('prefix') / 'hyper' / 'image' / 'boot',
)

install_data('data/hyper/BOOTX64.EFI',
    install_dir : get_option('prefix') / 'hyper' / 'image' / 'EFI' / 'BOOT',
)

install_data('data/hyper.cfg',
    install_dir : get_option('prefix') / 'hyper' / 'image' / 'boot',
)

meson.add_install_script('data/image.sh')

# Unit tests

gtest = dependency('gtest', native : true)
gmock = dependency('gmock', native : true)
stb_image_write = dependency('stb_image_write')
gtest_main = dependency('gtest_main', native : true)
testkwargs = { 'protocol' : 'gtest', 'timeout': 5 }

# meson bug: native executables dont have all default_options applied
test_c_args = c_args + [ '-std=gnu23', '-DUTIL_TESTING=1' ]
test_cpp_args = cpp_args + [ '-std=c++26', '-DUTIL_TESTING=1' ]

src = [
    'kernel/test/test_shim.cpp',
    'kernel/test/ports.cpp',
    'kernel/src/delay.cpp',
]

test_shim = static_library('test_shim', src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest ],
    native : true,
)

test_exe_kwargs = {
    'include_directories': inc,
    'c_args': test_c_args,
    'cpp_args': test_cpp_args,
    'dependencies': [ gtest_main, gmock ],
    'override_options': [ 'b_sanitize=address' ],
    'native': true,
    'link_with': test_shim,
}

# Test cases

testcases = {
    'static vector': [
        'kernel/test/static_vector.cpp',
    ],
    'fixed deque': [
        'kernel/test/fixed_deque.cpp',
    ],
    'vector': [
        'kernel/test/vector.cpp',
    ],
    'format': [
        'kernel/test/format.cpp',
        'kernel/src/util/format.cpp',
    ],
    'bits': [
        'kernel/test/bits.cpp',
        'kernel/src/util/bits.cpp',
    ],
    'string': [ 'kernel/test/std/string.cpp', ],
    'static string': [ 'kernel/test/std/static_string.cpp', ],
    'string view': [ 'kernel/test/std/string_view.cpp', ],
    'memory range': [ 'kernel/test/memory_range.cpp' ],
    'pat': [ 'kernel/test/page_attribute_table.cpp', 'kernel/src/pat.cpp', 'kernel/src/util/cpuid.cpp', ],
    'paging': [ 'kernel/test/paging.cpp', ],
    'arch cr': [ 'kernel/test/arch/cr.cpp', 'kernel/src/arch/cr.cpp', ],
    'arch gdt': [ 'kernel/test/arch/gdt.cpp', 'kernel/src/gdt.cpp', ],
    'framebuffer': [ 'kernel/test/display.cpp', 'kernel/src/canvas.cpp', ],
    'crt': [ 'kernel/test/crt.cpp', 'kernel/src/crt.cpp', ],
    'virtual allocator': [
        'kernel/test/memory/virtual_allocator.cpp',
        'kernel/src/memory/virtual_allocator.cpp',
        'kernel/src/memory/page_allocator.cpp',
        'kernel/src/memory/layout.cpp',
        'kernel/src/util/bits.cpp',
        'kernel/src/boot.cpp',
    ],
    'serial': [ 'kernel/test/serial.cpp', 'kernel/src/uart.cpp', ],
    'range allocator': [
        'kernel/test/memory/range_allocator.cpp',
        'kernel/subprojects/tlsf/tlsf.c',
    ],
    'page table': [
        'kernel/test/memory/pte.cpp',
        'kernel/src/memory/page_tables.cpp',
        'kernel/subprojects/tlsf/tlsf.c',
    ],
    'partitions': [
        'kernel/test/drivers/partitions.cpp',
        'kernel/src/drivers/fs/driver.cpp',
        'kernel/src/drivers/block/driver.cpp',
        'kernel/src/drivers/block/ramblk.cpp',
    ],
    'ramblk': [
        'kernel/test/drivers/ramblk.cpp',
        'kernel/src/drivers/block/driver.cpp',
        'kernel/src/drivers/block/ramblk.cpp',
    ],
    'media': [
        'kernel/test/drivers/media.cpp',
        'kernel/src/drivers/block/driver.cpp',
        'kernel/src/drivers/block/ramblk.cpp',
    ],
    'vfs': [
        'kernel/test/vfs.cpp',
        'kernel/src/fs/vfs.cpp',
        'kernel/src/fs/path.cpp',
        'kernel/src/fs/ramfs.cpp',
    ],
    'fs2': [
        'kernel/test/fs/fs.cpp',
        'kernel/src/fs2/vfs.cpp',
        'kernel/src/fs2/path.cpp',
        'kernel/src/fs2/node.cpp',
        'kernel/src/fs2/handle.cpp',
    ],
    'vfs path': [
        'kernel/test/fs/path.cpp',
        'kernel/src/fs2/path.cpp',
    ],
    'shared ptr': [
        'kernel/test/shared.cpp',
    ]
}

if enable_aml
    testcases += {
        'aml details': [
            'kernel/test/acpi/detail.cpp',
            'kernel/src/acpi/format.cpp',
            'kernel/src/acpi/aml.cpp',
            'kernel/src/util/format.cpp',
        ],
        'aml names': [
            'kernel/test/acpi/name.cpp',
            'kernel/src/acpi/format.cpp',
            'kernel/src/acpi/aml.cpp',
            'kernel/src/util/format.cpp',
        ],
        'aml nodes': [
            'kernel/test/acpi/nodes.cpp',
            'kernel/src/acpi/format.cpp',
            'kernel/src/acpi/aml.cpp',
            'kernel/src/util/format.cpp',
        ]
    }
endif

foreach name, source : testcases
    exe = executable('test-kernel-' + name.to_lower().replace(' ', '-'), source,
        kwargs : test_exe_kwargs,
    )

    test(name, exe,
        suite : 'kernel',
        kwargs : testkwargs
    )
endforeach

testcases = {
    'terminal': {
        'sources': [
            'kernel/test/terminal.cpp',
            'kernel/src/canvas.cpp',
            'kernel/src/terminal/terminal.cpp',
            'kernel/src/terminal/buffered.cpp',
        ],
        'dependencies' : [ stb_image_write ]
    }
}

foreach name, kwargs : testcases
    new_kwargs = test_exe_kwargs
    new_kwargs += { 'dependencies': new_kwargs['dependencies'] + kwargs['dependencies'] }
    new_kwargs += { 'sources': kwargs['sources'] }

    exe = executable('test-kernel-' + name.to_lower().replace(' ', '-'),
        kwargs : new_kwargs,
    )

    test(name, exe,
        suite : 'kernel',
        kwargs : testkwargs
    )
endforeach

# AML tests

if enable_aml
    testcases = [
        'B450M-MORTAR',
        'H13SSL-N',
        'G513RM',
        'QEMU',
    ]

    acpixtract = find_program('data/test/acpi/dump.sh')

    src = [
        'kernel/test/acpi/aml.cpp',
        'kernel/src/acpi/aml.cpp',
        'kernel/src/acpi/format.cpp',
        'kernel/src/util/format.cpp',
        'kernel/src/allocator/bitmap.cpp',
        'kernel/subprojects/tlsf/tlsf.c',
    ]

    exe = executable('test-aml', src,
        kwargs : test_exe_kwargs,
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
endif
