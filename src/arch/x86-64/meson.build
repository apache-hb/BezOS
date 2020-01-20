linker = meson.current_source_dir()/'link.ld'

foreach asm : [ 'real', 'boot', 'prot', 'long' ]
    sources += assemble.process(meson.current_source_dir()/asm+'.asm')
endforeach

sources32 += meson.current_source_dir()/'init32.c'