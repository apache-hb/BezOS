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

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

concurrentqueue = dependency('concurrentqueue')
tlsf = dependency('tlsf')
mp_units = dependency('mp-units')
absl = subproject('abseil-cpp')
sysapi = subproject('sysapi').get_variable('sysapi_dep')

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

inc = [ inc, absl.get_variable('absl_include_dir') ]
libs = [ absl.get_variable('absl_container_lib') ]
deps = [ mp_units, sysapi, concurrentqueue, tlsf ]

# Kernel

src = [
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

    # Architecture features
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

    # Device drivers
    'src/drivers/block/driver.cpp',
    'src/drivers/block/virtio_blk.cpp',
    'src/drivers/block/ramblk.cpp',

    'src/drivers/fs/driver.cpp',

    # Userspace
    'src/user/user.cpp',

    # Services
    'src/notify.cpp',

    # Process
    'src/process/process.cpp',
    'src/launch.cpp',

    # Kernel components
    'src/log.cpp',
    'src/memory.cpp',
]

kernel = static_library('kernel', src,
    include_directories : inc,
    c_args : c_args + [ '-mllvm', '-asm-macro-max-nesting-depth=256', '-D__BEZOS__=1' ],
    cpp_args : cpp_args + [ '-D__BEZOS__=1' ],
    dependencies : deps,
    link_with : libs,
    override_options : [ 'b_coverage=false' ],
)

kernel_dep = declare_dependency(
    include_directories : inc,
    link_with : kernel,
    dependencies : deps,
)
