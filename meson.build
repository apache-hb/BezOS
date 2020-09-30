project('bezos', 'c',
    default_options : [
        'c_std=c99',
        'werror=true',
        'warning_level=3',
        'b_pie=false',
        'b_staticpic=false',
        'debug=false'
    ]
)

src = []
inc = [include_directories('.')]

subdir('kernel')
subdir('boot'/get_option('boot'))
