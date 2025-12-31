project(
  'tools',
  'cpp',
  default_options: [
    'cpp_std=c++23',
    'default_library=static',
    'force_fallback_for=protoc,grpc_cpp_plugin,absl_base',
  ],
)

cpp = meson.get_compiler('cpp')

absl_span = dependency('absl_span')
absl_strings = dependency('absl_strings')
absl_base = dependency('absl_base')
gtest_main = dependency('gtest_main')
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
quill = dependency('quill')
fmt = dependency('fmt')
tl_expected = dependency('tl-expected')

subdir('pkgtoold')
subdir('libpkgtool')
subdir('pkgtool')

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

configure_file(
  input: 'resources/systemd-pkgtoold-install.sh.in',
  output: 'systemd-pkgtoold-install.sh',
  configuration: {
    'MESON_INSTALL_PREFIX': get_option('prefix'),
  },
  install: true,
  install_dir: get_option('prefix') / 'bin',
)

src = [
  'package/main.cpp',
]

executable(
  'package.elf',
  src,
  install: true,
  dependencies: [
    libxml2,
    openssl,
    curl,
    argo,
    argparse,
    subprocess,
    libarchive,
    sqlitecpp,
    indicators,
    glob,
    fmt,
    absl_span,
    absl_strings,
    absl_base,
  ],
)

executable(
  'ktest.elf',
  'ktest/main.cpp',
  install: true,
  dependencies: [argparse, subprocess, absl_base, fmt],
)
