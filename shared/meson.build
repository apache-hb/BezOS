project('bezos', 'c')

src = [
    'src/syscall_fs.c',
    'src/syscall_threads.c',
    'src/syscall.S',
]

inc = include_directories('include')

libbezos = static_library('bezos-sysapi', src,
    include_directories : inc,
)

sysapi = declare_dependency(
    link_with : libbezos,
    include_directories : inc,
)
