foreach src : [ 'kernel.c', 'vga/vga.c', 'mm/memory.c', 'mm/paging.c', 'utils/cpuid.c', 'utils/stdlib.c' ]
    sources += meson.current_source_dir()/src
endforeach

sources += assemble.process(meson.current_source_dir()/'hw/registers.asm')

incs += include_directories('.')