foreach src : ['real', 'prot', 'paging']
    sources += assemble.process(meson.current_source_dir()/src+'.asm')
endforeach

linker = meson.current_source_dir()/'link.ld'