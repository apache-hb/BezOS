builddir=$1
shift

prefix=$(pwd)/install
builddir=$(pwd)/$builddir

(cd tools; meson setup $builddir/tools --prefix $prefix/tools) > /dev/null || exit 1

make tools > /dev/null || exit 1

$prefix/tools/bin/package.elf --config repo/repo.xml --output build --prefix install --workspace repo.code-workspace --clangd $@
