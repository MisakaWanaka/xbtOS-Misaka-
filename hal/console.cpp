#include "console.h"
#include "ports.h"

namespace hal {

static volatile uint16_t* const VGA_BUFFER = (volatile uint16_t*)0xB8000;
static const uint32_t VGA_WIDTH = 80;
static const uint32_t VGA_HEIGHT = 25;
static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;
static uint8_t color = 0x0F;

static void update_cursor() {
    uint16_t pos = (uint16_t)(cursor_y * VGA_WIDTH + cursor_x);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void scroll_if_needed() {
    if (cursor_y < VGA_HEIGHT) {
        return;
    }

    for (uint32_t i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; ++i) {
        VGA_BUFFER[i] = VGA_BUFFER[i + VGA_WIDTH];
    }
    for (uint32_t i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; ++i) {
        VGA_BUFFER[i] = ((uint16_t)color << 8) | ' ';
    }
    cursor_y = VGA_HEIGHT - 1;
}

static void vga_clear() {
    for (uint32_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i) {
        VGA_BUFFER[i] = ((uint16_t)color << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

static void vga_put_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        ++cursor_y;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            --cursor_x;
            VGA_BUFFER[cursor_y * VGA_WIDTH + cursor_x] = ((uint16_t)color << 8) | ' ';
        }
    } else {
        VGA_BUFFER[cursor_y * VGA_WIDTH + cursor_x] = ((uint16_t)color << 8) | (uint8_t)c;
        ++cursor_x;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        ++cursor_y;
    }
    scroll_if_needed();
    update_cursor();
}

static void vga_write(const char* text) {
    if (!text) {
        text = "(null)";
    }
    for (uint32_t i = 0; text[i] != '\0'; ++i) {
        vga_put_char(text[i]);
    }
}

static void vga_write_utf8(const char* text) {
    if (!text) {
        vga_write("(null)");
        return;
    }

    for (uint32_t i = 0; text[i] != '\0'; ++i) {
        uint8_t byte = (uint8_t)text[i];
        if (byte < 0x80) {
            vga_put_char((char)byte);
            continue;
        }

        vga_put_char('?');
        if ((byte & 0xE0) == 0xC0) {
            i += 1;
        } else if ((byte & 0xF0) == 0xE0) {
            i += 2;
        } else if ((byte & 0xF8) == 0xF0) {
            i += 3;
        }
    }
}

static const ConsoleDevice vga_console = {
    vga_clear,
    vga_put_char,
    vga_write,
    vga_write_utf8
};

void console_init() {
    vga_clear();
}

const ConsoleDevice& console() {
    return vga_console;
}

void clear_screen() {
    vga_console.clear();
}

void print_char(char c) {
    vga_console.put_char(c);
}

void print_string(const char* text) {
    vga_console.write(text);
}

void print_utf8(const char* text) {
    vga_console.write_utf8(text);
}

} // namespace hal

extern "C" void clear_screen() {
    hal::clear_screen();
}

extern "C" void print_char(char c) {
    hal::print_char(c);
}

extern "C" void print_string(const char* text) {
    hal::print_string(text);
}
