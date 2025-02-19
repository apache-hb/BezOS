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

enable_aml = false

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

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

limine = dependency('limine')
ultra = dependency('ultra')

mp_units = dependency('mp-units')
absl = subproject('abseil-cpp')
concurrentqueue = dependency('concurrentqueue')
tlsf = subproject('tlsf')
sysapi = subproject('shared').get_variable('sysapi_dep')

inc = [ inc, absl.get_variable('absl_include_dir'), tlsf.get_variable('includes') ]
libs = [ absl.get_variable('absl_container_lib') ]
deps = [ mp_units, sysapi, concurrentqueue ]

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
    tlsf.get_variable('sources'),

    # Utilities
    'src/util/cpuid.cpp',
    'src/util/memory.cpp',
    'src/util/format.cpp',
    'src/util/bits.cpp',
    'src/util/uuid.cpp',

    # RCU
    'src/std/rcu.cpp',
    'src/std/rcuptr.cpp',

    # Memory management
    'src/memory/layout.cpp',
    'src/memory/allocator.cpp',
    'src/memory/page_allocator.cpp',
    'src/memory/virtual_allocator.cpp',
    'src/memory/page_tables.cpp',

    # C and C++ Runtime
    'src/crt.cpp',
    'src/cxxrt.cpp',
    'src/crt_freestanding.cpp',

    # Architecture features
    'src/isr.cpp',
    'src/isr.S',
    'src/gdt.cpp',
    'src/pat.cpp',
    'src/check.cpp',
    'src/apic.cpp',
    'src/uart.cpp',
    'src/hypervisor.cpp',
    'src/smbios.cpp',
    'src/delay.cpp',
    'src/cmos.cpp',
    'src/pit.cpp',
    'src/hpet.cpp',

    'src/thread.cpp',
    'src/user.S',

    # PCI
    'src/pci/pci.cpp',
    'src/pci/config.cpp',

    # ACPI
    'src/acpi/acpi.cpp',
    'src/acpi/format.cpp',

    # HID
    'src/hid/hid.cpp',
    'src/hid/ps2.cpp',

    # Terminal
    'src/canvas.cpp',
    'src/terminal/terminal.cpp',
    'src/terminal/buffered.cpp',

    # Wrappers
    'src/arch/cr.cpp',
    'src/boot.cpp',

    # VFS
    'src/fs2/vfs.cpp',
    'src/fs2/path.cpp',
    'src/fs2/node.cpp',
    'src/fs2/handle.cpp',

    # VFS drivers
    'src/fs2/ramfs.cpp',
    'src/fs2/tarfs.cpp',

    # Smp setup
    'src/smp.cpp',
    smp,

    # Device drivers
    'src/drivers/block/driver.cpp',
    'src/drivers/block/virtio_blk.cpp',
    'src/drivers/block/ramblk.cpp',

    # Userspace
    'src/user/user.cpp',

    # Services
    'src/notify.cpp',

    # Process
    'src/process/process.cpp',
    'src/process/schedule.cpp',
    'src/process/schedule.S',
    'src/syscall.cpp',
    'src/processor.cpp',
    'src/launch.cpp',

    # Kernel components
    'src/log.cpp',
    'src/memory.cpp',
    'src/main.cpp',
]

if enable_aml
    src += [ 'src/acpi/aml.cpp', ]
endif

kernel = static_library('kernel', src,
    include_directories : inc,
    c_args : c_args + [ '-mllvm', '-asm-macro-max-nesting-depth=256', '-D__BEZOS__=1' ],
    cpp_args : cpp_args + [ '-D__BEZOS__=1' ],
    dependencies : deps,
    link_with : libs,
    override_options : [ 'b_coverage=false' ],
)

###
### sqlite
###

# sqlite = subproject('sqlite')
# sqlite_args = [
#     '-DSQLITE_OMIT_FLOATING_POINT=1',
#     '-DSQLITE_OMIT_DEPRECATED=1',
#     '-DSQLITE_OMIT_LOAD_EXTENSION=1',
#     '-DSQLITE_OS_OTHER=1',
#     '-DSQLITE_OMIT_JSON=1',
#     '-DSQLITE_OMIT_EXPLAIN=1',
#     '-DSQLITE_OMIT_DATETIME_FUNCS=1',
#     '-DSQLITE_ZERO_MALLOC=1',
#     '-Wno-keyword-macro',
#     '-Wno-unused-parameter',
#     '-Wno-literal-conversion',
# ]
# sqlite_kernel = library('sqlite', sqlite.get_variable('src'),
#     include_directories : [ inc, sqlite.get_variable('inc') ],
#     c_args : c_args + sqlite_args,
#     cpp_args : cpp_args + sqlite_args,
#     override_options : [ 'b_coverage=false' ],
# )

###
### Limine version of the kernel image
###

executable('bezos-limine', 'src/boot/limine.cpp',
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

executable('bezos-hyper', 'src/boot/hyper.cpp',
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

subdir_done()

gtest = dependency('gtest', native : true)
gmock = dependency('gmock', native : true)
libvirt = dependency('libvirt', native : true)
stb_image_write = dependency('stb_image_write')
gtest_main = dependency('gtest_main', native : true)
testkwargs = { 'protocol' : 'gtest', 'timeout': 5 }

# meson bug: native executables dont have all default_options applied
test_c_args = c_args + [ '-std=gnu23', '-DUTIL_TESTING=1' ]

# use NDEBUG_SANITIZER to disable some abseil debug tools,
# it causes issues with the freestanding patches ive made
test_cpp_args = cpp_args + [ '-std=c++26', '-DUTIL_TESTING=1', '-DNDEBUG_SANITIZER=1' ]

src = [
    'test/test_shim.cpp',
    'test/ports.cpp',
    'src/delay.cpp',
]

test_shim = static_library('test_shim', src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest ],
    native : true,
)

test_exe_kwargs = {
    'include_directories': [ inc ],
    'c_args': test_c_args,
    'cpp_args': test_cpp_args,
    'dependencies': [ gtest_main, gmock ],
    'override_options': [ 'b_sanitize=address' ],
    'native': true,
    'link_with': [ test_shim ],
}

# Test cases

testcases = {
    'static vector': [
        'test/static_vector.cpp',
    ],
    'fixed deque': [
        'test/fixed_deque.cpp',
    ],
    'vector': [
        'test/vector.cpp',
    ],
    'format': [
        'test/format.cpp',
        'src/util/format.cpp',
    ],
    'bits': [
        'test/bits.cpp',
        'src/util/bits.cpp',
    ],
    'string': [ 'test/std/string.cpp', ],
    'spinlock': [ 'test/std/spinlock.cpp', ],
    'static string': [ 'test/std/static_string.cpp', ],
    'string view': [ 'test/std/string_view.cpp', ],
    'memory range': [ 'test/memory_range.cpp' ],
    'pat': [ 'test/page_attribute_table.cpp', 'src/pat.cpp', 'src/util/cpuid.cpp', ],
    'paging': [ 'test/paging.cpp', ],
    'arch cr': [ 'test/arch/cr.cpp', 'src/arch/cr.cpp', ],
    'arch gdt': [ 'test/arch/gdt.cpp', 'src/gdt.cpp', ],
    'framebuffer': [ 'test/display.cpp', 'src/canvas.cpp', ],
    'crt': [ 'test/crt.cpp', 'src/crt.cpp', ],
    'virtual allocator': [
        'test/memory/virtual_allocator.cpp',
        'src/memory/virtual_allocator.cpp',
        'src/memory/page_allocator.cpp',
        'src/memory/layout.cpp',
        'src/util/bits.cpp',
        'src/boot.cpp',
    ],
    'serial': [ 'test/serial.cpp', 'src/uart.cpp', ],
    'range allocator': [
        'test/memory/range_allocator.cpp',
    ],
    'page table': [
        'test/memory/pte.cpp',
        'src/memory/page_tables.cpp',
    ],
    'partitions': [
        'test/drivers/partitions.cpp',
        'src/drivers/fs/driver.cpp',
        'src/drivers/block/driver.cpp',
        'src/drivers/block/ramblk.cpp',
    ],
    'ramblk': [
        'test/drivers/ramblk.cpp',
        'src/drivers/block/driver.cpp',
        'src/drivers/block/ramblk.cpp',
    ],
    'media': [
        'test/drivers/media.cpp',
        'src/drivers/block/driver.cpp',
        'src/drivers/block/ramblk.cpp',
    ],
    'fs2': [
        'test/fs/fs.cpp',
        'src/fs2/vfs.cpp',
        'src/fs2/path.cpp',
        'src/fs2/node.cpp',
        'src/fs2/handle.cpp',
        'src/fs2/ramfs.cpp',
    ],
    'vfs path': [
        'test/fs/path.cpp',
        'src/fs2/path.cpp',
    ],
    'vfs root': [
        'test/fs/root.cpp',
        'src/fs2/vfs.cpp',
        'src/fs2/path.cpp',
        'src/fs2/node.cpp',
        'src/fs2/handle.cpp',
        'src/fs2/ramfs.cpp',
    ],
    'ramfs': [
        'test/fs/ramfs.cpp',
        'src/fs2/vfs.cpp',
        'src/fs2/path.cpp',
        'src/fs2/node.cpp',
        'src/fs2/handle.cpp',
        'src/fs2/ramfs.cpp',
        'test/fs/fs_test.cpp',
    ],
    'shared ptr': [
        'test/shared.cpp',
    ],
    'atomic forward list': [
        'test/std/forward_list.cpp',
    ],
    'rcu': [
        'test/std/rcu.cpp',
        'src/std/rcu.cpp',
    ],
    'rcuptr': [
        'test/std/rcuptr.cpp',
        'src/std/rcu.cpp',
        'src/std/rcuptr.cpp',
    ],
    'cmos': [
        'test/cmos.cpp',
        'src/cmos.cpp',
    ],
    'notify': [
        'test/notify.cpp',
        'src/notify.cpp',
        'src/std/rcu.cpp',
        'src/std/rcuptr.cpp',
    ],
}

if enable_aml
    testcases += {
        'aml details': [
            'test/acpi/detail.cpp',
            'src/acpi/format.cpp',
            'src/acpi/aml.cpp',
            'src/util/format.cpp',
        ],
        'aml names': [
            'test/acpi/name.cpp',
            'src/acpi/format.cpp',
            'src/acpi/aml.cpp',
            'src/util/format.cpp',
        ],
        'aml nodes': [
            'test/acpi/nodes.cpp',
            'src/acpi/format.cpp',
            'src/acpi/aml.cpp',
            'src/util/format.cpp',
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

tar_test_script = find_program('data/test/tar/tar.sh')
tar_test_data = custom_target('tar-test-data',
    output : 'test.tar',
    command : [ tar_test_script, meson.current_source_dir() / 'data/test/tar', '@OUTPUT@' ],
)

source = [
    'test/fs/tarfs.cpp',
    'src/fs2/vfs.cpp',
    'src/fs2/path.cpp',
    'src/fs2/node.cpp',
    'src/fs2/handle.cpp',
    'src/fs2/ramfs.cpp',
    'src/fs2/tarfs.cpp',
    'src/drivers/block/driver.cpp',
    'src/drivers/block/ramblk.cpp',
    'test/fs/fs_test.cpp',
]

exe = executable('test-kernel-tarfs', source,
    kwargs : test_exe_kwargs,
)

test('tarfs', exe,
    suite : 'kernel',
    kwargs : testkwargs,
    depends : tar_test_data,
    env : {
        'TAR_TEST_ARCHIVE': tar_test_data.full_path(),
    }
)

# source = [
#     'test/launch/elf.cpp',

#     'src/launch.cpp',

#     'src/fs2/vfs.cpp',
#     'src/fs2/path.cpp',
#     'src/fs2/node.cpp',
#     'src/fs2/handle.cpp',
#     'src/fs2/ramfs.cpp',
#     'src/fs2/tarfs.cpp',

#     'src/boot.cpp',
#     'src/hypervisor.cpp',

#     'src/memory.cpp',
#     'src/memory/layout.cpp',
#     'src/memory/page_allocator.cpp',
#     'src/memory/virtual_allocator.cpp',
#     'src/memory/page_tables.cpp',

#     'src/util/format.cpp',
#     'src/util/memory.cpp',
#     'src/util/cpuid.cpp',

#     'subprojects/tlsf/tlsf.c',

#     'src/drivers/block/driver.cpp',
#     'src/drivers/block/ramblk.cpp',
# ]

# exe = executable('test-kernel-elf', source,
#     kwargs : test_exe_kwargs,
# )

# test('elf', exe,
#     suite : 'kernel',
#     kwargs : testkwargs,
#     depends : tar_test_data,
#     env : {
#         'TAR_TEST_ARCHIVE': meson.current_source_dir() / 'data/test/elf/initrd.tar'
#     }
# )

testcases = {
    'terminal': {
        'sources': [
            'test/terminal.cpp',
            'src/canvas.cpp',
            'src/terminal/terminal.cpp',
            'src/terminal/buffered.cpp',
        ],
        'dependencies' : [ stb_image_write ]
    },
    'launch': {
        'sources': [
            'test/emulated/launch.cpp',
        ],
        'dependencies' : [ libvirt ],
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
        kwargs : testkwargs,
        env : {
            'LSAN_OPTIONS': 'suppressions=' + meson.current_source_dir() / 'data/test/sanitizer/glib.supp',
            'MESON_SOURCE_ROOT': meson.current_source_dir(),
            'MESON_PREFIX': get_option('prefix'),
        }
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
        'test/acpi/aml.cpp',
        'src/acpi/aml.cpp',
        'src/acpi/format.cpp',
        'src/util/format.cpp',
        'src/allocator/bitmap.cpp',
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
