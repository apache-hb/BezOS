.PHONY: x86-64

BUILDDIR = build

$(BUILDDIR):
	@ mkdir -p $(BUILDDIR)

x86-64: $(BUILDDIR)
	nasm -fbin arch/x86-64/real.asm -o $(BUILDDIR)/real.bin
	nasm -fbin arch/x86-64/prot.asm -o $(BUILDDIR)/prot.bin
	cat $(BUILDDIR)/real.bin $(BUILDDIR)/prot.bin > bezos.bin