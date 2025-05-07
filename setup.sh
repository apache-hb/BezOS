#!/bin/sh

make pkgtool

alias repopkg='install/tool/bin/package.elf --config repo/repo.xml --target repo/targets/x86_64.xml --output build --prefix install --workspace repo.code-workspace --clangd kernel sysapi system common'
alias repobld='repopkg --rebuild'
alias repocnf='repopkg --reconfigure'
alias repofetch='repopkg --fetch'
alias repotest='repopkg --test'
alias repoinst='repopkg --reinstall'

alias ktest='install/tool/bin/ktest.elf'
alias kparse='ktest --parse events.bin'
alias ksoak='ktest --image install/image/bezos-limine.iso --output soak'

function disfn() {
    # Disassemble a function given an address inside it
    gdb -batch -ex "file $1" -ex "disassemble/rs $2"
}

function funqual() {
    # env PYTHONPATH=$(pwd)/build/sources/llvm-project-src/clang/bindings/python3:$PYTHONPATH \
    #     CLANG_LIBRARY_PATH=$($(pwd)/install/llvm/bin/llvm-config --libdir) \
    $(pwd)/build/sources/funqual/funqual $@ \
        --compile-commands $(pwd)/build/packages/kernel \
        -t $(pwd)/sources/kernel/data/tools/funqual.qtag
}
