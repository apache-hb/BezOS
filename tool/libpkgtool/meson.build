src = files(
  'src/package.cpp',
  'src/pkgtool.cpp',
  'src/state.cpp',
  'src/download.cpp',
  'src/tools/autotools.cpp',
  'src/tools/basic.cpp',
  'src/tools/cmake.cpp',
  'src/tools/make.cpp',
  'src/tools/meson.cpp',
  'src/tools/shell.cpp',
  'src/workspace.cpp',
)

inc = include_directories('include')

libpkgtool = library(
  'pkgtool',
  src,
  include_directories: inc,
  dependencies: [libxml2, pkgtoold_api_dep, subprocess, sqlitecpp, quill, curl, openssl],
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
  },
}

foreach name, kwargs : testcases
  test_exe = executable(
    name,
    kwargs: kwargs,
    include_directories: inc,
    dependencies: [pkgtool_dep, gtest_main],
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
