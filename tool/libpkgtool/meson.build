src = files('src/workspace.cpp', 'src/package.cpp', 'src/pkgtool.cpp')

inc = include_directories('include')

libpkgtool = library('pkgtool', src,
    include_directories: inc,
    dependencies: [libxml2, pkgtoold_api_dep],
    install: true,
)

pkgtool_dep = declare_dependency(
    link_with: libpkgtool,
    include_directories: inc,
)
