src = files('src/main.cpp')

pkgviz = executable(
  'pkgviz.elf',
  src,
  dependencies: [pkgtool_dep, argparse, libxml2, quill, openssl, fmt, absl_span],
  install: true,
)
