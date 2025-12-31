src = files('src/main.cpp')

pkgtool = executable(
  'pkgtool.elf',
  src,
  dependencies: [pkgtool_dep, argparse, libxml2, quill, openssl, fmt, absl_span],
  install: true,
)
