# pkg-config support for GMock is in the master
# branch of the upstream repository.
# In order to be consistent with the upstream
# naming and the library conventions, the
# following changes have been made:
#
#   gmock_mainless_dep -> gmock_dep
#   gmock_with_main_dep -> gmock_main_dep
#
# Both GMock dependencies now include GTest,
# as this mimicks the same behavior as the
# upstream repo.

gmock_incdir = include_directories('include', '.', is_system : true)
gmock_libsources = files('src/gmock-all.cc')
gmock_mainsources = files('src/gmock_main.cc')

deps = [ dependency('threads') ]

libgmock = static_library('gmock', gmock_libsources,
    include_directories : [gtest_incdir, gmock_incdir],
    dependencies : deps,
    build_by_default : false,
    native : true,
)

libgmockmain = static_library('gmock_main', gmock_mainsources,
    include_directories : [gtest_incdir, gmock_incdir],
    dependencies : deps,
    build_by_default : false,
    native : true,
)

gmock_dep = declare_dependency(
    include_directories : [gtest_incdir, gmock_incdir],
    objects : libgmock.extract_objects(gmock_libsources),
    dependencies : deps,
)

gmock_main_dep = declare_dependency(
    include_directories : [gtest_incdir, gmock_incdir],
    objects : libgmockmain.extract_objects(gmock_mainsources),
    dependencies : [ deps, gmock_dep ],
)
