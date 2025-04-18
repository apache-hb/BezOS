project('bezos', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++26',
        'c_std=c23',
        'warning_level=3',
        'b_pie=false',
        'b_staticpic=false',
        'b_ndebug=true',
        'cpp_eh=none',
        'cpp_rtti=false',
        'b_coverage=true',
        'abseil-cpp:b_coverage=false',
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
    '-ffile-prefix-map=' + meson.current_source_dir() + '=/',

    '-Wno-gnu-anonymous-struct',
    '-Wno-gnu-case-range',
    '-Wno-gcc-compat',
    '-Wno-gnu-designator',
    '-Wno-gnu-conditional-omitted-operand',
    '-Wno-gnu-include-next',

    '-Wno-missing-field-initializers',
    '-Wno-missing-designated-field-initializers',
    '-Wno-zero-length-array',
    '-Wno-nested-anon-types',
    '-Wno-nullability-extension',

    '-fno-omit-frame-pointer',
    '-Wthread-safety',
    '-Wfunction-effects',
    '-D_LIBCPP_VERBOSE_ABORT(...)=(__builtin_abort())',
]

c_args = cc.get_supported_arguments([ '-Wno-language-extension-token' ] + common_args)
cpp_args = cpp.get_supported_arguments([ '-Wno-c99-designator', '-Wno-c99-extensions' ] + common_args)

limine = dependency('limine')
ultra = dependency('ultra')

mp_units = dependency('mp-units')
absl = subproject('abseil-cpp')
concurrentqueue = dependency('concurrentqueue')
frozen = dependency('frozen')
tlsf = subproject('tlsf')
stb_sprintf = dependency('stb_sprintf')
sysapi = subproject('sysapi').get_variable('sysapi_dep')
common = subproject('common').get_variable('common_dep')

inc = [ inc, absl.get_variable('absl_include_dir'), tlsf.get_variable('includes') ]
libs = [ absl.get_variable('absl_container_lib') ]
deps = [ mp_units, sysapi, concurrentqueue, stb_sprintf, common ]

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

system_src = [
    'src/system/system.cpp',
    'src/system/schedule.cpp',
    'src/system/handle.cpp',
    'src/system/process.cpp',
    'src/system/thread.cpp',
    'src/system/mutex.cpp',
    'src/system/node.cpp',
    'src/system/device.cpp',
    'src/system/transaction.cpp',

    'src/system/vm/mapping.cpp',
]

src = [
    tlsf.get_variable('sources'),

    # Utilities
    'src/util/cpuid.cpp',
    'src/util/memory.cpp',
    'src/util/format.cpp',
    'src/util/uuid.cpp',

    # RCU
    'src/std/rcu.cpp',
    'src/std/rcuptr.cpp',

    # Memory management
    'src/memory/layout.cpp',
    'src/memory/allocator.cpp',
    'src/memory/page_allocator.cpp',
    'src/memory/table_allocator.cpp',
    'src/memory/pte.cpp',
    'src/memory/tables.cpp',
    'src/memory/memory.cpp',
    'src/memory/address_space.cpp',
    'src/memory/detail/table_list.cpp',
    'src/memory/detail/control_block.cpp',

    # C and C++ Runtime
    'src/crt.cpp',
    'src/cxxrt.cpp',
    'src/crt_freestanding.cpp',

    # Interrupt handling
    'src/isr/isr.cpp',
    'src/isr/isr.S',
    'src/isr/tables.cpp',
    'src/isr/default.cpp',

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
    'src/debug.cpp',
    'src/xsave.cpp',
    'src/clock.cpp',

    'src/thread.cpp',
    'src/user.S',

    # Timers and tick sources
    'src/timer/sleep.cpp',
    'src/timer/pit.cpp',
    'src/timer/hpet.cpp',
    'src/timer/apic_timer.cpp',
    'src/timer/tsc.cpp',

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
    'src/arch/dr.cpp',
    'src/boot.cpp',

    # VFS
    'src/fs2/vfs.cpp',
    'src/fs2/path.cpp',
    'src/fs2/node.cpp',
    'src/fs2/handle.cpp',
    'src/fs2/device.cpp',
    'src/fs2/folder.cpp',
    'src/fs2/utils.cpp',
    'src/fs2/identify.cpp',
    'src/fs2/query.cpp',
    'src/fs2/iterator.cpp',

    # VFS drivers
    'src/fs2/ramfs.cpp',
    'src/fs2/tarfs.cpp',

    # Devices
    'src/devices/ddi.cpp',
    'src/devices/hid.cpp',
    'src/devices/sysfs.cpp',
    'src/devices/stream.cpp',

    # Smp setup
    'src/smp.cpp',
    smp,

    # Device drivers
    'src/drivers/block/driver.cpp',
    'src/drivers/block/virtio_blk.cpp',
    'src/drivers/block/ramblk.cpp',

    # Userspace
    'src/user/user.cpp',
    'src/user/sysapi/clock.cpp',
    'src/user/sysapi/node.cpp',
    'src/user/sysapi/device.cpp',
    'src/user/sysapi/process.cpp',
    'src/user/sysapi/vmem.cpp',
    'src/user/sysapi/thread.cpp',
    'src/user/sysapi/mutex.cpp',

    # Services
    'src/notify.cpp',

    'src/system/runtime.cpp',
    system_src,

    # Process
    # 'src/process/system.cpp',
    # 'src/process/object.cpp',
    # 'src/process/process.cpp',
    # 'src/process/thread.cpp',
    # 'src/process/schedule.cpp',
    # 'src/process/schedule.S',

    'src/syscall.cpp',
    'src/processor.cpp',

    'src/elf/elf.cpp',
    'src/elf/launch.cpp',

    # Kernel components
    'src/log.cpp',
    'src/memory.cpp',
    'src/setup.cpp',
]

if host_machine.cpu_family() == 'x86_64'
    src += [
        'src/arch/x86_64/setjmp.S',
        'src/system/schedule.S',
    ]
elif host_machine.cpu_family() == 'sparc64'
    deps += [ frozen ]
    src += [
        'src/arch/sparcv9/machine.cpp'
    ]
endif

if enable_aml
    src += [ 'src/acpi/aml.cpp', ]
endif

kernel_c_args = c_args
kernel_cpp_args = cpp_args

ubsan = get_option('ubsan').disable_auto_if(not get_option('buildtype').contains('debug'))
if ubsan.allowed()
    src += files(
        'src/sanitizer/ubsan.cpp',
        # 'src/sanitizer/asan.cpp',
    )
    san_args = [
        '-fsanitize=undefined,nullability',
        '-fno-sanitize=function',
        # '-fsanitize=kernel-address',
        # '-mllvm', '-asan-globals=false',
        # '-mllvm', '-asan-mapping-offset=0xdfffe00000000000',
    ]
    kernel_c_args += san_args
    kernel_cpp_args += san_args
endif

kernel = static_library('kernel', src,
    include_directories : inc,
    c_args : kernel_c_args + [ '-mllvm', '-asm-macro-max-nesting-depth=256', '-D__BEZOS__=1' ],
    cpp_args : kernel_cpp_args + [ '-D__BEZOS__=1' ],
    dependencies : deps,
    link_with : libs,
    override_options : [ 'b_coverage=false' ],
)

kmain = static_library('kernel-main', 'src/main.cpp',
    include_directories : inc,
    c_args : c_args + [ '-D__BEZOS__=1' ],
    cpp_args : cpp_args + [ '-D__BEZOS__=1' ],
    dependencies : deps,
    link_with : libs + [ kernel ],
    override_options : [ 'b_coverage=false' ],
)

###
### Limine version of the kernel image
###

executable('bezos-limine.elf', 'src/boot/limine.cpp',
    c_args : c_args + [ '-D__BEZOS__=1' ],
    cpp_args : cpp_args + [ '-D__BEZOS__=1' ],
    include_directories : inc,
    link_with : kmain,
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
    link_with : kmain,
    dependencies : [ ultra, deps ],
    link_depends : 'data/linker-hyper.ld',
    link_args : [ '-T', meson.current_source_dir() / 'data/linker-hyper.ld' ],
    install : true,
    override_options : [ 'b_coverage=false' ],
)

# Unit tests

gtest = dependency('gtest', native : true)
gmock = dependency('gmock', native : true)
# libvirt = dependency('libvirt', native : true)
stb_image_write = dependency('stb_image_write')
capstone = dependency('capstone', native : true)
gtest_main = dependency('gtest_main', native : true)
testkwargs = { 'protocol' : 'gtest', 'timeout': 5 }

# meson bug: native executables dont have all default_options applied
test_c_args = c_args + [ '-std=gnu23', '-DUTIL_TESTING=1' ]

# use NDEBUG_SANITIZER to disable some abseil debug tools,
# it causes issues with the freestanding patches ive made
test_cpp_args = cpp_args + [ '-std=gnu++26', '-DUTIL_TESTING=1', '-DNDEBUG_SANITIZER=1' ]

test_shim_src = [
    'test/test_shim.cpp',
    'test/ports.cpp',
    'src/delay.cpp',
    'src/util/format.cpp',
]

fs_test_src = [
    'src/fs2/vfs.cpp',
    'src/fs2/path.cpp',
    'src/fs2/node.cpp',
    'src/fs2/handle.cpp',
    'src/fs2/ramfs.cpp',
    'src/fs2/device.cpp',
    'src/fs2/folder.cpp',
    'src/fs2/utils.cpp',
    'src/fs2/identify.cpp',
    'src/fs2/query.cpp',
    'src/fs2/iterator.cpp',

    'src/util/uuid.cpp',

    'src/std/rcu.cpp',
    'src/std/rcuptr.cpp',
]

memory_test_src = [
    'src/memory/pte.cpp',
    'src/memory/tables.cpp',
    'src/memory/memory.cpp',
    'src/memory/table_allocator.cpp',
    'src/memory/page_allocator.cpp',
    'src/memory/address_space.cpp',
    'src/memory/detail/table_list.cpp',
    'src/memory/detail/control_block.cpp',

    'src/util/memory.cpp',
]

system_test_src = [
    system_src,

    'src/devices/stream.cpp',

    'src/memory.cpp',
    'src/boot.cpp',

    'src/xsave.cpp',
    'src/util/cpuid.cpp',
]

new_shim_test_src = [
    'test/new_shim.cpp'
]

operator_new_shim_test = static_library('operator_new_shim_test', new_shim_test_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    native : true,
)

test_shim = static_library('test_shim', test_shim_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    native : true,
)

fs_test_shim = static_library('fs_test_shim', fs_test_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    override_options: [ 'b_sanitize=address,undefined', 'b_coverage=true' ],
    native : true,
)

system_test_lib = static_library('system_test', system_test_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    override_options: [ 'b_sanitize=address,undefined', 'b_coverage=true' ],
    native : true,
)

memory_test_shim = static_library('memory_test_shim', memory_test_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    override_options: [ 'b_sanitize=address,undefined', 'b_coverage=true' ],
    native : true,
)

test_exe_kwargs = {
    'include_directories': [ inc ],
    'c_args': test_c_args,
    'cpp_args': test_cpp_args,
    'dependencies': [ gtest_main, gmock, sysapi, concurrentqueue, mp_units, common ],
    'override_options': [ 'b_sanitize=address,undefined', 'b_coverage=true' ],
    'native': true,
    'link_with': [ test_shim, fs_test_shim, memory_test_shim, system_test_lib ],
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
    'serial': [ 'test/serial.cpp', 'src/uart.cpp', ],
    'range allocator': [
        'test/memory/range_allocator.cpp',
    ],
    'page table': [
        'test/memory/pte.cpp',
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
    ],
    'vfs path': [
        'test/fs/path.cpp',
    ],
    'vfs root': [
        'test/fs/root.cpp',
    ],
    'ramfs': [
        'test/fs/ramfs.cpp',
    ],
    'fs update': [
        'test/fs/update.cpp',
    ],
    'identify': [
        'test/fs/ramfs.cpp',
    ],
    'verify': [
        'test/user/verify.cpp',
        'src/user/context.cpp',
    ],
    'shared ptr': [
        'test/shared.cpp',
    ],
    'atomic forward list': [
        'test/std/forward_list.cpp',
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
        tlsf.get_variable('sources'),
    ],
    'isr table': [
        'test/isr/isr.cpp',
        'src/isr/default.cpp',
    ],
    'isr fault': [
        'test/isr/fault.cpp',
        'src/isr/default.cpp',
    ],
    'layout': [
        'test/memory/layout.cpp',
    ],
    # 'elf': [
    #     'test/elf.cpp',
    #     'src/memory.cpp',
    #     'src/elf/elf.cpp',
    #     'src/util/memory.cpp',
    #     'src/boot.cpp',
    #     'src/xsave.cpp',
    #     'src/util/cpuid.cpp',

    #     'src/std/rcu.cpp',
    #     'src/std/rcuptr.cpp',

    #     'src/process/system.cpp',
    #     'src/process/process.cpp',
    #     'src/process/thread.cpp',
    #     'src/process/object.cpp',
    # ],
    'memory': [
        'test/memory.cpp',
        'src/memory.cpp',
        'src/util/memory.cpp',
        'src/boot.cpp',
    ],
    'pte': [
        'test/memory/memory.cpp',
        'src/memory/memory.cpp',
    ],
    'table allocator': [
        'test/memory/table_allocator.cpp',
    ],
    'page table list': [
        'test/memory/table_list.cpp',
    ],
    # 'process': [
    #     'test/process/process.cpp',

    #     'src/process/system.cpp',
    #     'src/process/process.cpp',
    #     'src/process/thread.cpp',
    #     'src/process/object.cpp',

    #     'src/memory.cpp',
    #     'src/xsave.cpp',
    #     'src/util/cpuid.cpp',
    #     'src/elf/elf.cpp',

    #     'src/util/memory.cpp',
    #     'src/boot.cpp',
    # ],
    'system': [
        'test/system/system.cpp',
    ],
    # 'transaction': [
    #     'test/system/transaction.cpp',
    # ],
    'query': [
        'test/system/query.cpp',
    ],
    'handles': [
        'test/system/handles.cpp',
    ],
    'threads': [
        'test/system/thread.cpp',
    ],
    'device': [
        'test/system/device.cpp',
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
    this_test_exe_kwargs = test_exe_kwargs

    if name.contains('+new')
        this_test_exe_kwargs += { 'link_with': this_test_exe_kwargs['link_with'] + [ operator_new_shim_test ] }
        name = name.replace('+new', '')
    endif

    exe = executable('test-kernel-' + name.to_lower().replace(' ', '-').strip(), source,
        kwargs : this_test_exe_kwargs,
    )

    test(name, exe,
        suite : 'kernel',
        kwargs : testkwargs
    )
endforeach

# fuzztest = dependency('fuzztest', native : true)
# fuzz_cpp_args = [
#     '-Wno-sign-compare',
#     '-Wno-missing-field-initializers',
#     '-Wno-unused-parameter',
#     '-Wno-gcc-compat',
# ]

# fuzz_exe_kwargs = test_exe_kwargs
# fuzz_exe_kwargs += { 'dependencies': fuzz_exe_kwargs['dependencies'] + [ fuzztest ] }
# fuzz_exe_kwargs += { 'cpp_args': fuzz_exe_kwargs['cpp_args'] + fuzz_cpp_args }

# exe = executable('test-kernel-fuzz', 'test/fuzz.cpp',
#     kwargs : fuzz_exe_kwargs,
# )

# test('fuzz', exe,
#     suite : 'kernel',
#     kwargs : testkwargs
# )

tar_test_script = find_program('data/test/tar/tar.sh')
tar_test_data = custom_target('tar-test-data',
    output : 'test.tar',
    command : [ tar_test_script, meson.current_source_dir() / 'data/test/tar', '@OUTPUT@' ],
)

source = [
    'test/fs/tarfs.cpp',
    'src/fs2/vfs.cpp',
    'src/fs2/device.cpp',
    'src/fs2/path.cpp',
    'src/fs2/node.cpp',
    'src/fs2/handle.cpp',
    'src/fs2/ramfs.cpp',
    'src/fs2/tarfs.cpp',
    'src/fs2/folder.cpp',
    'src/fs2/utils.cpp',
    'src/fs2/identify.cpp',
    'src/fs2/query.cpp',
    'src/drivers/block/driver.cpp',
    'src/drivers/block/ramblk.cpp',
    'test/fs/fs_test.cpp',
    'src/util/uuid.cpp',
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
#     'src/process/process.cpp',

#     'src/boot.cpp',
#     'src/hypervisor.cpp',

#     'src/std/rcu.cpp',
#     'src/std/rcuptr.cpp',

#     'src/memory.cpp',
#     'src/memory/layout.cpp',
#     'src/memory/page_allocator.cpp',
#     'src/memory/virtual_allocator.cpp',
#     'src/memory/pte.cpp',

#     'src/util/format.cpp',
#     'src/util/memory.cpp',
#     'src/util/cpuid.cpp',

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

mmio_test_src = [
    'test/ktest.cpp',
]

mmio_test_shim = static_library('mmio_test_shim', mmio_test_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, capstone, common ],
    native : true,
)

no_sanitize_test_shim = static_library('no_sanitize_test_shim', test_shim_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    override_options: [ 'b_sanitize=none', 'b_coverage=true' ],
    native : true,
)

no_sanitize_fs_test_lib = static_library('no_sanitize_fs_test', fs_test_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    override_options: [ 'b_sanitize=none', 'b_coverage=true' ],
    native : true,
)

no_sanitize_memory_test_shim = static_library('no_sanitize_memory_test_shim', memory_test_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    override_options: [ 'b_sanitize=none', 'b_coverage=true' ],
    native : true,
)

no_sanitize_system_test_lib = static_library('no_sanitize_system_test', system_test_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    override_options: [ 'b_sanitize=none', 'b_coverage=true' ],
    native : true,
)

testcases = {
    'terminal': {
        'sources': [
            'test/terminal.cpp',
            'src/canvas.cpp',
            'src/terminal/terminal.cpp',
            'src/terminal/buffered.cpp',
        ],
        'dependencies' : [ stb_image_write ],
    },
    'rcu': {
        'sources': [
            'test/std/rcu.cpp',
            'src/std/rcu.cpp',
        ],
        'override_options': [ 'b_sanitize=thread', 'b_coverage=true' ],
        'dependencies' : [ ],
    },
    # rcu shared ptrs are still unsound
    'rcuptr': {
        'sources': [
            'test/std/rcuptr.cpp',
            'src/std/rcu.cpp',
            'src/std/rcuptr.cpp',
        ],
        'override_options': [ 'b_sanitize=address', 'b_coverage=true' ],
        'dependencies' : [ ],
    },
    # 'launch': {
    #     'sources': [
    #         'test/emulated/launch.cpp',
    #     ],
    #     'dependencies' : [ libvirt ],
    # }
}

testcases += {
    'ioapic': {
        'sources': [
            'test/apic/ioapic.cpp',
            'src/apic.cpp',
            'src/memory.cpp',
            'src/boot.cpp',
            'src/util/cpuid.cpp',
            'src/acpi/acpi.cpp',
            'src/acpi/format.cpp',
        ],
        'dependencies': [ capstone, common ],
        # this test must not be run with sanitizers, it ptraces itself
        # and purposely triggers segv and traps
        'override_options': [ 'b_sanitize=none', 'b_coverage=true' ],
        'link_with' : [ no_sanitize_test_shim, no_sanitize_memory_test_shim ]
    },
    'apic': {
        'sources': [
            'test/apic/apic.cpp',
            'src/apic.cpp',
            'src/memory.cpp',
            'src/boot.cpp',
            'src/util/cpuid.cpp',
            'src/acpi/acpi.cpp',
            'src/acpi/format.cpp',
        ],
        'dependencies': [ capstone, common ],
        'override_options': [ 'b_sanitize=none', 'b_coverage=true' ],
        'link_with' : [ no_sanitize_test_shim, no_sanitize_memory_test_shim, mmio_test_shim ],
    },
    'schedule': {
        'sources': [
            'test/system/schedule.cpp',
        ],
        'dependencies': [ capstone, common ],
        'override_options': [ 'b_sanitize=none', 'b_coverage=true' ],
        'link_with' : [ no_sanitize_test_shim, no_sanitize_memory_test_shim, no_sanitize_fs_test_lib, no_sanitize_system_test_lib, mmio_test_shim ],
    }
}

foreach name, kwargs : testcases
    new_kwargs = test_exe_kwargs
    new_kwargs += { 'dependencies': new_kwargs['dependencies'] + kwargs['dependencies'] }
    new_kwargs += { 'sources': kwargs['sources'] }
    new_kwargs += { 'override_options': kwargs.get('override_options', test_exe_kwargs['override_options']) }
    new_kwargs += { 'link_with': kwargs.get('link_with', test_exe_kwargs['link_with']) }

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


# images = {
#     'base': [
#         'test/image/base.cpp',
#     ],
#     'scheduler': [
#         'test/image/schedule.cpp',
#     ]
# }

# foreach name, source : images
#     executable('bezos-test-' + name + '.elf', source + [ 'src/boot/hyper.cpp', ],
#         c_args : c_args + [ '-D__BEZOS__=1' ],
#         cpp_args : cpp_args + [ '-D__BEZOS__=1' ],
#         include_directories : inc,
#         link_with : kernel,
#         dependencies : [ ultra, deps ],
#         link_depends : 'data/linker-hyper.ld',
#         link_args : [ '-T', meson.current_source_dir() / 'data/linker-hyper.ld' ],
#         install : true,
#         override_options : [ 'b_coverage=false' ],
#     )
# endforeach
