builddir=$1
shift

prefix=$(pwd)/install
builddir=$(pwd)/$builddir

make hyper || exit 1

(cd system; meson setup $builddir/system --cross-file data/bezos.ini --prefix $prefix/system)

(cd kernel; meson setup $builddir/kernel --native-file data/x64-clang.ini --cross-file data/kernel.ini --prefix $prefix/kernel)
