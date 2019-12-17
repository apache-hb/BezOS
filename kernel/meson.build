foreach f : [ 'kernel.cpp', 'vga/vga.cpp' ]
    sources += meson.current_source_dir()/f
endforeach

incs += include_directories('.')

subdir('arch'/meson.get_cross_property('arch'))