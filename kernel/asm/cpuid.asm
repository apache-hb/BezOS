section .text
bits 32


global has_cpuid:function (has_cpuid.end - has_cpuid)
has_cpuid:
    pushfd
    pushfd
    xor dword [esp], 0x00200000
    popfd
    pushfd
    pop eax
    xor eax, [esp]
    popfd
    and eax, 0x00200000
    ret
.end: