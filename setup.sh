builddir=$1
shift

prefix=$(pwd)/install
builddir=$(pwd)/$builddir

(cd tools; meson setup $builddir/tools --prefix $prefix/tools $@)

make tools || exit 1

$prefix/tools/bin/package.elf --config repo/repo.xml --build build --prefix install --workspace repo.code-workspace
