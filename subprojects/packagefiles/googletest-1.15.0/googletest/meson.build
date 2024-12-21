# pkg-config support for GTest is in the master
# branch of the upstream repository.
# In order to be consistent with the upstream
# naming and the library conventions, the
# following changes have been made:
#
#   gtest_mainless_dep -> gtest_dep
#   gtest_with_main_dep -> gtest_main_dep

gtest_incdir = include_directories('include', '.', is_system : true)
gtest_libsources = files('src/gtest-all.cc')
gtest_mainsources = files('src/gtest_main.cc')

deps = [ dependency('threads') ]

libgtest = static_library('gtest', gtest_libsources,
    include_directories : gtest_incdir,
    dependencies : deps,
    build_by_default : false,
    native : true,
)

gtest_dep = declare_dependency(
    objects : libgtest.extract_objects(gtest_libsources),
    include_directories : gtest_incdir,
    dependencies : deps,
)

libgtestmain = static_library('gtest_main', gtest_mainsources,
    include_directories : gtest_incdir,
    dependencies : deps,
    build_by_default : false,
    native : true,
)

gtest_main_dep = declare_dependency(
    objects : libgtestmain.extract_objects(gtest_mainsources),
    include_directories : gtest_incdir,
    dependencies : [ deps, gtest_dep ],
)
