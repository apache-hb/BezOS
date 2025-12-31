v1alpha_reflection = custom_target('v1alpha_reflection_proto',
  command: [
    protoc_cc,
    '--proto_path', meson.current_source_dir(),
    '--grpc_out=@OUTDIR@',
    '--cpp_out=@OUTDIR@',
    '--plugin=protoc-gen-grpc=' + grpc_cpp_plugin.full_path(),
    'reflection.proto',
  ],
  input: 'reflection.proto',
  output: ['reflection.pb.cc', 'reflection.pb.h', 'reflection.grpc.pb.cc', 'reflection.grpc.pb.h'],
  depends: [ grpc_cpp_plugin ],
  build_by_default: true,
)
