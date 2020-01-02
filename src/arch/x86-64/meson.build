linker = meson.current_source_dir()/'link.ld'

foreach asm : [ 'boot', 'prot' ]
    sources += assemble.process(meson.current_source_dir()/asm+'.asm')
endforeach

sources32 += meson.current_source_dir()/'paging.cpp'