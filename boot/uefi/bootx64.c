#include <stdint.h>
#include <stddef.h>

typedef uint64_t EFI_STATUS;
typedef void* EFI_HANDLE;
typedef uint16_t CHAR16;

#define EFI_SUCCESS 0
#define EFI_ERROR(status) ((status) != EFI_SUCCESS)
#define EFI_LOAD_ERROR 0x8000000000000001ULL

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef struct EFI_BOOT_SERVICES EFI_BOOT_SERVICES;
typedef struct EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;

typedef EFI_STATUS (*EFI_TEXT_STRING)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* self,
    CHAR16* string
);

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void* reset;
    EFI_TEXT_STRING output_string;
};

typedef EFI_STATUS (*EFI_ALLOCATE_POOL)(
    uint32_t pool_type,
    uint64_t size,
    void** buffer
);

struct EFI_BOOT_SERVICES {
    char reserved0[24];
    void* raise_tpl;
    void* restore_tpl;
    EFI_ALLOCATE_POOL allocate_pool;
};

struct EFI_SYSTEM_TABLE {
    char reserved0[64];
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* con_out;
    char reserved1[24];
    EFI_BOOT_SERVICES* boot_services;
};

static void uefi_print(EFI_SYSTEM_TABLE* system_table, CHAR16* text) {
    system_table->con_out->output_string(system_table->con_out, text);
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
    (void)image_handle;

    uefi_print(system_table, L"XBTOS UEFI 引导程序\r\n");
    uefi_print(system_table, L"当前版本已进入 UEFI 环境。\r\n");
    uefi_print(system_table, L"下一阶段需要 x86_64 内核入口或 ELF32 兼容跳板。\r\n");
    uefi_print(system_table, L"请使用 scripts\\build_iso_zh.ps1 生成 EFI/BOOT/BOOTX64.EFI。\r\n");

    return EFI_LOAD_ERROR;
}
