.PHONY: x86-64 clean

BUILDDIR = build

$(BUILDDIR):
	@ mkdir -p $(BUILDDIR)

x86-64: $(BUILDDIR) arch/x86-64/real.asm arch/x86-64/prot.asm arch/x86-64/link.ld kernel/kernel.c
	@# build assembly bits
	@nasm -felf64 arch/x86-64/real.asm -o $(BUILDDIR)/real.o
	@nasm -felf64 arch/x86-64/prot.asm -o $(BUILDDIR)/prot.o

	@ clang --target=x86_64-pc-none-elf -march=native -ffreestanding -fno-builtin -nostdlib -nostdinc -static -c kernel/kernel.c -o build/kernel.o

	@ld -o bezos.bin $(wildcard $(BUILDDIR)/*.o) -Tarch/x86-64/link.ld --oformat binary

clean:
	@ rm -rf $(BUILDDIR)
	@ rm bezos.bin