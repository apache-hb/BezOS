x86-64:
	nasm -fbin arch/x86-64/real.asm -o real.bin
	nasm -fbin arch/x86-64/prot.asm -o prot.bin
	cat real.bin prot.bin > bezos.bin