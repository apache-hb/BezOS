project('bezos-common', 'cpp',
    default_options : [
        'cpp_std=c++26',
        'c_std=c23',
        'warning_level=3',
    ],
    version : '0.1.0',
    license : 'GPL-3.0-only',
    meson_version : '>=1.1.0',
)

inc = include_directories('include')

common_dep = declare_dependency(
    include_directories : inc,
)

if meson.is_subproject()
    subdir_done()
endif

install_subdir('include/common', install_dir : 'common', install_tag : 'headers')

gtest = dependency('gtest_main', native : true)

tests = {
    'memory range': [
        'test/memory_range.cpp'
    ],
    'compiler': [
        'test/compiler.cpp'
    ],
}

test_exe_kwargs = {
    'include_directories': inc,
    'native': true,
    'dependencies': [ gtest ],
}

test_kwargs = {
    'protocol': 'gtest',
    'timeout': 5
}

foreach name, sources : tests
    test_exe = executable('test-' + name, sources, kwargs : test_exe_kwargs)
    test(name, test_exe, kwargs : test_kwargs)
endforeach
