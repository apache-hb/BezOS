project('uacpi', 'c')

sources = files(
    'source/tables.c',
    'source/types.c',
    'source/uacpi.c',
    'source/utilities.c',
    'source/interpreter.c',
    'source/opcodes.c',
    'source/namespace.c',
    'source/stdlib.c',
    'source/shareable.c',
    'source/opregion.c',
    'source/default_handlers.c',
    'source/io.c',
    'source/notify.c',
    'source/sleep.c',
    'source/registers.c',
    'source/resources.c',
    'source/event.c',
    'source/mutex.c',
    'source/osi.c',
)

includes = include_directories('include')

uacpi_dep = declare_dependency(
    sources : sources,
    include_directories : includes,
)

meson.override_dependency('uacpi', uacpi_dep)
