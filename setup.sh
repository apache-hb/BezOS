builddir=$1
shift

prefix=$(pwd)/install

if [ "$builddir" = "release" ]; then
    prefix=$(pwd)/install-release
fi

make hyper || exit 1

meson setup $builddir \
    --native-file data/x64-clang.ini \
    --cross-file data/kernel.ini \
    --prefix $prefix $@
