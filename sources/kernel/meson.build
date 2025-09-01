project('bezos', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++26',
        'c_std=c23',
        'build.cpp_std=c++26',
        'build.c_std=c23',
        'warning_level=3',
        'b_pie=false',
        'b_staticpic=false',
        'b_coverage=true',

        'gtest:b_coverage=false',
    ],
    version : '0.0.1',
    license : 'GPL-3.0-only',
    meson_version : '>=1.1.0',
)

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

inc = include_directories('include')

common_args = [
    # '-fmacro-prefix-map=' + meson.current_source_dir() + '=/',
    # '-fmacro-prefix-map=../../../=/',

    '-Wno-gcc-compat',
    '-Wno-gnu-anonymous-struct',
    '-Wno-gnu-empty-struct',
    '-Wno-gnu-case-range',
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
deps = [ mp_units, sysapi, concurrentqueue, stb_sprintf, common ]

objcopy = find_program('objcopy')
is_debug = get_option('buildtype').contains('debug')

# Smp startup blob

smp_blob = custom_target('smp',
    input : 'src/smp.S',
    output : 'smp',
    depend_files : 'data/smp.ld',
    command : [
        cc.cmd_array(), '@INPUT@', '-fPIC',
        '-T', meson.current_source_dir() / 'data/smp.ld',
        '-I', meson.current_source_dir() / 'include',
        '-o', '@OUTPUT@',
        '-nostdlib', '-nostdlib++', '-ffreestanding',
        '-fuse-ld=ld', '--ld-path=' + meson.get_external_property('gnu_ld'),
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

aml_option = get_option('aml')

# Sanitizer configuration

sanitizer_src = []
sanitizer_args = []

stack_protector_option = get_option('stack_protector').disable_auto_if(not is_debug)
stack_protector_args = [
    '-fstack-protector-strong'
]

ubsan_option = get_option('ubsan').disable_auto_if(not is_debug)
ubsan_args = [
    '-fsanitize=undefined,nullability',
    '-fno-sanitize=function',
]

if stack_protector_option.allowed()
    sanitizer_src += files('src/sanitizer/stack.cpp')
    sanitizer_args += stack_protector_args
endif

if ubsan_option.allowed()
    sanitizer_src += files('src/sanitizer/ubsan.cpp')
    sanitizer_args += ubsan_args
endif

kernel_args = [
    '-fdebug-prefix-map=../../../=/'
]

kernel_c_args = c_args + kernel_args + sanitizer_args
kernel_cpp_args = cpp_args + kernel_args + sanitizer_args

test_args = [
    # '-fdebug-prefix-map=../../../=/',
    '-DNDEBUG_SANITIZER=1'
]

test_c_args = c_args + test_args
test_cpp_args = cpp_args + test_args

# Kwargs for different build types

# Kernel - targets the kernel image
# Native - default testing target, has addrsan, ubsan, and coverage
# NoSan - for tests that cant run with sanitizers only coverage
# ThreadSan - for tests that need thread sanitizer
# Bench - for benchmarks, no sanitizers, no coverage

kernel_kwargs = {
    'c_args': kernel_c_args,
    'cpp_args': kernel_cpp_args,
    'override_options': [ 'b_sanitize=none', 'b_coverage=false', 'b_ndebug=true' ],
}

native_kwargs = {
    'c_args': test_c_args,
    'cpp_args': test_cpp_args,
    'native': true,
    'build_by_default': false,
    'override_options' : [ 'b_sanitize=address,undefined', 'b_coverage=true', 'b_ndebug=false' ],
}

nosan_kwargs = {
    'c_args': test_c_args,
    'cpp_args': test_cpp_args,
    'native': true,
    'build_by_default': false,
    'override_options' : [ 'b_sanitize=none', 'b_coverage=true', 'b_ndebug=false' ],
}

tsan_kwargs = {
    'c_args': test_c_args,
    'cpp_args': test_cpp_args,
    'native': true,
    'build_by_default': false,
    'override_options' : [ 'b_sanitize=thread', 'b_coverage=false', 'b_ndebug=false' ],
}

bench_kwargs = {
    'c_args': test_c_args,
    'cpp_args': test_cpp_args,
    'native': true,
    'build_by_default': false,
    'override_options' : [ 'b_sanitize=none', 'b_coverage=false', 'b_ndebug=false', 'buildtype=release' ],
}

# TLSF

tlsf_src = tlsf.get_variable('sources')

libtlsf_kernel = static_library('tlsf', tlsf_src,
    kwargs : kernel_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libtlsf_native = static_library('tlsf_native', tlsf_src,
    kwargs : native_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libtlsf_nosanitize = static_library('tlsf_nosanitize', tlsf_src,
    kwargs : nosan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libtlsf_tsan = static_library('tlsf_tsan', tlsf_src,
    kwargs : tsan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libtlsf_bench = static_library('tlsf_bench', tlsf_src,
    kwargs : bench_kwargs,
    include_directories : inc,
    dependencies : deps,
)

# Abseil

absl_inc = [
    absl.get_variable('absl_include_dir'),
]

## Abseil hash

absl_hash_src = [
    absl.get_variable('absl_hash_sources')
]

libabsl_hash_kernel = static_library('absl_hash', absl_hash_src,
    kwargs : kernel_kwargs,
    include_directories : absl_inc,
)

libabsl_hash_native = static_library('absl_hash_native', absl_hash_src,
    kwargs : native_kwargs,
    include_directories : absl_inc,
)

libabsl_hash_nosanitize = static_library('absl_hash_nosanitize', absl_hash_src,
    kwargs : nosan_kwargs,
    include_directories : absl_inc,
)

## Abseil container

absl_container_src = [
    absl.get_variable('absl_container_sources')
]

libabsl_container_kernel = static_library('absl_container', absl_container_src,
    kwargs : kernel_kwargs,
    include_directories : absl_inc,
    link_with : [ libabsl_hash_kernel ],
)

libabsl_container_native = static_library('absl_container_native', absl_container_src,
    kwargs : native_kwargs,
    include_directories : absl_inc,
    link_with : [ libabsl_hash_native ],
)

libabsl_container_nosanitize = static_library('absl_container_nosanitize', absl_container_src,
    kwargs : nosan_kwargs,
    include_directories : absl_inc,
    link_with : [ libabsl_hash_nosanitize ],
)

# RCU

rcu_src = files(
    'src/std/rcu.cpp',
    'src/std/rcuptr.cpp',
)

librcu_kernel = static_library('rcu', rcu_src,
    kwargs : kernel_kwargs,
    include_directories : inc,
    dependencies : deps,
)

librcu_native = static_library('rcu_native', rcu_src,
    kwargs : native_kwargs,
    include_directories : inc,
    dependencies : deps,
)

librcu_nosanitize = static_library('rcu_nosanitize', rcu_src,
    kwargs : nosan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

librcu_tsan = static_library('rcu_tsan', rcu_src,
    kwargs : tsan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

librcu_bench = static_library('rcu_bench', rcu_src,
    kwargs : bench_kwargs,
    include_directories : inc,
    dependencies : deps,
)

# Formatting

format_src = files(
    'src/util/format.cpp',
)

libformat_kernel = static_library('format', format_src,
    kwargs : kernel_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libformat_native = static_library('format_native', format_src,
    kwargs : native_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libformat_nosanitize = static_library('format_nosanitize', format_src,
    kwargs : nosan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libformat_bench = static_library('format_bench', format_src,
    kwargs : bench_kwargs,
    include_directories : inc,
    dependencies : deps,
)

# Logging

logging_src = files(
    'src/logger/logger.cpp',
    'src/logger/global_logger.cpp',
    'src/logger/vga_appender.cpp',
    'src/logger/serial_appender.cpp',
    'src/logger/e9_appender.cpp',
)

liblogging_kernel = static_library('logging', logging_src,
    kwargs : kernel_kwargs,
    include_directories : inc,
    dependencies : deps,
)

liblogging_native = static_library('logging_native', logging_src,
    kwargs : native_kwargs,
    include_directories : inc,
    dependencies : deps,
)

liblogging_nosanitize = static_library('logging_nosanitize', logging_src,
    kwargs : nosan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

# Paging

paging_src = files(
    'src/memory/memory.cpp',
    'src/memory/pte.cpp',
    'src/memory/table_allocator.cpp',
    'src/memory/detail/table_list.cpp',
    'src/memory/detail/control_block.cpp',
)

libpaging_kernel = static_library('paging', paging_src,
    kwargs : kernel_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libpaging_native = static_library('paging_native', paging_src,
    kwargs : native_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libpaging_nosanitize = static_library('paging_nosanitize', paging_src,
    kwargs : nosan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

# ACPI

acpi_src = files(
    'src/acpi/acpi.cpp',
    'src/acpi/format.cpp',
)

libacpi_kernel = static_library('acpi', acpi_src,
    kwargs : kernel_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libacpi_native = static_library('acpi_native', acpi_src,
    kwargs : native_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libacpi_nosanitize = static_library('acpi_nosanitize', acpi_src,
    kwargs : nosan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

# Task scheduling

task_src = files(
    'src/task/scheduler_queue.cpp',
    'src/task/scheduler.cpp',
    'src/task/mutex.cpp',
    'src/task/runtime.cpp',
    'src/task/runtime.S',
)

libtask_kernel = static_library('task', task_src,
    kwargs : kernel_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libtask_native = static_library('task_native', task_src,
    kwargs : native_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libtask_nosanitize = static_library('task_nosanitize', task_src,
    kwargs : nosan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libtask_tsan = static_library('task_tsan', task_src,
    kwargs : tsan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libtask_bench = static_library('task_bench', task_src,
    kwargs : bench_kwargs,
    include_directories : inc,
    dependencies : deps,
)

# Memory

memory_src = files(
    'src/memory/layout.cpp',
    'src/memory/allocator.cpp',
    'src/memory/page_allocator.cpp',
    'src/memory/page_allocator_command_list.cpp',
    'src/memory/pte_command_list.cpp',
    'src/memory/tables.cpp',
    'src/memory/address_space.cpp',
    'src/memory/heap.cpp',
    'src/memory/heap_command_list.cpp',
)

libmemory_kernel = static_library('memory', memory_src,
    kwargs : kernel_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libmemory_native = static_library('memory_native', memory_src,
    kwargs : native_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libmemory_nosanitize = static_library('memory_nosanitize', memory_src,
    kwargs : nosan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

# VFS

vfs_src = files(
    'src/fs/vfs.cpp',
    'src/fs/path.cpp',
    'src/fs/node.cpp',
    'src/fs/handle.cpp',
    'src/fs/device.cpp',
    'src/fs/folder.cpp',
    'src/fs/utils.cpp',
    'src/fs/identify.cpp',
    'src/fs/query.cpp',
    'src/fs/iterator.cpp',
)

libvfs_kernel = static_library('vfs', vfs_src,
    kwargs : kernel_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libvfs_native = static_library('vfs_native', vfs_src,
    kwargs : native_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libvfs_nosanitize = static_library('vfs_nosanitize', vfs_src,
    kwargs : nosan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

# System

system_src = files(
    'src/system/system.cpp',
    'src/system/schedule.cpp',
    'src/system/handle.cpp',
    'src/system/process.cpp',
    'src/system/thread.cpp',
    'src/system/mutex.cpp',
    'src/system/node.cpp',
    'src/system/device.cpp',
    'src/system/transaction.cpp',
    'src/system/pmm.cpp',
    'src/system/vmm.cpp',
    'src/system/sanitize.cpp',

    'src/system/vm/mapping.cpp',
)

libsystem_kernel = static_library('system', system_src,
    kwargs : kernel_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libsystem_native = static_library('system_native', system_src,
    kwargs : native_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libsystem_nosanitize = static_library('system_nosanitize', system_src,
    kwargs : nosan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

# Utilities

util_src = files(
    'src/util/cpuid.cpp',
    'src/util/memory.cpp',
    'src/util/uuid.cpp',
)

libutil_kernel = static_library('util', util_src,
    kwargs : kernel_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libutil_native = static_library('util_native', util_src,
    kwargs : native_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libutil_nosanitize = static_library('util_nosanitize', util_src,
    kwargs : nosan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

# APIC

apic_src = files(
    'src/apic.cpp',
    'src/delay.cpp',
)

libapic_kernel = static_library('apic', apic_src,
    kwargs : kernel_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libapic_native = static_library('apic_native', apic_src,
    kwargs : native_kwargs,
    include_directories : inc,
    dependencies : deps,
)

libapic_nosanitize = static_library('apic_nosanitize', apic_src,
    kwargs : nosan_kwargs,
    include_directories : inc,
    dependencies : deps,
)

# Kernel

src = [
    # C and C++ Runtime
    'src/crt.cpp',
    'src/cxxrt.cpp',
    'src/crt_freestanding.cpp',
    sanitizer_src,

    # Interrupt handling
    'src/isr/isr.cpp',
    'src/isr/isr.S',
    'src/isr/tables.cpp',
    'src/isr/default.cpp',

    # Architecture features
    'src/gdt.cpp',
    'src/pat.cpp',
    'src/check.cpp',
    'src/uart.cpp',
    'src/hypervisor.cpp',
    'src/smbios.cpp',
    # 'src/delay.cpp',
    'src/cmos.cpp',
    'src/debug.cpp',
    'src/xsave.cpp',
    'src/clock.cpp',
    'src/ipl.cpp',

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

    # VFS drivers
    'src/fs/ramfs.cpp',
    'src/fs/tarfs.cpp',

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
    'src/user/sysapi/device.cpp',
    'src/user/sysapi/handle.cpp',
    'src/user/sysapi/node.cpp',
    'src/user/sysapi/process.cpp',
    'src/user/sysapi/vmem.cpp',
    'src/user/sysapi/thread.cpp',
    'src/user/sysapi/mutex.cpp',

    # Services
    'src/notify.cpp',

    'src/system/runtime.cpp',
    'src/system/runtime2.cpp',

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

if aml_option.allowed()
    src += [ 'src/acpi/aml.cpp', ]
endif

kernel_libs = [
    libabsl_hash_kernel,
    libabsl_container_kernel,
    libtlsf_kernel,
    librcu_kernel,
    libutil_kernel,
    libformat_kernel,
    liblogging_kernel,
    libmemory_kernel,
    libacpi_kernel,
    libapic_kernel,
    libvfs_kernel,
    libsystem_kernel,
    libpaging_kernel,
    libtask_kernel,
]

kernel = static_library('kernel', src,
    include_directories : inc,
    c_args : kernel_c_args + [ '-mllvm', '-asm-macro-max-nesting-depth=256' ],
    cpp_args : kernel_cpp_args,
    dependencies : deps,
    link_with : kernel_libs,
    override_options : kernel_kwargs['override_options'],
)

kmain = static_library('kernel-main', 'src/main.cpp',
    include_directories : inc,
    c_args : kernel_c_args,
    cpp_args : kernel_cpp_args,
    dependencies : deps,
    link_with : kernel,
    override_options : kernel_kwargs['override_options'],
)

###
### Limine version of the kernel image
###

executable('bezos-limine.elf', 'src/boot/limine.cpp',
    c_args : c_args,
    cpp_args : cpp_args,
    include_directories : inc,
    link_with : kmain,
    dependencies : [ limine, deps ],
    link_depends : 'data/linker-limine.ld',
    link_args : [ '-T', meson.current_source_dir() / 'data/linker-limine.ld' ],
    install : true,
    override_options : kernel_kwargs['override_options'],
)

###
### Hyper version of the kernel image
###

executable('bezos-hyper.elf', 'src/boot/hyper.cpp',
    c_args : c_args,
    cpp_args : cpp_args,
    include_directories : inc,
    link_with : kmain,
    dependencies : [ ultra, deps ],
    link_depends : 'data/linker-hyper.ld',
    link_args : [ '-T', meson.current_source_dir() / 'data/linker-hyper.ld' ],
    install : true,
    override_options : kernel_kwargs['override_options'],
)

# Unit tests

gtest = dependency('gtest', native : true)
gmock = dependency('gmock', native : true)
stb_image_write = dependency('stb_image_write')
capstone = dependency('capstone', native : true)
gtest_main = dependency('gtest_main', native : true)
testkwargs = { 'protocol' : 'gtest', 'timeout': 5 }

subdir('pvtest')
subdir('ktest')
subdir('test')
