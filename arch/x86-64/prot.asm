; protected mode assembly
; we are in the page after 0x7C00 so make sure the offset is right
org 0x7E00
; we're in protected mode now, so we can use 32 bit instructions
bits 32

; 32 bit start
start:
    