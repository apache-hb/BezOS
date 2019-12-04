.section multiboot
.align 4
.long MULTIBOOT_MAGIC
.long MULTIBOOT_FLAGS
.long MULTIBOOT_CHECKSUM
.skip 20
.long VIDEO_MODE
.long HEIGHT
.long WIDTH
.long 32

.code32
.section bootstrap_text, "ax"
.global _start32
.type _start32, @function
_start32:
    cld

    call enable_sse
    call enable_avx

.code64
    jmp _start64
.size _start32, . - _start32

_start64:
    call kernel_main
.size _start64, . - _start64


.code32

bad_cpu:
    nop 

.section bootstrap_rodata, "a"
badCpuMessage:
    .asciz "An x86_64 cpu is required"

.section bootstrap_bss, "aw", @nobits
.align 4096
.global kernel_pml4
kernel_pm4:
    .skip 4096

## we only support x86_64 so we dont need to check if sse is available
enable_sse:
    mov eax, cr0
    and ax, 0xFFFB
    or ax, 0x2
    mov cr0, eax
    mov eax, cr4
    or ax, 3 << 9
    mov cr4, eax
    ret

## we do need to check for avx support 
enable_avx:
    ## check for avx support
    mov ecx, 0
    cpuid
    and ecx, (1 << 26)
    ## if we dont support avx just keep going
    jz end
yes:
    ## enable avx
    push rax
    push rcx

    xor rcx, rcx
    xgetbv
    or eax, 7
    xsetbv

    pop rcx
    pop rax
end:
    ret
    