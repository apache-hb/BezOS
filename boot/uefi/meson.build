src += [ 'uefi.c' ]

kernel = executable('bezos.efi', src, 
    c_args : [ '-target x86_64-pc-win32-coff' ],
    link_args : [ '-subsystem:efi_application', '-nodefaultlib', '-dll', '-entry:efi_main' ],
    include_directories : inc
)
