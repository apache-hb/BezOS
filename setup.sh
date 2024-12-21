builddir=$1
shift
meson setup $builddir --native-file data/x64-clang.ini --cross-file data/kernel.ini --prefix $(pwd)/install $@
