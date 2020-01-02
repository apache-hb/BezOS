extern thing

section .protected
bits 32
    global start32
    start32:
        call thing
        jmp $