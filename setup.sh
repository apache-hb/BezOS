meson setup $1 --cross-file data/x64-clang.ini --cross-file data/kernel.ini --prefix $(pwd)/install
meson setup $1-test --native-file data/x64-clang.ini -Db_coverage=true
