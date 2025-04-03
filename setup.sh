#!/bin/sh

make pkgtool

alias repopkg='install/tool/bin/package.elf --config repo/repo.xml --output build --prefix install --workspace repo.code-workspace --clangd kernel sysapi system runtime'
alias repobld='repopkg --rebuild'
alias repocnf='repopkg --reconfigure'
alias repofetch='repopkg --fetch'
alias repotest='repopkg --test'
alias repoinst='repopkg --reinstall'

alias ktest='install/tool/bin/ktest.elf'
alias kparse='ktest --parse events.bin'
alias ksoak='ktest --image install/image/bezos-limine.iso --output soak'
