project('fmt', 'cpp', version: '11.0.2', license: 'MIT WITH fmt-exception', default_options: ['cpp_std=c++14'])

fmt_private_cpp_args = []
fmt_interface_cpp_args = []
if get_option('default_library') == 'shared'
  fmt_private_cpp_args += ['-DFMT_LIB_EXPORT']
  fmt_interface_cpp_args += ['-DFMT_SHARED']
endif

fmt_lib = library(
  'fmt',
  'src/format.cc',
  'src/os.cc',
  cpp_args: fmt_private_cpp_args,
  include_directories: 'include',
  build_by_default: false,
)

install_subdir('include',
  strip_directory: true,
  install_dir: get_option('includedir'),
)

fmt_dep = declare_dependency(
  include_directories: 'include',
  compile_args: fmt_interface_cpp_args,
  link_with: fmt_lib,
)

fmt_header_only_dep = declare_dependency(
  include_directories: 'include',
  compile_args: '-DFMT_HEADER_ONLY',
)

if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('fmt', fmt_dep)
  meson.override_dependency('fmt_header_only', fmt_header_only_dep)
endif

pkg_mod = import('pkgconfig')
pkg_mod.generate(fmt_lib,
  name: 'fmt',
  filebase: 'fmt',
)
