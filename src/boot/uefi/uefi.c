#include <efi/boot-services.h>
#include <efi/runtime-services.h>
#include <efi/system-table.h>
#include <efi/types.h>

#define TRY(...) { efi_status err = __VA_ARGS__; if(err != EFI_SUCCESS) return err; }

efi_status efi_main(efi_handle handle, efi_system_table* table)
{
    (void)handle;

    TRY(table->BootServices->SetWatchdogTimer(0, 0, 0, NULL));

    TRY(table->ConOut->ClearScreen(table->ConOut));

    TRY(table->ConOut->OutputString(table->ConOut, (char16_t*)L"Hello world\n\r"));

    TRY(table->ConIn->Reset(table->ConIn, false));

    efi_input_key key;
    while(table->ConIn->ReadKeyStroke(table->ConIn, &key) == EFI_NOT_READY);

    return EFI_SUCCESS;
}
