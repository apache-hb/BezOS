project('bezos', 'c',
    default_options : [
        'c_std=gnu99',
    ]
)

sources = []
subdir('Source')

executable('bezos', sources,
    c_args : [ '-ffreestanding' ],
    link_args : [ '-ffreestanding', '-nostdlib', '-lgcc' ]
)