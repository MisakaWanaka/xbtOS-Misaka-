#include <types.h>
#include <string.h>
#include "app/app.h"
#include "fs/vfs.h"
#include "hal/console.h"
#include "hal/hal.h"
#include "i18n/i18n.h"
#include "memory/heap.h"
#include "memory/physmem.h"

static FileSystem vfs;

static int demo_app_main(app::AppContext* context) {
    hal::print_string("Hello from sandboxed app.\n");
    hal::print_string("App ID: ");
    hal::print_string(context->manifest->identifier);
    hal::print_string("\nContainer: ");
    hal::print_string(context->sandbox.container_path);
    hal::print_string("\n");
    return 0;
}

static void register_builtin_apps() {
    app::AppManifest demo = {
        "com.xbtos.demo",
        "XBTOS Demo",
        "1.0",
        app::APP_PERMISSION_CONSOLE | app::APP_PERMISSION_FILE_READ
    };

    app::manager().register_app(&demo, demo_app_main);
}

#define BUFFER_SIZE 256
#define HISTORY_SIZE 10

static char input_buffer[BUFFER_SIZE];
static int buffer_index = 0;
static bool shift_pressed = false;
static bool extended_key = false;
static char history[HISTORY_SIZE][BUFFER_SIZE];
static int history_count = 0;
static int history_index = -1;

extern "C" void keyboard_handler_asm();

static const char scancode_to_ascii[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0
};

static const char scancode_to_ascii_shift[] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0
};

static void prompt() {
    hal::print_utf8(i18n::text(i18n::MSG_PROMPT));
}

static bool starts_with(const char* text, const char* prefix) {
    while (*prefix) {
        if (*text++ != *prefix++) {
            return false;
        }
    }
    return true;
}

static const char* skip_spaces(const char* text) {
    while (*text == ' ') {
        ++text;
    }
    return text;
}

static void copy_token(const char* start, char* out, uint32_t out_size) {
    uint32_t i = 0;
    while (start[i] != '\0' && start[i] != ' ' && i + 1 < out_size) {
        out[i] = start[i];
        ++i;
    }
    out[i] = '\0';
}

static void print_file_result(FileResult result) {
    if (result == fs::FILE_ERR_NOT_FOUND) {
        hal::print_utf8(i18n::text(i18n::MSG_FILE_NOT_FOUND));
    } else if (result == fs::FILE_ERR_EXISTS) {
        hal::print_string("File already exists.\n");
    } else if (result == fs::FILE_ERR_FULL) {
        hal::print_string("File table is full.\n");
    } else if (result == fs::FILE_ERR_INVALID_NAME) {
        hal::print_string("Invalid file name.\n");
    } else if (result == fs::FILE_ERR_TOO_LARGE) {
        hal::print_string("File content is too large.\n");
    }
}

static void get_cpuid(uint32_t info_type, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    __asm__ volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(info_type));
}

static void print_uint(uint32_t value) {
    char buffer[12];
    uint32_t pos = 0;
    if (value == 0) {
        hal::print_char('0');
        return;
    }
    while (value > 0 && pos < sizeof(buffer)) {
        buffer[pos++] = (char)('0' + value % 10);
        value /= 10;
    }
    while (pos > 0) {
        hal::print_char(buffer[--pos]);
    }
}

static void execute_command(const char* cmd) {
    if (kstrcmp(cmd, "help") == 0) {
        hal::print_utf8(i18n::text(i18n::MSG_HELP_HEADER));
        hal::print_utf8(i18n::text(i18n::MSG_HELP_CORE));
        hal::print_utf8(i18n::text(i18n::MSG_HELP_FILES));
        hal::print_utf8(i18n::text(i18n::MSG_HELP_LANG));
        hal::print_string(" apps run <app-id>\n");
        prompt();
        return;
    }

    if (kstrcmp(cmd, "clear") == 0) {
        hal::clear_screen();
        hal::print_utf8(i18n::text(i18n::MSG_WELCOME));
        prompt();
        return;
    }

    if (kstrcmp(cmd, "about") == 0) {
        hal::print_utf8(i18n::text(i18n::MSG_ABOUT));
        prompt();
        return;
    }

    if (kstrcmp(cmd, "sysmit") == 0 || kstrcmp(cmd, "sysinfo") == 0) {
        uint32_t eax, ebx, ecx, edx;
        char vendor[13];
        hal::print_utf8(i18n::text(i18n::MSG_SYSINFO_HEADER));
        hal::print_string("OS: XBTOS-LC Alpha 1029\nArchitecture: x86 protected mode\nCPU Vendor: ");
        get_cpuid(0, &eax, &ebx, &ecx, &edx);
        *(uint32_t*)&vendor[0] = ebx;
        *(uint32_t*)&vendor[4] = edx;
        *(uint32_t*)&vendor[8] = ecx;
        vendor[12] = '\0';
        hal::print_string(vendor);
        hal::print_string("\n");
        prompt();
        return;
    }

    if (kstrcmp(cmd, "lst") == 0) {
        vfs.list("/");
        prompt();
        return;
    }

    if (starts_with(cmd, "touch ")) {
        const char* filename = skip_spaces(cmd + 5);
        FileResult result = vfs.create(filename);
        if (result == fs::FILE_OK) {
            hal::print_utf8(i18n::text(i18n::MSG_FILE_CREATED));
        } else {
            print_file_result(result);
        }
        prompt();
        return;
    }

    if (starts_with(cmd, "rdf ")) {
        const char* filename = skip_spaces(cmd + 3);
        const char* content = vfs.read(filename);
        if (content) {
            hal::print_utf8(content);
            hal::print_string("\n");
        } else {
            hal::print_utf8(i18n::text(i18n::MSG_FILE_NOT_FOUND));
        }
        prompt();
        return;
    }

    if (starts_with(cmd, "rec ")) {
        const char* args = skip_spaces(cmd + 3);
        char filename[XBTOS_FILE_NAME_SIZE];
        copy_token(args, filename, sizeof(filename));
        const char* content = args;
        while (*content != '\0' && *content != ' ') {
            ++content;
        }
        content = skip_spaces(content);
        if (filename[0] == '\0' || content[0] == '\0') {
            hal::print_utf8(i18n::text(i18n::MSG_USAGE_WRITE));
        } else {
            FileResult result = vfs.write(filename, content);
            if (result == fs::FILE_OK) {
                hal::print_utf8(i18n::text(i18n::MSG_FILE_WRITE_OK));
            } else {
                print_file_result(result);
            }
        }
        prompt();
        return;
    }

    if (starts_with(cmd, "append ")) {
        const char* args = skip_spaces(cmd + 6);
        char filename[XBTOS_FILE_NAME_SIZE];
        copy_token(args, filename, sizeof(filename));
        const char* content = args;
        while (*content != '\0' && *content != ' ') {
            ++content;
        }
        content = skip_spaces(content);
        FileResult result = vfs.append(filename, content);
        if (result == fs::FILE_OK) {
            hal::print_utf8(i18n::text(i18n::MSG_FILE_WRITE_OK));
        } else {
            print_file_result(result);
        }
        prompt();
        return;
    }

    if (starts_with(cmd, "rm ")) {
        FileResult result = vfs.remove(skip_spaces(cmd + 2));
        if (result == fs::FILE_OK) {
            hal::print_utf8(i18n::text(i18n::MSG_FILE_DELETED));
        } else {
            print_file_result(result);
        }
        prompt();
        return;
    }

    if (starts_with(cmd, "stat ")) {
        FileStat stat;
        FileResult result = vfs.stat(skip_spaces(cmd + 4), &stat);
        if (result == fs::FILE_OK) {
            hal::print_string("Name: ");
            hal::print_string(stat.name);
            hal::print_string("\nSize: ");
            print_uint(stat.size);
            hal::print_string(" bytes\n");
        } else {
            print_file_result(result);
        }
        prompt();
        return;
    }

    if (kstrcmp(cmd, "dirs") == 0) {
        hal::print_utf8(i18n::text(i18n::MSG_CURRENT_PATH));
        prompt();
        return;
    }

    if (kstrcmp(cmd, "apps") == 0) {
        hal::print_string("Installed apps:\n");
        hal::print_string(" - com.xbtos.demo\n");
        prompt();
        return;
    }

    if (starts_with(cmd, "run ")) {
        const char* identifier = skip_spaces(cmd + 3);
        app::AppResult result = app::manager().launch(identifier);
        if (result == app::APP_ERR_NOT_FOUND) {
            hal::print_string("App not found.\n");
        } else if (result != app::APP_OK) {
            hal::print_string("App launch failed.\n");
        }
        prompt();
        return;
    }

    if (starts_with(cmd, "lang ")) {
        const char* language = skip_spaces(cmd + 4);
        if (i18n::set_language_by_name(language)) {
            if (i18n::get_language() == i18n::LANG_ZH_CN) {
                hal::print_utf8(i18n::text(i18n::MSG_LANGUAGE_ZH));
            } else {
                hal::print_utf8(i18n::text(i18n::MSG_LANGUAGE_EN));
            }
        } else {
            hal::print_utf8(i18n::text(i18n::MSG_HELP_LANG));
        }
        prompt();
        return;
    }

    hal::print_utf8(i18n::text(i18n::MSG_UNKNOWN_COMMAND));
    hal::print_string(cmd);
    hal::print_string("\n");
    prompt();
}

static void clear_input_buffer() {
    while (buffer_index > 0) {
        --buffer_index;
        hal::print_char('\b');
    }
    input_buffer[0] = '\0';
}

static void recall_history(int direction) {
    if (history_count == 0) {
        return;
    }

    if (direction < 0) {
        if (history_index == -1) {
            history_index = history_count - 1;
        } else if (history_index > 0) {
            --history_index;
        }
    } else if (history_index != -1) {
        ++history_index;
        if (history_index >= history_count) {
            history_index = -1;
            clear_input_buffer();
            return;
        }
    }

    if (history_index >= 0 && history_index < history_count) {
        clear_input_buffer();
        for (int i = 0; history[history_index][i] != '\0' && buffer_index < BUFFER_SIZE - 1; ++i) {
            input_buffer[buffer_index++] = history[history_index][i];
            hal::print_char(history[history_index][i]);
        }
        input_buffer[buffer_index] = '\0';
    }
}

static void save_history() {
    if (buffer_index == 0) {
        return;
    }

    if (history_count < HISTORY_SIZE) {
        kstrcpy(history[history_count++], input_buffer);
    } else {
        for (int i = 0; i < HISTORY_SIZE - 1; ++i) {
            kstrcpy(history[i], history[i + 1]);
        }
        kstrcpy(history[HISTORY_SIZE - 1], input_buffer);
    }
    history_index = -1;
}

static void handle_enter() {
    hal::print_string("\n");
    input_buffer[buffer_index] = '\0';
    save_history();
    if (buffer_index > 0) {
        execute_command(input_buffer);
    } else {
        prompt();
    }
    buffer_index = 0;
    input_buffer[0] = '\0';
}

extern "C" void keyboard_handler() {
    uint8_t scancode = hal::keyboard_read_scancode();

    if (scancode == 0xE0) {
        extended_key = true;
        hal::interrupt_controller().end_of_interrupt(1);
        return;
    }

    if (extended_key) {
        if (scancode == 0x48) {
            recall_history(-1);
        } else if (scancode == 0x50) {
            recall_history(1);
        }
        extended_key = false;
        hal::interrupt_controller().end_of_interrupt(1);
        return;
    }

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        hal::interrupt_controller().end_of_interrupt(1);
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = false;
        hal::interrupt_controller().end_of_interrupt(1);
        return;
    }

    if (scancode < sizeof(scancode_to_ascii)) {
        char c = shift_pressed ? scancode_to_ascii_shift[scancode] : scancode_to_ascii[scancode];
        if (c == '\n') {
            handle_enter();
        } else if (c == '\b') {
            if (buffer_index > 0) {
                --buffer_index;
                input_buffer[buffer_index] = '\0';
                hal::print_char('\b');
                history_index = -1;
            }
        } else if (c != 0 && buffer_index < BUFFER_SIZE - 1) {
            input_buffer[buffer_index++] = c;
            input_buffer[buffer_index] = '\0';
            hal::print_char(c);
            history_index = -1;
        }
    }

    hal::interrupt_controller().end_of_interrupt(1);
}

extern "C" void kernel_main() {
    pm_init(128 * 1024 * 1024);
    heap_init();
    hal::hal_init();
    hal::idt_set_gate(0x21, keyboard_handler_asm, 0x08, 0x8E);
    vfs.init();
    app::manager().init();
    register_builtin_apps();
    hal::enable_interrupts();

    hal::clear_screen();
    hal::print_string("128/128MB --- MEM  OK\n");
    hal::print_string("i386-X86  --- Heap OK\n");
    hal::print_string("HAL       --- Ready\n");
    hal::print_string("VFS       --- Ready\n");
    hal::print_utf8(i18n::text(i18n::MSG_WELCOME));
    prompt();

    while (true) {
        hal::halt_cpu();
    }
}

