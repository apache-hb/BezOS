.text
.global internal_memory_size
.type internal_memory_size, @function

internal_memory_size:
    # more memory query to register
    movl $0xE820, %eax
    
    xorl %ebx, %ebx

    # query the amount of memory
    int 0x15
    ret