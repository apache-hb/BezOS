src = files('src/workspace.cpp', 'src/package.cpp')

inc = include_directories('include')

libpkgtool = library('pkgtool', src,
    include_directories: inc,
    dependencies: [libxml2],
    install: true,
)

pkgtool_dep = declare_dependency(
    link_with: libpkgtool,
    include_directories: inc,
)
