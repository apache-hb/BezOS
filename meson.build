project('bezos', 'cpp',
    default_options : [
        'cpp_std=c++17',
        'werror=true',
        'warning_level=3',
        'b_pie=false',
        'b_staticpic=false',
        'debug=false',
        'cpp_eh=none',
        'cpp_rtti=false'
    ]
)

src = []
inc = []
args = []

if get_option('buildtype').startswith('debug')
    args += '-DDEBUG=1'
endif

subdir('kernel')
subdir('boot'/get_option('boot'))
