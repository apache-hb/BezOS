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