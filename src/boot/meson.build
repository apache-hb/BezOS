linker = meson.current_source_dir()/'link.ld'

foreach asm : [ 'boot', 'prot', 'long' ]
    sources += assemble.process(meson.current_source_dir()/asm+'.asm')
endforeach