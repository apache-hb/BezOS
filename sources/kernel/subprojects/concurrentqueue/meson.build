project(
    # moodycamel::ConcurrentQueue
    'concurrentqueue',
    ['cpp'],
    # Dual licensing
    license: ['symplified-BSD', 'Boost 1.0'],
    license_files : ['LICENSE.md'],
    version: '1.0.3',
    meson_version : '>=1.1.0',
)

concurrentqueue_dep = declare_dependency(

    # it should be better if it was in a subdirectory
    # like moodycamel/ConcurrentQueue
    include_directories: [include_directories('.')],
)

meson.override_dependency('concurrentqueue', concurrentqueue_dep)
