
pvtest_testcases = {
    'basic': {
        'sources': files('basic.cpp')
    },
    'asm': {
        'sources': files('asm.cpp'),
        'c_args': [ ],
        'cpp_args': [ ],
        'link_args' : [ ]
    },
    'mmu': {
        'sources': files(
            'mmu.cpp',

            '../../src/util/memory.cpp',

            '../../src/memory/pte.cpp',
            '../../src/memory/table_allocator.cpp',
            '../../src/memory/memory.cpp',
            '../../src/memory/detail/table_list.cpp',
            '../../src/memory/detail/control_block.cpp',

            # TODO: eventually this needs to be removed, pvtest lets the kernel
            #       use real implementations rather than the shim.
            '../../test/test_shim.cpp',
        ),
        'c_args': [ ],
        'cpp_args': [ ],
        'link_args' : [ ]
    },
    # 'idt': {
    #     'sources': files(
    #         'test/idt.cpp',
    #         'test/idt.S'
    #     )
    # }
}

foreach name, setup : pvtest_testcases
    exe = executable('pvtest-' + name.strip().replace(' ', '-'), setup['sources'],
        c_args : setup.get('c_args', [ '-fsanitize=nullability' ]),
        cpp_args : setup.get('cpp_args', [ '-fsanitize=nullability' ]) + pvtest_args,
        link_args : setup.get('link_args', [ '-fsanitize=nullability' ]),
        dependencies : setup.get('dependencies', []) + [ pvtest, gtest_main ],
        native : true,
        pie : true,
    )

    test('pvtest ' + name.strip(), exe,
        suite : 'pvtest',
        protocol : 'gtest',
        timeout : 5,
    )
endforeach
