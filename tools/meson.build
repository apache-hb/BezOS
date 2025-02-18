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

executable('package.elf', 'package/main.cpp',
    install : true,
    dependencies : [libxml2, openssl, curl, argo, argparse, subprocess, libarchive]
)
