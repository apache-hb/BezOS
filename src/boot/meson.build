sources += assemble.process('boot.asm')
ldscript = meson.current_source_dir()/'link.ld'

boot = static_library('boot', 'boot.c',
    c_args : [ '-m32' ]
)

bootloader = custom_target('bootload',
    input : boot,
    output : 'boot32.o',
    command : [ 'objcopy', '@INPUT@', '@OUTPUT@', '-O', 'elf64-x86-64',
        '--rename-section', '.text=.text32',
        '--rename-section', '.data=.data32',
        '--rename-section', '.rodata=.rodata32',
        '--rename-section', '.bss=.bss32'
    ]
)

sources += bootloader