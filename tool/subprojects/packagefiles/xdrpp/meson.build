project('xdrpp', 'c', 'cpp',
    license : 'GPL-3.0-or-later',
)

configure_file(
    output : 'build_config.h',
    configuration : {
        'CPP_COMMAND': '"@0@ -xc -E"'.format(meson.get_compiler('c').get_id()),
        'PACKAGE_VERSION': '"@0@"'.format(meson.project_version()),
    }
)

configure_file(
    input : 'xdrpp/build_endian.h.in',
    output : 'config_build_endian.h',
    configuration : {
        'IS_BIG_ENDIAN': host_machine.endian() == 'big',
    }
)

flex = find_program('flex')
bison = find_program('bison')

lex = generator(flex,
    output : [ '@BASENAME@.cc', '@BASENAME@.hh' ],
    arguments : [
        '--outfile=@OUTPUT0@',
        '--header-file=@OUTPUT1@',
        '--yylineno',
        '@INPUT@'
    ]
)

parse = generator(bison,
    output : [ '@BASENAME@.cc', '@BASENAME@.hh' ],
    arguments : [
        '-d', '@INPUT@', '-v',
        '--output=@OUTPUT0@',
        '--defines=@OUTPUT1@'
    ]
)

xdrc_src = [
    'xdrc/gen_server.cc',
    'xdrc/gen_hh.cc',
    'xdrc/xdrc.cc',
    lex.process('xdrc/scan.ll'),
    parse.process('xdrc/parse.yy'),
]

xdrc_inc = include_directories('.', 'xdrc')

xdrc_exe = executable('xdrc', xdrc_src,
    include_directories : xdrc_inc,
    native : true,
)

meson.override_find_program('xdrc', xdrc_exe)

xdrc_hh = generator(xdrc_exe,
    output : [ 'xdr_@BASENAME@.hh' ],
    arguments : [ '@INPUT@', '-hh', '-o', '@OUTPUT@' ],
)

xdrpp_src = [
    'xdrpp/arpc.cc',
    'xdrpp/iniparse.cc',
    'xdrpp/marshal.cc',
    'xdrpp/msgsock.cc',
    'xdrpp/pollset.cc',
    'xdrpp/printer.cc',
    'xdrpp/rpc_msg.cc',
    'xdrpp/rpcbind.cc',
    'xdrpp/server.cc',
    'xdrpp/socket.cc',
    'xdrpp/socket_unix.cc',
    'xdrpp/srpc.cc',
    xdrc_hh.process(meson.current_source_dir() / 'xdrpp/rpc_msg.x'),
    xdrc_hh.process(meson.current_source_dir() / 'xdrpp/rpcb_prot.x')
]

xdrpp_inc = include_directories('.')

xdrpp_lib = static_library('xdrpp', xdrpp_src,
    include_directories : xdrpp_inc,
    cpp_args : '-Wno-invalid-offsetof',
    install : true,
)

xdrpp_dep = declare_dependency(
    link_with : xdrpp_lib,
    include_directories : xdrpp_inc,
)

meson.override_dependency('xdrpp', xdrpp_dep)
