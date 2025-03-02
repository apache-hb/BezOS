project('re2', 'cpp')

absl = subproject('abseil-cpp')
absl_include_dir = absl.get_variable('absl_include_dir')

args = [
    '-Wno-c99-extensions',
    '-Wno-gcc-compat'
]

src = [
    're2/bitmap256.cc',
    're2/bitstate.cc',
    're2/compile.cc',
    're2/dfa.cc',
    're2/filtered_re2.cc',
    're2/mimics_pcre.cc',
    're2/nfa.cc',
    're2/onepass.cc',
    're2/parse.cc',
    're2/perl_groups.cc',
    're2/prefilter.cc',
    're2/prefilter_tree.cc',
    're2/prog.cc',
    're2/re2.cc',
    're2/regexp.cc',
    're2/set.cc',
    're2/simplify.cc',
    're2/tostring.cc',
    're2/unicode_casefold.cc',
    're2/unicode_groups.cc',
]

inc = include_directories('re2')

libre2 = static_library('re2', src,
    include_directories : [ inc, absl_include_dir ],
    dependencies : [dependency('threads')],
    native : true,
)

re2_dep = declare_dependency(
    include_directories : [ '.', absl_include_dir ],
    link_with : libre2,
)

meson.override_dependency('re2', re2_dep)
