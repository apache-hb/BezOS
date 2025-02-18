project('argo', 'cpp',
    license : 'MIT',
    license_files : [ 'LICENSE' ],
    meson_version : '>=1.1.0',
    default_options : [ 'cpp_std=c++14' ]
)

cpp = meson.get_compiler('cpp')

args = cpp.get_supported_arguments([
    '-D_CRT_SECURE_NO_WARNINGS',
    '-D_CRT_NONSTDC_NO_WARNINGS'
])

src = [
    'json.cpp',
    'utf8.cpp',
    'lexer.cpp',
    'reader.cpp',
    'stream_reader.cpp',
    'fd_reader.cpp',
    'file_reader.cpp',
    'writer.cpp',
    'stream_writer.cpp',
    'fd_writer.cpp',
    'file_writer.cpp',
    'token.cpp',
    'json_exception.cpp',
    'parser.cpp',
    'unparser.cpp',
    'json_io_exception.cpp',
    'json_parser_exception.cpp',
    'json_utf8_exception.cpp',
    'json_array_index_range_exception.cpp',
    'json_pointer_exception.cpp',
    'json_invalid_key_exception.cpp',
    'pointer.cpp',
]

headers = [
    'argo.hpp',
    'common.hpp',
    'documentation.hpp',
    'fd_reader.hpp',
    'fd_writer.hpp',
    'file_reader.hpp',
    'file_writer.hpp',
    'json.hpp',
    'json_array_index_range_exception.hpp',
    'json_exception.hpp',
    'json_invalid_key_exception.hpp',
    'json_io_exception.hpp',
    'json_parser_exception.hpp',
    'json_pointer_exception.hpp',
    'json_utf8_exception.hpp',
    'lexer.hpp',
    'parser.hpp',
    'pointer.hpp',
    'reader.hpp',
    'stream_reader.hpp',
    'stream_writer.hpp',
    'token.hpp',
    'unparser.hpp',
    'utf8.hpp',
    'writer.hpp'
]

inc = include_directories('.')

libargo = library('argo', src,
    cpp_args : args,
    include_directories : inc,
    override_options : [ 'b_sanitize=none' ],
)

argo_dep = declare_dependency(
    include_directories : inc,
    link_with : libargo,
)

meson.override_dependency('argo', argo_dep)
