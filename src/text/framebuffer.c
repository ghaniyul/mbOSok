#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include "header/cpu/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    uint16_t position = (uint16_t) r * FRAMEBUFFER_WIDTH + c;

    out(CURSOR_PORT_CMD, 0x0E);
    out(CURSOR_PORT_DATA, (uint8_t) (position >> 8));
    out(CURSOR_PORT_CMD, 0x0F);
    out(CURSOR_PORT_DATA, (uint8_t) (position & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    uint32_t index = ((uint32_t) row * FRAMEBUFFER_WIDTH + col) * 2;

    FRAMEBUFFER_MEMORY_OFFSET[index]     = c;
    FRAMEBUFFER_MEMORY_OFFSET[index + 1] = (bg << 4) | (fg & 0x0F);
}

void framebuffer_clear(void) {
    for (uint32_t i = 0; i < FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT; i++) {
        FRAMEBUFFER_MEMORY_OFFSET[i * 2]     = 0x00;
        FRAMEBUFFER_MEMORY_OFFSET[i * 2 + 1] = 0x07;
    }
}
