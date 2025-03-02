project('fuzztest', 'cpp')

gmock = dependency('gmock', native : true)
re2 = dependency('re2', native : true)

absl = subproject('abseil-cpp')
absl_include_dir = absl.get_variable('absl_include_dir')

args = [
    '-Wno-sign-compare',
    '-Wno-missing-field-initializers',
    '-Wno-unused-parameter',
    '-Wno-gcc-compat',
    '-Wno-nullability-extension',
    '-Wno-c++20-designator',
    '-Wno-unused-but-set-variable',
    '-DCENTIPEDE_DISABLE_RIEGELI=1',
]

add_project_arguments(args, language : 'cpp', native : true)

absl_base_lib = static_library(
    'absl_base',
    absl.get_variable('absl_base_sources'),
    include_directories: absl_include_dir,
    cpp_args: absl.get_variable('arch_flags'),
    dependencies: [dependency('threads')],
    native : true,
)

absl_base_dep = declare_dependency(
    include_directories: absl_include_dir,
    link_with: absl_base_lib,
)

src = [
    'fuzztest/fuzztest_gtest_main.cc',
    'fuzztest/fuzztest_macros.cc',
    'fuzztest/init_fuzztest.cc',
    'fuzztest/llvm_fuzzer_main.cc',
    'fuzztest/llvm_fuzzer_wrapper.cc',

    'fuzztest/internal/centipede_adaptor.cc',
    'fuzztest/internal/compatibility_mode.cc',
    'fuzztest/internal/configuration.cc',
    'fuzztest/internal/corpus_database.cc',
    'fuzztest/internal/coverage.cc',
    'fuzztest/internal/fixture_driver.cc',
    'fuzztest/internal/googletest_adaptor.cc',
    'fuzztest/internal/io.cc',
    'fuzztest/internal/logging.cc',
    'fuzztest/internal/registry.cc',
    'fuzztest/internal/runtime.cc',
    'fuzztest/internal/seed_seq.cc',
    'fuzztest/internal/serialization.cc',
    'fuzztest/internal/status.cc',
    'fuzztest/internal/subprocess.cc',
    'fuzztest/internal/type_support.cc',
]

deps = [ absl_base_dep, gmock, re2 ]

libfuzztest = static_library('fuzztest', src,
    dependencies : deps,
    native : true
)

fuzztest_dep = declare_dependency(
    link_with : libfuzztest,
    include_directories : '.',
    dependencies : deps,
)

meson.override_dependency('fuzztest', fuzztest_dep)
