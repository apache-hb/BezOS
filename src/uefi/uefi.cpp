#include "kernel/boot.h"

#include <efi.h>
#include <efilib.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE handle, EFI_SYSTEM_TABLE* table)
{
    (void)handle;
    EFI_STATUS status = table->ConOut->OutputString(table->ConOut, (u16*)L"Hello world\n\r");
    if(EFI_ERROR(status))
        return status;

    status = table->ConIn->Reset(table->ConIn, false);
    if(EFI_ERROR(status))
        return status;

    kmain(nullptr);

    EFI_INPUT_KEY key;
    while((status = table->ConIn->ReadKeyStroke(table->ConIn, &key)) == EFI_NOT_READY);

    return status;
}