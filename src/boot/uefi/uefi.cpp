#include <efi/boot-services.h>
#include <efi/runtime-services.h>
#include <efi/system-table.h>
#include <efi/types.h>

#include "boot.h"

#define TRY(...) if(efi_status err = __VA_ARGS__; err != EFI_SUCCESS) return err;
#define EXPECT(expr, ret) if(auto res = expr; res != ret) return err;

efi_status EFIAPI efi_main(efi_handle handle, efi_system_table* table)
{
    (void)handle;

    TRY(table->BootServices->SetWatchdogTimer(0, 0, 0, NULL));

    TRY(table->ConOut->ClearScreen(table->ConOut));

    UINTN map_size = 0;
    UINTN key = 0;
    UINTN desc_size = 0;
    UINT32 version;
    efi_memory_v
    efi_memory_descriptor* map = nullptr;

    EXPECT(table->BootServices->GetMemoryMap(5, &map_size, &map, nullptr, &desc_size, nullptr), EFI_BUFFER_TOO_SMALL);

    return EFI_SUCCESS;
}
