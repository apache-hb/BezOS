src = files(
    'src/workspace.cpp',
    'src/package.cpp',
    'src/pkgtool.cpp',
    'src/state.cpp',
    'src/tools/basic.cpp',
    'src/tools/autotools.cpp',
    'src/tools/make.cpp',
    'src/tools/shell.cpp',
    'src/tools/cmake.cpp',
    'src/tools/meson.cpp',
)

inc = include_directories('include')

libpkgtool = library('pkgtool', src,
    include_directories: inc,
    dependencies: [libxml2, pkgtoold_api_dep, subprocess, sqlitecpp],
    install: true,
)

pkgtool_dep = declare_dependency(
    link_with: libpkgtool,
    include_directories: inc,
)

workspace_state_test = executable('workspace_state_test',
    'test/workspace_state_test.cpp',
    include_directories: inc,
    dependencies: [pkgtool_dep, gtest_main],
)

test('workspace_state_test', workspace_state_test, suite: 'libpkgtool')
