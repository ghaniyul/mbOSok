#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/driver/keyboard.h"
#include "header/filesystem/ext2.h"
#include "header/memory/paging.h"
#include "header/process/process.h"

#define SHELL_PARENT_INODE 1u          // root inode in EXT2 IF2130 Edition

static void print(const char *s) {
    uint32_t count = 0;
    while (s[count] != '\0') count++;
    framebuffer_puts(s, count, 0x0F);
}

void kernel_setup(void) {
    // Chapter 0: Global Descriptor Table
    load_gdt(&_gdt_gdtr);

    // Chapter 1: Interrupts & Hardware Drivers
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();

    // Chapter 1: Clear screen (higher-half MMIO 0xC00B8000)
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);

    // Chapter 2: EXT2 File System Initializer
    initialize_filesystem_ext2();

    // Chapter 3: Task State Segment (TSS) for Ring 0 <-> Ring 3 transitions
    gdt_install_tss();
    set_tss_register();

    // Chapter 4: the shell is the init process, loaded from the EXT2 root directory
    struct EXT2DriverRequest request = {
        .buf          = (void *) 0,
        .name         = "shell",
        .name_len     = 5,
        .parent_inode = SHELL_PARENT_INODE,
        .buffer_size  = PROCESS_EXECUTABLE_SIZE_MAX,
        .is_directory = false,
    };

    // Set TSS.esp0 for interprivilege interrupt
    set_tss_kernel_current_stack();

    // Create & execute process 0
    int32_t retcode = process_create_user_process(request);
    if (retcode != PROCESS_CREATE_SUCCESS) {
        if (retcode == PROCESS_CREATE_FAIL_FS_READ_FAILURE)
            print("Error: Failed to load shell binary from disk! (run 'make prepare')\n");
        else
            print("Error: Failed to create the init process!\n");
        while (true) __asm__ volatile("hlt");
    }
    paging_use_page_directory(_process_list[0].context.page_directory_virtual_addr);
    kernel_execute_user_program((void *) 0x0);

    while (true) __asm__ volatile("hlt");
}
