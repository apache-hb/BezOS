project('tools', 'cpp',
    default_options : [
        'cpp_std=gnu++26',
        'default_library=static',
    ]
)

libxml2 = dependency('libxml-2.0')
openssl = dependency('openssl')
curl = dependency('libcurl')
argo = dependency('argo')
argparse = dependency('argparse')
subprocess = dependency('cpp-subprocess')
libarchive = dependency('libarchive')
sqlitecpp = dependency('sqlitecpp')
indicators = dependency('indicators')
glob = dependency('glob')
xdrc = find_program('xdrc')
xdrpp = dependency('xdrpp')

subdir('libpkgtool')
subdir('pkgtool')

overlayfs_hh = custom_target('overlayfs_hh',
    input : 'package/overlay.x',
    output : 'overlayfs.hh',
    command : [xdrc, '@INPUT@', '-hh', '-o', '@OUTPUT@'],
)

src = [
    'package/main.cpp',
    overlayfs_hh
]

executable('package.elf', src,
    install : true,
    dependencies : [libxml2, openssl, curl, argo, argparse, subprocess, libarchive, sqlitecpp, indicators, glob]
)

executable('ktest.elf', 'ktest/main.cpp',
    install : true,
    dependencies : [argparse, subprocess]
)
