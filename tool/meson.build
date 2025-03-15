project('tools', 'cpp',
    default_options : [
        'cpp_std=gnu++26',
        'default_library=static',
    ]
)

add_global_arguments('-target', 'x86_64-pc-linux-gnu', language : [ 'c', 'cpp' ])

libxml2 = dependency('libxml-2.0')
openssl = dependency('openssl')
curl = dependency('libcurl')
argo = dependency('argo')
argparse = dependency('argparse')
subprocess = dependency('cpp-subprocess')
libarchive = dependency('libarchive')
sqlitecpp = dependency('sqlitecpp')
indicators = dependency('indicators')

executable('package.elf', 'package/main.cpp',
    install : true,
    dependencies : [libxml2, openssl, curl, argo, argparse, subprocess, libarchive, sqlitecpp, indicators]
)

executable('ktest.elf', 'ktest/main.cpp',
    install : true,
    dependencies : [argparse, subprocess]
)
