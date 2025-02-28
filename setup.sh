#!/bin/sh

make pkgtool

alias repopkg='install/tool/bin/package.elf --config repo/repo.xml --output build --prefix install --workspace repo.code-workspace --clangd kernel sysapi system'
alias repobld='repopkg --rebuild'
alias repocnf='repopkg --reconfigure'
alias repofetch='repopkg --fetch'
alias repotest='repopkg --test'
alias repoinst='repopkg --reinstall'
