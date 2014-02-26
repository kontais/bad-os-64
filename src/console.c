#include "common.h"

static char * const VideoStart = (char *) 0xB8000;
static char *current = (char*) 0xB8000;

static const uint16_t lines = 25;
static const uint16_t cols = 160; // in bytes, 80 columns

typedef unsigned long ptrdiff_t;

static char hex(int i) {
    if (i > 16) return '?';
    if (i > 9) return 'A' + (i - 10);
    else return '0' + i;
}

void scroll() {
//    uint32_t esp;
//    asm ("mov %%esp, %0" : "=r"(esp));
//    for (int i = 28; i >= 0; i -= 4) {
//        VideoStart[cols * 3 - i / 2 - 2] = hex((esp >> i) & 0xf );
//    }

    for (uint16_t row = 0; row < lines - 1; row++) {
        for(uint16_t col = 0; col < cols; col++) {
            uint16_t pos = row * cols + col;
            VideoStart[pos] =
                VideoStart[pos + cols];
        }
    }

    char * lastLine = VideoStart + (lines - 1) * cols;
    for(int i = 0; i < cols; i+=2) {
        lastLine[i] = ' ';
    }

    current -= cols;
}

void set_cursor() {
    // ftp://ftp.apple.asimov.net/pub/apple_II/documentation/hardware/video/Second%20Sight%20VGA%20Registers.pdf
    static const uint16_t basePort= 0x3d4;
    uint16_t position = (current - VideoStart) / 2;
    outb(basePort, 0x0a);
    outb(basePort+1, 13); // start scan line
    outb(basePort, 0x0b);
    outb(basePort+1, 14); // end scan line

    if (position < 80 * 25) {
        outb(basePort, 0x0e);
        outb(basePort+1, (position >> 8) & 0xff); // position y
        outb(basePort, 0x0f);
        outb(basePort+1, position & 0xff); //position x
    }
}

void console_put(char c) {
    switch(c) {
        case('\n'):
            current += cols;
            ptrdiff_t dist = current - VideoStart;
            current = VideoStart + (dist / cols) * cols;
            break;
        case('\b'):
            current -= 2;
            *current = ' ';
            if (current < VideoStart)
                current = VideoStart;
            break;
        default:
            *current = c;
            current += 2;
    }

    while (current >= VideoStart + (lines  - 1) * cols) {
        scroll();
    }

    set_cursor();
}

void console_clear_screen() {
    char * p = VideoStart;
    for (; p < VideoStart + lines * cols; p += 2) {
        *p = ' ';
    }

    current = VideoStart;
    set_cursor();
}

void console_print_string(const char * str) {
    while (*str) {
        console_put(*str++);
    }
}

void console_put_hex(uint32_t v) {
    console_put('0');
    console_put('x');
    for (int i = 28; i >= 0; i -= 4) {
        console_put(hex((v >> i) & 0xf ));
    }
}

void console_put_hex64(uint64_t v) {
    console_put('0');
    console_put('x');
    for (int i = 60; i >= 0; i -= 4) {
        console_put(hex((v >> i) & 0xf ));
    }
}

void console_put_dec(uint32_t v) {
    char str[11];
    str[10] = 0;
    if (!v) {
        console_put('0');
        return;
    }

    char * p = str + 9;
    for(; v; p--) {
        *p = v % 10 + '0';
        v /= 10;
    }

    console_print_string(p + 1);
}
