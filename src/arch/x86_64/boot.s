#define MULTIBOOT_HEADER_MAGIC 0x1BADB002
#define MULTIBOOT_HEADER_FLAGS 0x00000003
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

#define STACK_SIZE 0x4000

#define KERNEL_LMA 0x100000
#define KERNEL_VMA (0xFFFF800000000000 + KERNEL_LMA)

.section .multiboot
    
    .align 4
    multiboot:
        .long MULTIBOOT_HEADER_MAGIC ## magic
        .long MULTIBOOT_HEADER_FLAGS ## flags
        .long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS) ## checksum

    .align 4
    .global bootstrap
    bootstrap:
    .code32
        cli

        lgdt (init_gdt64_ptr - KERNEL_VMA)

        movl $(init_stack_end - KERNEL_VMA), %esp

        pushl $0
        popf

        xorl %eax, %eax
        movl $(_bss - KERNEL_VMA), %edi
        movl $(_end - KERNEL_VMA), %ecx
        subl %edi, %ecx
        cld
        rep stosb

        movl %cr0, %eax
        andl $0x7fffffff, %eax
        movl %eax, %cr0

        movl %cr4, %eax
        orl $0x20, %eax
        movl %eax, %cr4

        movl $(init_pml4 - KERNEL_VMA), %eax
        mov %eax, %cr3

        movl $0xc0000080, %ecx
        rdmsr

        orl $0x00000101, %eax
        wrmsr

        movl %cr0, %eax
        orl $0x80000000, %eax
        movl %eax, %cr0

        ljmp $0x08, $(boot64 - KERNEL_VMA)

    .code64
    boot64_high:
        movq $KERNEL_VMA, %rax
        addq %rax, %rsp

        movq $0x0, init_pml4
        invlpg 0

        call _kernel

    boot64_hang:
        hlt
        jmp boot64_hang

    boot64:
        movabsq $boot64_high, %rax
        jmp *%rax

    .data
    .align 16
    gdt64:
        .quad 0x0000000000000000
        .quad 0x0020980000000000
    gdt64_end:

    .align 16
    init_gdt64_ptr:
        .word gdt64_end - gdt64 - 1
        .long gdt64 - KERNEL_VMA

    .align 0x1000
    init_pml4:
        .quad init_pdp - KERNEL_VMA + 3
        .fill 510, 8, 0
        .quad init_pdp - KERNEL_VMA + 3

    init_pdp:
        .quad init_pd - KERNEL_VMA + 3
        .fill 509, 8, 0
        .quad init_pd - KERNEL_VMA + 3
        .fill 1, 8, 0

    init_pd:
        .quad 0x0000000000000083
        .quad 0x0000000000200083
        .fill 510, 8, 0

.section .bss, "aw", @nobits

    .align 4096
    init_stack_start:
        .fill 0x1000, 1, 0
    init_stack_end:
