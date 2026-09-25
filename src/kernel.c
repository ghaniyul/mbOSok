#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/driver/keyboard.h"

void kernel_setup(void) {
    // M0: Load Global Descriptor Table
    load_gdt(&_gdt_gdtr);

    // M1: Interrupt setup
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();

    // M1: Framebuffer test
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    framebuffer_write(3, 8,  'H', 0, 0xF);
    framebuffer_write(3, 9,  'a', 0, 0xF);
    framebuffer_write(3, 10, 'i', 0, 0xF);
    framebuffer_write(3, 11, '!', 0, 0xF);

    // M1: Keyboard echo
    int row = 5, col = 0;
    keyboard_state_activate();
    while (true) {
        char c = 0;
        get_keyboard_buffer(&c);
        if (c) {
            framebuffer_write(row, col, c, 0xF, 0);
            ++col;
            if (col >= FRAMEBUFFER_WIDTH) {
                col = 0;
                ++row;
            }
            framebuffer_set_cursor(row, col);
        }
    }
}
