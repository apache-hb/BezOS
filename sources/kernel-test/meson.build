project('bezos', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++26',
        'c_std=c23',
        'warning_level=3',
        'b_pie=false',
        'b_staticpic=false',
        'b_ndebug=true',
    ],
    version : '0.0.1',
    license : 'GPL-3.0-only',
    meson_version : '>=1.1.0',
)

enable_aml = false

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

common = subproject('kernel-common').get_variable('kernel_dep')

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

deps = [ common ]

# Unit tests

gtest = dependency('gtest')
gmock = dependency('gmock')
libvirt = dependency('libvirt')
stb_image_write = dependency('stb_image_write')
gtest_main = dependency('gtest_main')
testkwargs = { 'protocol' : 'gtest', 'timeout': 5 }

# meson bug: native executables dont have all default_options applied
test_c_args = c_args + [ '-DUTIL_TESTING=1' ]

# use NDEBUG_SANITIZER to disable some abseil debug tools,
# it causes issues with the freestanding patches ive made
test_cpp_args = cpp_args + [ '-DUTIL_TESTING=1', '-DNDEBUG_SANITIZER=1' ]

src = [
    'test/test_shim.cpp',
    'test/ports.cpp',
]

test_shim = static_library('test_shim', src,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, deps ],
)

test_exe_kwargs = {
    'c_args': test_c_args,
    'cpp_args': test_cpp_args,
    'dependencies': [ gtest_main, gmock, deps ],
    'override_options': [ 'b_sanitize=address' ],
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
    ],
    'bits': [
        'test/bits.cpp',
    ],
    'string': [ 'test/std/string.cpp', ],
    'spinlock': [ 'test/std/spinlock.cpp', ],
    'static string': [ 'test/std/static_string.cpp', ],
    'string view': [ 'test/std/string_view.cpp', ],
    'memory range': [ 'test/memory_range.cpp' ],
    'pat': [ 'test/page_attribute_table.cpp', ],
    'paging': [ 'test/paging.cpp', ],
    'arch cr': [ 'test/arch/cr.cpp', ],
    'arch gdt': [ 'test/arch/gdt.cpp', ],
    'framebuffer': [ 'test/display.cpp', ],
    'crt': [ 'test/crt.cpp', ],
    'virtual allocator': [
        'test/memory/virtual_allocator.cpp',
    ],
    'serial': [ 'test/serial.cpp', ],
    'range allocator': [
        'test/memory/range_allocator.cpp',
    ],
    'page table': [
        'test/memory/pte.cpp',
    ],
    'partitions': [
        'test/drivers/partitions.cpp',
    ],
    'ramblk': [
        'test/drivers/ramblk.cpp',
    ],
    'media': [
        'test/drivers/media.cpp',
    ],
    'fs2': [
        'test/fs/fs.cpp',
    ],
    'vfs path': [
        'test/fs/path.cpp',
    ],
    'vfs root': [
        'test/fs/root.cpp',
    ],
    'ramfs': [
        'test/fs/ramfs.cpp',
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
    ],
    'rcuptr': [
        'test/std/rcuptr.cpp',
    ],
    'cmos': [
        'test/cmos.cpp',
    ],
    'notify': [
        'test/notify.cpp',
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
        'subprojects/tlsf/tlsf.c',
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
