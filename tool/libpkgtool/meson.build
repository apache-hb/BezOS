src = files(
  'src/download.cpp',
  'src/package.cpp',
  'src/pkgtool.cpp',
  'src/state.cpp',
  'src/tools/autotools.cpp',
  'src/tools/basic.cpp',
  'src/tools/cmake.cpp',
  'src/tools/make.cpp',
  'src/tools/meson.cpp',
  'src/tools/shell.cpp',
  'src/tools/custom.cpp',
  'src/workspace.cpp',
)

inc = include_directories('include')

libpkgtool = library(
  'pkgtool',
  src,
  include_directories: inc,
  dependencies: [
    libxml2,
    pkgtoold_api_dep,
    subprocess,
    sqlitecpp,
    quill,
    curl,
    openssl,
    libarchive,
    fmt,
    absl_span,
    absl_strings,
    absl_base,
  ],
  install: true,
)

pkgtool_dep = declare_dependency(
  link_with: libpkgtool,
  dependencies: [quill],
  include_directories: inc,
)

testcases = {
  'workspace_state_test': {
    'sources': files('test/workspace_state_test.cpp'),
  },
  'workspace_test': {
    'sources': files('test/workspace_test.cpp'),
    'dependencies': [pkgtoold_api_dep],
  },
}

foreach name, kwargs : testcases
  # Meson doesn't have a dictionary merge operator, so we have to do this manually
  kwargs += {
    'dependencies': kwargs.get('dependencies', []) + [pkgtool_dep, gtest_main, fmt, absl_base, absl_strings],
  }

  test_exe = executable(
    name,
    kwargs: kwargs,
    include_directories: inc,
  )

  test(
    name,
    test_exe,
    env: {
      'RESOURCEDIR': meson.current_source_dir() / 'test' / 'resources',
    },
    suite: 'libpkgtool',
    protocol: 'gtest',
  )
endforeach
