bench = dependency('benchmark-main',
    native : true,
    default_options : [
        'tests=disabled',
    ]
)

test_sanitize_args = [
    '-fsanitize=nullability'
]

# native executables dont have all default_options applied
test_c_args = c_args + [ '-std=gnu23', '-DUTIL_TESTING=1', '-DNDEBUG_SANITIZER=1' ]

# use NDEBUG_SANITIZER to disable some abseil debug tools,
# it causes issues with the freestanding patches ive made
test_cpp_args = cpp_args + [ '-std=gnu++26', '-DUTIL_TESTING=1', '-DNDEBUG_SANITIZER=1' ]

test_link_args = [ test_sanitize_args ]

test_shim_src = [
    'test_shim.cpp',
    'ports.cpp',
    '../src/delay.cpp',
    '../src/util/format.cpp',
]

fs_test_src = [
    '../src/fs/vfs.cpp',
    '../src/fs/path.cpp',
    '../src/fs/node.cpp',
    '../src/fs/handle.cpp',
    '../src/fs/ramfs.cpp',
    '../src/fs/device.cpp',
    '../src/fs/folder.cpp',
    '../src/fs/utils.cpp',
    '../src/fs/identify.cpp',
    '../src/fs/query.cpp',
    '../src/fs/iterator.cpp',

    '../src/util/uuid.cpp',
]

system_test_src = [
    '../src/system/system.cpp',
    '../src/system/schedule.cpp',
    '../src/system/handle.cpp',
    '../src/system/process.cpp',
    '../src/system/thread.cpp',
    '../src/system/mutex.cpp',
    '../src/system/node.cpp',
    '../src/system/device.cpp',
    '../src/system/transaction.cpp',
    '../src/system/pmm.cpp',
    '../src/system/vmm.cpp',
    '../src/system/detail/address_segment.cpp',
    '../src/system/sanitize.cpp',

    '../src/system/vm/mapping.cpp',

    '../src/devices/stream.cpp',

    '../src/memory.cpp',
    '../src/boot.cpp',

    '../src/xsave.cpp',
    '../src/util/cpuid.cpp',
]

new_shim_test_src = [
    'new_shim.cpp'
]

operator_new_shim_test = static_library('operator_new_shim_test', new_shim_test_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, common ],
    native : true,
    override_options : native_kwargs['override_options'],
)

libtest_shim = static_library('test_shim', test_shim_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    link_with : [ libabsl_container_native ],
    native : true,
    override_options : native_kwargs['override_options'],
)

libtest_shim_tsan = static_library('test_shim_tsan', test_shim_src,
    include_directories : inc,
    c_args : test_c_args + test_sanitize_args,
    cpp_args : test_cpp_args + test_sanitize_args,
    link_args : test_sanitize_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    native : true,
    override_options : tsan_kwargs['override_options'],
)

libtest_shim_nosanitize = static_library('test_shim_nosanitize', test_shim_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    native : true,
    override_options : nosan_kwargs['override_options'],
)

fs_test_shim = static_library('fs_test_shim', fs_test_src,
    include_directories : inc,
    c_args : test_c_args + test_sanitize_args,
    cpp_args : test_cpp_args + test_sanitize_args,
    link_args : test_sanitize_args,
    link_with : [ librcu_native ],
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    override_options : native_kwargs['override_options'],
    native : true,
)

system_test_lib = static_library('system_test', system_test_src,
    include_directories : inc,
    c_args : test_c_args + test_sanitize_args,
    cpp_args : test_cpp_args + test_sanitize_args,
    link_args : test_sanitize_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    override_options : native_kwargs['override_options'],
    native : true,
)

test_exe_kwargs = {
    'include_directories': [ inc, include_directories('.') ],
    'c_args': test_c_args + test_sanitize_args,
    'cpp_args': test_cpp_args + test_sanitize_args,
    'link_args': test_sanitize_args,
    'dependencies': [ gtest_main, sysapi, concurrentqueue, mp_units, common ],
    'override_options': native_kwargs['override_options'],
    'native': true,
    'link_with': [
        libtest_shim,
        fs_test_shim,
        system_test_lib,

        libutil_native,
        libformat_native,
        libpaging_native,
        libmemory_native,
        liblogging_native,
        libtask_native
    ],
}

# Benchmarks

benchcases = {
    'ringbuffer': {
        'sources': files('bench/ringbuffer.cpp'),
        'link_with': [ libformat_bench ],
    },
    'btree': {
        'sources': files('std/container/btree_bench.cpp')
    }
}

foreach name, setup : benchcases
    exe = executable('bench-kernel-' + name.strip().replace(' ', '-'), setup['sources'],
        c_args : setup.get('c_args', test_c_args),
        cpp_args : setup.get('cpp_args', test_cpp_args),
        link_args : setup.get('link_args', test_link_args),
        link_with : setup.get('link_with', []),
        include_directories : inc,
        dependencies : setup.get('dependencies', []) + [ common, sysapi, bench ],
        override_options : setup.get('override_options', bench_kwargs['override_options']),
        native : true,
        pie : true,
    )

    benchmark(name, exe,
        suite : 'kernel',
        timeout : 120,
    )
endforeach

##
## Tests
##

testcases = {
    'static vector': {
        'sources': files('static_vector.cpp'),
        'link_with': [ libtest_shim ],
    },
    'fixed deque': {
        'sources': files('fixed_deque.cpp')
    },
    'vector': {
        'sources': files('vector.cpp'),
        'link_with': [ libtest_shim ],
    },
    'inlined vector': {
        'sources': files('std/inlined_vector.cpp'),
        'link_with': [ operator_new_shim_test ],
    },
    'format': {
        'sources': files('format.cpp', '../src/util/format.cpp')
    },
    'string': {
        'sources': files('std/string.cpp'),
    },
    'btree': {
        'sources': files('std/container/btree.cpp'),
        'link_with': [ libtest_shim ],
        'timeout': 60,
    },
    'btree 128': {
        'sources': files('std/container/btree.cpp'),
        'link_with': [ libtest_shim ],
        'cpp_args': [ test_cpp_args, '-DBTREE_KEY_SIZE=128' ],
        'timeout': 30,
    },
    'btree 256': {
        'sources': files('std/container/btree.cpp'),
        'link_with': [ libtest_shim ],
        'cpp_args': [ test_cpp_args, '-DBTREE_KEY_SIZE=256' ],
        'timeout': 30,
    },
    'btree 1024': {
        'sources': files('std/container/btree.cpp'),
        'link_with': [ libtest_shim ],
        'cpp_args': [ test_cpp_args, '-DBTREE_KEY_SIZE=1024' ],
        'timeout': 30,
    },
    'btree 2048': {
        'sources': files('std/container/btree.cpp'),
        'link_with': [ libtest_shim ],
        'cpp_args': [ test_cpp_args, '-DBTREE_KEY_SIZE=2048' ],
        'timeout': 30,
    },
    'btree string': {
        'sources': files('std/container/btree_string.cpp'),
        'link_with': [ libtest_shim ],
        'timeout': 30,
    },
    'spinlock': {
        'sources': files('std/spinlock.cpp'),
    },
    'ringbuffer': {
        'sources': files('std/ringbuffer.cpp'),
    },
    'static string': {
        'sources': files('std/static_string.cpp'),
    },
    'string view': {
        'sources': files('std/string_view.cpp'),
    },
    'logger': {
        'sources': files('logger.cpp', '../src/logger/logger.cpp')
    },
    'memory range': {
        'sources': files('memory_range.cpp'),
        'link_with': [ libtest_shim ],
    },
    'pat': {
        'sources': files('page_attribute_table.cpp', '../src/pat.cpp', '../src/util/cpuid.cpp'),
    },
    'paging': {
        'sources': files('paging.cpp'),
    },
    'arch cr': {
        'sources': files('arch/cr.cpp', '../src/arch/cr.cpp'),
    },
    'arch gdt': {
        'sources': files('arch/gdt.cpp', '../src/gdt.cpp'),
    },
    'framebuffer': {
        'sources': files('display.cpp', '../src/canvas.cpp'),
    },
    'crt': {
        'sources': files('crt.cpp', '../src/crt.cpp'),
    },
    # TODO: these are broken due to some quite arkane issues with abseil and sanitizers
    'serial': {
        'sources': files('serial.cpp', '../src/uart.cpp'),
        'dependencies': [ gmock ],
        'link_with': [ libtest_shim, liblogging_native ],
    },
    # 'query': {
    #     'sources': files('system/query.cpp'),
    #     'dependencies': [ mp_units ],
    #     'link_with': [
    #         libtest_shim,
    #         librcu_native,
    #         libtlsf_native,
    #         libsystem_native,
    #         libpaging_native,
    #         libmemory_native,
    #         liblogging_native,
    #         libvfs_native,
    #     ],
    # },
    'page table': {
        'sources': files('memory/page_tables.cpp'),
        'link_with': [ libtest_shim, libutil_native, libpaging_native, liblogging_native, ],
    },
    'rcu': {
        'sources': [
            files('std/rcu.cpp'),
        ],
        'override_options': tsan_kwargs['override_options'],
        'link_with': [ libtlsf_tsan, librcu_tsan, libtest_shim_tsan ],
    },
    'rcu retire slots': {
        'sources': [
            files('std/rcu_retire_slots.cpp'),
        ],
        'override_options': tsan_kwargs['override_options'],
        'link_with': [ libtlsf_tsan, librcu_tsan, libtest_shim_tsan ],
    },
    'rcu shared': {
        'sources': [
            files('std/rcu_shared.cpp'),
        ],
        'link_with': [ librcu_native, libtest_shim ],
    },
    'task scheduler queue': {
        'sources': [
            files('task/scheduler_queue.cpp'),
        ],
        'link_with': [ libtask_native, liblogging_native, libtest_shim ],
        'dependencies': [ mp_units ],
    },
    # TODO: this test is run without sanitizers until I can figure out why addrsan doesnt like
    # sigqueue happening on multiple threads
    'task scheduler': {
        'sources': [
            files('task/scheduler.cpp'),
        ],
        'override_options': nosan_kwargs['override_options'],
        'link_with': [ libtask_nosanitize, liblogging_nosanitize, libtest_shim_nosanitize ],
        'dependencies': [ mp_units ],
    },
    'task scheduler sleep': {
        'sources': [
            files('task/scheduler_sleep.cpp'),
        ],
        'override_options': nosan_kwargs['override_options'],
        'link_with': [ libtask_nosanitize, liblogging_nosanitize, libtest_shim_nosanitize ],
        'dependencies': [ mp_units ],
    }
}

##
## Soak tests
##

testcases += {
    'rcu soak': {
        'sources': [
            files('std/soak/rcu_soak.cpp'),
        ],
        'link_with': [ libtlsf_native, librcu_native, libtest_shim ],
        'soak': true,
        'protocol': 'exitcode',
    },
    'rcu retire soak': {
        'sources': [
            files('std/soak/rcu_retire_soak.cpp'),
        ],
        'link_with': [ libtlsf_native, librcu_native, libtest_shim ],
        'soak': true,
        'protocol': 'exitcode',
    },
    'rcu retire slots soak': {
        'sources': [
            files('std/soak/rcu_retire_slots_soak.cpp'),
        ],
        'link_with': [ libtlsf_native, librcu_native, libtest_shim ],
        'soak': true,
        'protocol': 'exitcode',
    },
    'ringbuffer soak': {
        'sources': [
            files('std/soak/ringbuffer_soak.cpp'),
        ],
        'soak': true,
        'link_with': [ libtlsf_native ],
    },
    'rcu shared soak': {
        'sources': [
            files('std/soak/rcu_shared_soak.cpp'),
        ],
        'link_with': [ librcu_native, libtest_shim ],
        'soak': true,
        'protocol': 'exitcode',
    },
    'rcu cmpxchg soak': {
        'sources': [
            files('std/soak/rcu_cmpxchg_soak.cpp'),
        ],
        'link_with': [ librcu_native, libtest_shim ],
        'soak': true,
        'protocol': 'exitcode',
    },
    'btree stress': {
        'sources': [
            files('std/container/btree_stress.cpp'),
        ],
        'link_with': [ libtest_shim ],
        'soak': true,
        'protocol': 'exitcode',
    },
}

foreach name, setup : testcases
    protocol = setup.get('protocol', 'gtest')
    dependencies = [ common, sysapi ]
    if protocol == 'gtest'
        dependencies += [ gtest_main ]
    endif

    exe = executable('test-kernel-' + name.strip().replace(' ', '-'), setup['sources'],
        c_args : setup.get('c_args', test_c_args),
        cpp_args : setup.get('cpp_args', test_cpp_args),
        link_args : setup.get('link_args', test_link_args),
        link_with : setup.get('link_with', []),
        include_directories : inc,
        dependencies : setup.get('dependencies', []) + dependencies,
        override_options : setup.get('override_options', native_kwargs['override_options']),
        native : true,
        pie : true,
    )

    timeout = setup.get('timeout', 5)
    suite = 'kernel'
    if setup.get('soak', false)
        timeout = 60 * 5
        suite = 'kernel-soak'
    endif

    test(name, exe,
        suite : suite,
        protocol : protocol,
        timeout : timeout,
    )
endforeach

fuzztests = {
    'btree': {
        'sources': files('std/container/btree_fuzz.cpp'),
    }
}

foreach name, setup : fuzztests
    formatted_name = name.strip().replace(' ', '-')
    exe = executable('test-fuzz-' + formatted_name, setup['sources'],
        c_args : test_c_args + [ '-fsanitize=fuzzer' ],
        cpp_args : test_cpp_args + [ '-fsanitize=fuzzer' ],
        link_args : test_link_args + [ '-fsanitize=fuzzer' ],
        include_directories : inc,
        link_with : [ libtest_shim ],
        dependencies : [ sysapi, mp_units, common ],
        override_options : native_kwargs['override_options'],
        native : true,
    )

    run_target('fuzz-' + formatted_name,
        command : [ exe ]
    )
endforeach

testcases = {
    'ipl': [ 'ipl.cpp' ],
    'page table command list +new': [
        'memory/pte_command_list.cpp',
    ],
    'partitions': [
        'drivers/partitions.cpp',
        '../src/drivers/fs/driver.cpp',
        '../src/drivers/block/driver.cpp',
        '../src/drivers/block/ramblk.cpp',
    ],
    'ramblk': [
        'drivers/ramblk.cpp',
        '../src/drivers/block/driver.cpp',
        '../src/drivers/block/ramblk.cpp',
    ],
    'media': [
        'drivers/media.cpp',
        '../src/drivers/block/driver.cpp',
        '../src/drivers/block/ramblk.cpp',
    ],
    'fs2': [
        'fs/fs.cpp',
    ],
    'vfs path': [
        'fs/path.cpp',
    ],
    'vfs root': [
        'fs/root.cpp',
    ],
    'ramfs': [
        'fs/ramfs.cpp',
    ],
    'fs update': [
        'fs/update.cpp',
    ],
    'identify': [
        'fs/ramfs.cpp',
    ],
    'verify': [
        'user/verify.cpp',
        '../src/user/context.cpp',
    ],
    'shared ptr': [
        'shared.cpp',
    ],
    'atomic forward list': [
        'std/forward_list.cpp',
    ],
    'cmos': [
        'cmos.cpp',
        '../src/cmos.cpp',
    ],
    'notify': [
        'notify.cpp',
        '../src/notify.cpp',
        '../src/std/rcu.cpp',
        '../src/std/rcuptr.cpp',
        tlsf.get_variable('sources'),
    ],
    'isr table': [
        'isr/isr.cpp',
        '../src/isr/default.cpp',
    ],
    'isr fault': [
        'isr/fault.cpp',
        '../src/isr/default.cpp',
    ],
    'layout': [
        'memory/layout.cpp',
    ],
    # 'elf': [
    #     'elf.cpp',
    #     '../src/memory.cpp',
    #     '../src/elf/elf.cpp',
    #     '../src/util/memory.cpp',
    #     '../src/boot.cpp',
    #     '../src/xsave.cpp',
    #     '../src/util/cpuid.cpp',

    #     '../src/std/rcu.cpp',
    #     '../src/std/rcuptr.cpp',

    #     '../src/process/system.cpp',
    #     '../src/process/process.cpp',
    #     '../src/process/thread.cpp',
    #     '../src/process/object.cpp',
    # ],
    'memory': [
        'memory.cpp',
        '../src/memory.cpp',
        '../src/util/memory.cpp',
        '../src/boot.cpp',
    ],
    'pte': [
        'memory/memory.cpp',
        '../src/memory/memory.cpp',
    ],
    'table allocator': [
        'memory/table_allocator.cpp',
    ],
    'page table list': [
        'memory/table_list.cpp',
    ],
    'system': [
        'system/system.cpp',
    ],
    # 'transaction': [
    #     'system/transaction.cpp',
    # ],
    'handles': [
        'system/handles.cpp',
    ],
    # 'threads': [
    #     'system/thread.cpp',
    # ],
    'device': [
        'system/device.cpp',
    ],
    'system detail range table': [
        'system/detail/range_table.cpp',
    ],
    'system vmm +new': [
        'system/vmm.cpp',
        '../src/memory/heap.cpp',
        '../src/system/pmm.cpp',
        '../src/system/vmm.cpp',
        '../src/memory/heap_command_list.cpp',
    ],
    'system pmm +new': [
        'system/pmm.cpp',
        '../src/memory/heap.cpp',
        '../src/system/pmm.cpp',
        '../src/memory/heap_command_list.cpp',
    ],
    'tlsf heap +new': [
        'memory/tlsf.cpp',
        '../src/memory/heap.cpp',
    ],
    'tlsf heap command list +new': [
        'memory/heap_command_list.cpp',
        '../src/memory/heap_command_list.cpp',
        '../src/memory/heap.cpp',
    ],
    'pool allocator +new': [
        'memory/pool.cpp',
    ],
    'page allocator': [
        'memory/page_allocator.cpp',
    ],
    'address space': [
        'memory/address_space.cpp',
    ],
    'system memory': [
        'system_memory.cpp',
    ],
}

if aml_option.allowed()
    testcases += {
        'aml details': [
            'acpi/detail.cpp',
            '../src/acpi/format.cpp',
            '../src/acpi/aml.cpp',
            '../src/util/format.cpp',
        ],
        'aml names': [
            'acpi/name.cpp',
            '../src/acpi/format.cpp',
            '../src/acpi/aml.cpp',
            '../src/util/format.cpp',
        ],
        'aml nodes': [
            'acpi/nodes.cpp',
            '../src/acpi/format.cpp',
            '../src/acpi/aml.cpp',
            '../src/util/format.cpp',
        ]
    }
endif

foreach name, source : testcases
    this_test_exe_kwargs = test_exe_kwargs

    if name.contains('+new')
        this_test_exe_kwargs += { 'link_with': this_test_exe_kwargs['link_with'] + [ operator_new_shim_test ] }
        name = name.replace('+new', '').strip()
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

tar_test_script = find_program('../data/test/tar/tar.sh')
tar_test_data = custom_target('tar-test-data',
    output : 'test.tar',
    command : [ tar_test_script, meson.project_source_root() / 'data/test/tar', '@OUTPUT@' ],
)

source = [
    'fs/tarfs.cpp',
    '../src/fs/vfs.cpp',
    '../src/fs/device.cpp',
    '../src/fs/path.cpp',
    '../src/fs/node.cpp',
    '../src/fs/handle.cpp',
    '../src/fs/ramfs.cpp',
    '../src/fs/tarfs.cpp',
    '../src/fs/folder.cpp',
    '../src/fs/utils.cpp',
    '../src/fs/identify.cpp',
    '../src/fs/query.cpp',
    '../src/drivers/block/driver.cpp',
    '../src/drivers/block/ramblk.cpp',
    'fs/fs_test.cpp',
    '../src/util/uuid.cpp',
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

#     '../src/launch.cpp',

#     '../src/fs/vfs.cpp',
#     '../src/fs/path.cpp',
#     '../src/fs/node.cpp',
#     '../src/fs/handle.cpp',
#     '../src/fs/ramfs.cpp',
#     '../src/fs/tarfs.cpp',
#     '../src/process/process.cpp',

#     '../src/boot.cpp',
#     '../src/hypervisor.cpp',

#     '../src/std/rcu.cpp',
#     '../src/std/rcuptr.cpp',

#     '../src/memory.cpp',
#     '../src/memory/layout.cpp',
#     '../src/memory/page_allocator.cpp',
#     '../src/memory/virtual_allocator.cpp',
#     '../src/memory/pte.cpp',

#     '../src/util/format.cpp',
#     '../src/util/memory.cpp',
#     '../src/util/cpuid.cpp',

#     '../src/drivers/block/driver.cpp',
#     '../src/drivers/block/ramblk.cpp',
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
    'ktest.cpp',
]

mmio_test_shim = static_library('mmio_test_shim', mmio_test_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, capstone, common ],
    override_options : nosan_kwargs['override_options'],
    native : true,
)

no_sanitize_test_shim = static_library('no_sanitize_test_shim', test_shim_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    override_options : nosan_kwargs['override_options'],
    native : true,
)

no_sanitize_fs_test_lib = static_library('no_sanitize_fs_test', fs_test_src,
    include_directories : inc,
    c_args : test_c_args,
    cpp_args : test_cpp_args,
    dependencies : [ gtest, sysapi, concurrentqueue, mp_units, common ],
    override_options : nosan_kwargs['override_options'],
    native : true,
)

testcases = {
    'terminal': {
        'sources': [
            'terminal.cpp',
            '../src/canvas.cpp',
            '../src/terminal/terminal.cpp',
            '../src/terminal/buffered.cpp',
        ],
        'dependencies' : [ stb_image_write ],
    },
}

testcases += {
    'ioapic': {
        'sources': [
            'apic/ioapic.cpp',
            '../src/apic.cpp',
            '../src/memory.cpp',
            '../src/boot.cpp',
            '../src/util/cpuid.cpp',
            '../src/acpi/acpi.cpp',
            '../src/acpi/format.cpp',
        ],
        'dependencies': [ capstone, common ],
        'override_options': nosan_kwargs['override_options'],
        'link_with' : [
            no_sanitize_test_shim,
            libutil_nosanitize,
            libformat_nosanitize,
            libpaging_nosanitize,
            libmemory_nosanitize,
            liblogging_nosanitize,
        ]
    },
    'apic': {
        'sources': [
            'apic/apic.cpp',
            '../src/apic.cpp',
            '../src/memory.cpp',
            '../src/boot.cpp',
            '../src/util/cpuid.cpp',
            '../src/acpi/acpi.cpp',
            '../src/acpi/format.cpp',
        ],
        'dependencies': [ capstone, common ],
        'override_options': nosan_kwargs['override_options'],
        'link_with' : [
            no_sanitize_test_shim,
            mmio_test_shim,
            libutil_nosanitize,
            libformat_nosanitize,
            libpaging_nosanitize,
            libmemory_nosanitize,
            liblogging_nosanitize
        ],
    },
    # TODO: also broken due to abseil and sanitizers
    # 'schedule': {
    #     'sources': [
    #         'system/schedule.cpp',
    #     ],
    #     'dependencies': [ capstone, common ],
    #     'override_options': nosan_kwargs['override_options'],
    #     'link_with' : [
    #         libtest_shim_nosanitize,
    #         librcu_nosanitize,
    #         libvfs_nosanitize,
    #         libmemory_nosanitize,
    #         libpaging_nosanitize,
    #         libsystem_nosanitize,
    #         liblogging_nosanitize,
    #         libabsl_container_nosanitize
    #     ],
    # }
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
            # 'LSAN_OPTIONS': 'suppressions=' + meson.current_source_dir() / 'data/test/sanitizer/glib.supp',
            'MESON_SOURCE_ROOT': meson.current_source_dir(),
            'MESON_PREFIX': get_option('prefix'),
        }
    )
endforeach

# AML tests

if aml_option.allowed()
    testcases = [
        'B450M-MORTAR',
        'H13SSL-N',
        'G513RM',
        'QEMU',
    ]

    acpixtract = find_program('../data/test/acpi/dump.sh')

    src = [
        'acpi/aml.cpp',
        '../src/acpi/aml.cpp',
        '../src/acpi/format.cpp',
        '../src/util/format.cpp',
        '../src/allocator/bitmap.cpp',
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
#     executable('bezos-test-' + name + '.elf', source + [ '../src/boot/hyper.cpp', ],
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
