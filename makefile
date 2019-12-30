.PHONY: x86-64 clean

BUILDDIR = build

$(BUILDDIR):
	@ mkdir -p $(BUILDDIR)

x86-64: $(BUILDDIR)
	@# build assembly bits
	@nasm -felf64 arch/x86-64/real.asm -o $(BUILDDIR)/real.o
	@nasm -felf64 arch/x86-64/prot.asm -o $(BUILDDIR)/prot.o
	@clang++ --target=x86_64-pc-none-elf64 -m32 -Qn -fno-addrsig -std=c++17 -fno-rtti -fno-exceptions -ffreestanding -fno-builtin -nostdlib -nostdinc -static -I$(shell pwd) -c arch/x86-64/tables.cpp -o tables.o
	@objcopy -O elf64-x86-64 tables.o build/tables.o


	@clang++ --target=x86_64-pc-none-elf -m64 -fno-addrsig -std=c++17 -fno-rtti -fno-exceptions -ffreestanding -fno-builtin -nostdlib -nostdinc -static -I$(shell pwd) -c kernel/kernel.cpp -o build/kernel.o

	@ld -o bezos.bin $(wildcard $(BUILDDIR)/*.o) -Tarch/x86-64/link.ld --oformat binary

clean:
	@ rm -rf $(BUILDDIR)
	@ rm bezos.bin