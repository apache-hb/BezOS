project('tools', 'cpp',
    default_options : [
        'cpp_std=gnu++26',
        'default_library=static',
        'force_fallback_for=protoc,grpc_cpp_plugin,absl_base',
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

subdir('libpkgtool')
subdir('pkgtool')
subdir('pkgtoold')

configure_file(
    input: 'resources/pkgtoold.service.in',
    output: 'pkgtoold.service',
    configuration: {
        'MESON_INSTALL_PREFIX': get_option('prefix'),
    },
    install: true,
    install_dir: get_option('prefix') / 'lib/systemd/system',
)

configure_file(
    input: 'resources/pkgtoold.json.in',
    output: 'pkgtoold.json',
    configuration: {
        'REPO': meson.project_source_root() / '..',
    },
    install: true,
    install_dir: get_option('prefix') / 'etc',
)

meson.add_install_script('resources/systemd-pkgtoold-install.sh')

src = [
    'package/main.cpp',
]

executable('package.elf', src,
    install : true,
    dependencies : [libxml2, openssl, curl, argo, argparse, subprocess, libarchive, sqlitecpp, indicators, glob]
)

executable('ktest.elf', 'ktest/main.cpp',
    install : true,
    dependencies : [argparse, subprocess]
)
