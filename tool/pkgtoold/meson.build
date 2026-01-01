inc = include_directories('include')

grpc_cpp_plugin = find_program('grpc_cpp_plugin', native: true)
protoc = find_program('protoc', native: true)
grpc = dependency('grpc')
grpcpp = dependency('grpc++')
grpcpp_reflection = dependency('grpc++_reflection')
protobuf = dependency('protobuf')

generated = custom_target(
  'overlay-proto',
  output: [
    '@BASENAME@.pb.cc',
    '@BASENAME@.pb.h',
    '@BASENAME@.grpc.pb.cc',
    '@BASENAME@.grpc.pb.h',
  ],
  input: 'protos/overlay.proto',
  command: [
    protoc,
    '--proto_path=@CURRENT_SOURCE_DIR@/protos',
    '--proto_path=@BUILD_ROOT@',
    '--grpc_out=@OUTDIR@',
    '--cpp_out=@OUTDIR@',
    '--plugin=protoc-gen-grpc=' + grpc_cpp_plugin.full_path(),
    '@INPUT@',
  ],
  depends: [grpc_cpp_plugin],
)

grpc_inc = include_directories('.')

src = files('src/pkgtoold.cpp', 'src/proc_mounts.cpp')

liboverlayd = library(
  'overlayd',
  generated,
  src,
  include_directories: inc,
  dependencies: [grpc, protobuf, fmt],
  cpp_args: cpp.get_supported_arguments('-Wno-inconsistent-missing-override'),
  install: true,
)

overlayd_dep = declare_dependency(
  link_with: liboverlayd,
  dependencies: [protobuf, grpc],
  include_directories: [inc, grpc_inc],
  sources: [generated[1], generated[3]],
)

src = files('src/api.cpp')

libpkgtoold_api = library(
  'pkgtoold-api',
  src,
  include_directories: inc,
  dependencies: [overlayd_dep, grpc, grpcpp, fmt, absl_span],
  install: true,
)

pkgtoold_api_dep = declare_dependency(
  link_with: libpkgtoold_api,
  include_directories: inc,
)

src = files('src/daemon.cpp')

pkgtoold = executable(
  'pkgtoold',
  src,
  include_directories: inc,
  dependencies: [
    overlayd_dep,
    grpc,
    grpcpp,
    grpcpp_reflection,
    sqlitecpp,
    quill,
    fmt,
    absl_span,
    absl_strings,
    absl_base,
    tl_expected,
  ],
  install: true,
)

testcases = {
  'proc_mounts_test': {
    'sources': files('test/proc_mounts_test.cpp'),
  },
}

foreach name, kwargs : testcases
  # Meson doesn't have a dictionary merge operator, so we have to do this manually
  kwargs += {
    'dependencies': kwargs.get('dependencies', []) + [pkgtoold_api_dep, gtest_main],
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
    suite: 'libpkgtoold',
    protocol: 'gtest',
  )
endforeach
