project('bezos', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++26',
        'c_std=c23',
        'build.cpp_std=c++26',
        'build.c_std=c23',
        'warning_level=3',
        'b_pie=false',
        'b_staticpic=false',
        'b_ndebug=true',
        'cpp_eh=none',
        'cpp_rtti=false',
        'b_coverage=false',
        'build.b_coverage=true',
    ],
    version : '0.0.1',
    license : 'GPL-3.0-only',
    meson_version : '>=1.1.0',
)

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

inc = include_directories('include')

common_args = [
    '-fmacro-prefix-map=' + meson.current_source_dir() + '=/',
    '-fmacro-prefix-map=../../../=/',

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
is_debug = get_option('buildtype').contains('debug')

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

aml_option = get_option('aml')

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

kernel_kwargs = {
    'c_args': c_args + kernel_args + sanitizer_args,
    'cpp_args': cpp_args + kernel_args + sanitizer_args,
}

native_kwargs = {
    'c_args': c_args,
    'cpp_args': cpp_args,
    'native': true,
    'override_options' : [ 'b_sanitize=address,undefined', 'b_coverage=true' ],
}

nosan_kwargs = {
    'c_args': c_args,
    'cpp_args': cpp_args,
    'native': true,
    'override_options' : [ 'b_sanitize=none', 'b_coverage=true' ],
}

# Kernel

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

rcu_src = files(
    'src/std/rcu.cpp',
    'src/std/rcuptr.cpp',
)

src = [
    tlsf.get_variable('sources'),

    # Utilities
    'src/util/cpuid.cpp',
    'src/util/memory.cpp',
    'src/util/format.cpp',
    'src/util/uuid.cpp',

    # RCU
    rcu_src,

    # Memory management
    'src/memory/layout.cpp',
    'src/memory/allocator.cpp',
    'src/memory/page_allocator.cpp',
    'src/memory/page_allocator_command_list.cpp',
    'src/memory/table_allocator.cpp',
    'src/memory/pte.cpp',
    'src/memory/pte_command_list.cpp',
    'src/memory/tables.cpp',
    'src/memory/memory.cpp',
    'src/memory/address_space.cpp',
    'src/memory/detail/table_list.cpp',
    'src/memory/detail/control_block.cpp',
    'src/memory/heap.cpp',
    'src/memory/heap_command_list.cpp',

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
    'src/apic.cpp',
    'src/uart.cpp',
    'src/hypervisor.cpp',
    'src/smbios.cpp',
    'src/delay.cpp',
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
    system_src,

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
    libs,
    liblogging_kernel,
]

kernel = static_library('kernel', src,
    include_directories : inc,
    c_args : kernel_c_args + [ '-mllvm', '-asm-macro-max-nesting-depth=256' ],
    cpp_args : kernel_cpp_args,
    dependencies : deps,
    link_with : kernel_libs,
    override_options : [ 'b_coverage=false' ],
)

kmain = static_library('kernel-main', 'src/main.cpp',
    include_directories : inc,
    c_args : kernel_c_args,
    cpp_args : kernel_cpp_args,
    dependencies : deps,
    link_with : kernel,
    override_options : [ 'b_coverage=false' ],
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
    override_options : [ 'b_coverage=false' ],
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

subdir('pvtest')
subdir('test')
