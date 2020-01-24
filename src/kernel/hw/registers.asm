bits 64
section .text
    global cr0_get
    cr0_get:
        mov rax, cr0
        ret

    global cr2_get
    cr2_get:
        mov rax, cr2
        ret

    global cr3_get
    cr3_get:
        mov rax, cr3
        ret

    global cr4_get
    cr4_get:
        mov rax, cr4
        ret

    global cr0_set
    cr0_set:
        pop rax
        mov cr0, rax
        ret

    global cr2_set
    cr2_set:
        pop rax
        mov cr2, rax
        ret

    global cr3_set
    cr3_set:
        pop rax
        mov cr3, rax
        ret

    global cr4_set
    cr4_set:
        pop rax
        mov cr4, rax
        ret