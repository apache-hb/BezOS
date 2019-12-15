project('bezos', ['c', 'cpp'],
    default_options : [
        'cpp_std=c++17',
        'c_std=gnu99',
        'cpp_eh=none',
        'cpp_rtti=false'
    ],
    version : '0.0.1'
)

sources = []
incs = []

links = []

args = []

warns = [
    '-Wreorder',
    '-Wno-unknown-pragmas',
    '-Wduplicated-cond',
    '-Wduplicated-branches',
    '-Wlogical-op',
    '-Werrror',
    '-Wall',
    '-Wextra'
]

common = [
    '-m64',
    '-march=x86_64',
    '-fno-PIC',
    '-ffreestanding',
    '-mno-red-zone',
    '-nostdlib',
    '-mcmodel=kernel',
    '-fno-stack-protector',
    '-fno-omit-frame-pointer',
    '-mno-sse3',
    '-mno-sse4',
    '-mno-sse4',
    '-mno-sse4.1',
    '-mno-sse4.2',
    '-mno-sse4a'
]

link = 'kernel/asm/link.ld'

link_flags = [
    '-T', link, 
    '-nostdlib',
    '-mcmodel=kernel',
    '-fno-PIC',
    '-no-pie',
    '-Wl,--build-id=none',
    '-Wl,-z,max-page-size=0x1000,-n',
    '-lgcc'
]

subdir('kernel')

kernel = executable('bezos.elf', sources,
    include_directories : incs,
    cpp_args : [warns, args],
    c_args : [warns, args],
    link_args : [link_flags],
    link_with : links
)