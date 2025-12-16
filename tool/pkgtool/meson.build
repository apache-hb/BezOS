src = files('src/main.cpp')

pkgtool = executable('pkgtool.elf', src,
    dependencies: [pkgtool_dep, argparse, libxml2],
    install: true,
)
