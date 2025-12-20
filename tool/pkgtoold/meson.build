inc = include_directories('include')

grpc_cpp_plugin = find_program('grpc_cpp_plugin')
protoc = find_program('protoc')
grpc = dependency('grpc')
grpcpp = dependency('grpc++')
grpcpp_ext_reflection = dependency('grpc++_ext_reflection')
protobuf = dependency('protobuf')

generated = custom_target(
    'overlay-proto',
    output: [ '@BASENAME@.pb.cc', '@BASENAME@.pb.h', '@BASENAME@.grpc.pb.cc', '@BASENAME@.grpc.pb.h' ],
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
    depends: [ grpc_cpp_plugin ],
)

grpc_inc = include_directories('.')

src = files('src/pkgtoold.cpp')

liboverlayd = library('overlayd', generated, src,
    include_directories: inc,
    dependencies: [grpc, protobuf],
    install: true,
)

overlayd_dep = declare_dependency(
    link_with: liboverlayd,
    dependencies: [protobuf, grpc],
    include_directories: [inc, grpc_inc],
    sources: [ generated[1], generated[3] ],
)

src = files('src/api.cpp')

libpkgtoold_api = library('pkgtoold-api', src,
    include_directories: inc,
    dependencies: [overlayd_dep, grpc, grpcpp],
    install: true,
)

pkgtoold_api_dep = declare_dependency(
    link_with: libpkgtoold_api,
    include_directories: inc,
)

src = files('src/daemon.cpp')

pkgtoold = executable('pkgtoold', src,
    include_directories: inc,
    dependencies: [overlayd_dep, grpc, grpcpp, grpcpp_ext_reflection, sqlitecpp],
    install: true,
)
