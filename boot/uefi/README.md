# XBTOS UEFI 引导程序

这个目录提供 UEFI x86_64 引导程序骨架，入口文件为 `bootx64.c`，目标产物为 `EFI/BOOT/BOOTX64.EFI`。

当前内核仍是 32 位 Multiboot/ELF 内核。UEFI 固件默认运行在 64 位长模式下，不能像 BIOS Multiboot 那样直接跳入现有 32 位入口。因此这里先提供可编译的 UEFI loader 骨架和中文启动提示，后续可继续扩展为：

- 加载并解析 `XBTOS.elf`
- 建立 GDT/页表并切换到兼容的内核执行模式
- 传递 framebuffer、内存映射、ACPI 等启动信息
- 跳转到新的 UEFI 内核入口

推荐后续把内核升级为 x86_64 freestanding 入口，这样 UEFI 支持会更干净。
