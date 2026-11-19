#include "header/cpu/interrupt.h"
#include "header/cpu/gdt.h"
#include "header/cpu/portio.h"
#include "header/driver/keyboard.h"
#include "header/filesystem/ext2.h"
#include "header/memory/paging.h"
#include "header/process/process.h"
#include "header/process/scheduler.h"
#include "header/text/framebuffer.h"
#include "header/user/syscall.h"

struct TSSEntry _interrupt_tss_entry = {
    .ss0  = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
};

void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile ("mov %%ebp, %0": "=r"(stack_ptr) : /* <Empty> */);
    // +8 skips saved ebp and return address, so esp0 = caller esp before the call
    _interrupt_tss_entry.esp0 = stack_ptr + 8;
}

void io_wait(void) {
    out(0x80, 0);
}

void pic_ack(uint8_t irq) {
    if (irq >= 8) out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void) {
    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100);      // ICW3: tell Master PIC, slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010);      // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Disable all interrupts
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK);
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

void activate_keyboard_interrupt(void) {
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_KEYBOARD));
}

void activate_timer_interrupt(void) {
    __asm__ volatile("cli");
    // Setup how often PIT fire
    uint32_t pit_timer_counter_to_fire = PIT_TIMER_COUNTER;
    out(PIT_COMMAND_REGISTER_PIO, PIT_COMMAND_VALUE);
    out(PIT_CHANNEL_0_DATA_PIO, (uint8_t) (pit_timer_counter_to_fire & 0xFF));
    out(PIT_CHANNEL_0_DATA_PIO, (uint8_t) ((pit_timer_counter_to_fire >> 8) & 0xFF));

    // Activate the interrupt
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_TIMER));
}

/* -- Syscall helpers -- */

// The whole range [ptr, ptr + len) must lie below the kernel base
static bool is_user_range(const void *ptr, uint32_t len) {
    uint32_t start = (uint32_t) ptr;
    return ptr != NULL && start < KERNEL_VIRTUAL_ADDRESS_BASE && len <= KERNEL_VIRTUAL_ADDRESS_BASE - start;
}

static bool is_user_pointer(const void *ptr) {
    return is_user_range(ptr, sizeof(uint32_t));
}

static bool is_valid_fs_request(const struct EXT2DriverRequest *request, bool name_optional, bool need_buffer) {
    if (!is_user_range(request, sizeof(*request))) return false;
    if (request->parent_inode == 0) return false;
    if (request->name != NULL) {
        if (request->name_len == 0 || !is_user_range(request->name, request->name_len)) return false;
    } else if (!name_optional) {
        return false;
    }
    if (need_buffer && !is_user_range(request->buf, request->buffer_size)) return false;
    return true;
}

/**
 * System call dispatcher for Milestone 3 (User Mode & Shell).
 *
 * | eax | service                | ebx                      | ecx            | edx        |
 * |-----|------------------------|--------------------------|----------------|------------|
 * | 0   | FS read()              | EXT2DriverRequest*       | int8_t* ret    | -          |
 * | 1   | FS read_directory()    | EXT2DriverRequest*       | int8_t* ret    | -          |
 * | 2   | FS write()             | EXT2DriverRequest*       | int8_t* ret    | -          |
 * | 3   | FS delete()            | EXT2DriverRequest*       | int8_t* ret    | -          |
 * | 4   | keyboard getchar()     | char* out                | -              | -          |
 * | 5   | putchar()              | char                     | text color     | -          |
 * | 6   | puts()                 | char* buffer             | char count     | text color |
 * | 7   | activate keyboard      | -                        | -              | -          |
 * | 8   | FS get_file_size()     | EXT2DriverRequest*       | uint32_t* out  | int8_t* ret|
 * | 9   | clear screen           | -                        | -              | -          |
 * | 15  | set text cursor        | row                      | col            | -          |
 * | 16  | get text cursor        | uint8_t* row             | uint8_t* col   | -          |
 */
void syscall(struct InterruptFrame *frame) {
    struct EXT2DriverRequest *request = (struct EXT2DriverRequest *) frame->cpu.general.ebx;
    uint32_t service = frame->cpu.general.eax;
    frame->cpu.general.eax = 0;

    switch (service) {
        case SYS_READ: {
            if (!is_valid_fs_request(request, false, true)) break;
            if (!is_user_pointer((void *) frame->cpu.general.ecx)) break;
            *((int8_t *) frame->cpu.general.ecx) = read(*request);
            break;
        }
        case SYS_READ_DIR: {
            if (!is_valid_fs_request(request, true, true)) break;
            if (!is_user_pointer((void *) frame->cpu.general.ecx)) break;
            *((int8_t *) frame->cpu.general.ecx) = read_directory(request);
            break;
        }
        case SYS_WRITE: {
            if (!is_valid_fs_request(request, false, false)) break;
            if (!request->is_directory && request->buffer_size > 0 &&
                !is_user_range(request->buf, request->buffer_size)) break;
            if (!is_user_pointer((void *) frame->cpu.general.ecx)) break;
            *((int8_t *) frame->cpu.general.ecx) = write(request);
            break;
        }
        case SYS_DELETE: {
            if (!is_valid_fs_request(request, false, false)) break;
            if (!is_user_pointer((void *) frame->cpu.general.ecx)) break;
            *((int8_t *) frame->cpu.general.ecx) = delete(*request);
            break;
        }
        case SYS_GETCHAR:
            if (is_user_pointer((void *) frame->cpu.general.ebx))
                get_keyboard_buffer((char *) frame->cpu.general.ebx);
            break;
        case SYS_PUTCHAR:
            framebuffer_putchar((char) frame->cpu.general.ebx, (uint8_t) frame->cpu.general.ecx);
            break;
        case SYS_PUTS:
            if (is_user_range((void *) frame->cpu.general.ebx, frame->cpu.general.ecx))
                framebuffer_puts(
                    (char *) frame->cpu.general.ebx,
                    frame->cpu.general.ecx,
                    (uint8_t) frame->cpu.general.edx
                );
            break;
        case SYS_ACTIVATE_KB:
            keyboard_state_activate();
            break;
        case SYS_FILE_SIZE: {
            if (!is_valid_fs_request(request, false, false)) break;
            if (!is_user_pointer((void *) frame->cpu.general.ecx)) break;
            if (!is_user_pointer((void *) frame->cpu.general.edx)) break;
            uint32_t size = 0;
            int8_t retcode = get_file_size(request->name, request->name_len, request->parent_inode, &size);
            if (retcode == 0) *((uint32_t *) frame->cpu.general.ecx) = size;
            *((int8_t *) frame->cpu.general.edx) = retcode;
            break;
        }
        case SYS_CLEAR_SCREEN:
            framebuffer_clear();
            framebuffer_set_cursor(0, 0);
            break;
        case SYS_SET_CURSOR:
            framebuffer_move_cursor((uint8_t) frame->cpu.general.ebx, (uint8_t) frame->cpu.general.ecx);
            break;
        case SYS_GET_CURSOR: {
            uint8_t *row_out = (uint8_t *) frame->cpu.general.ebx;
            uint8_t *col_out = (uint8_t *) frame->cpu.general.ecx;
            if (!is_user_pointer(row_out) || !is_user_pointer(col_out)) break;
            framebuffer_get_cursor(row_out, col_out);
            break;
        }
        case SYS_EXIT: {
            // Terminate the calling process, ebx (exit code of main) is not used.
            // The process is gone, so this syscall never returns to user mode.
            struct ProcessControlBlock *current_pcb = process_get_current_running_pcb_pointer();
            if (current_pcb != NULL) process_destroy(current_pcb->metadata.pid);
            scheduler_switch_to_next_process();
        }
        default:
            // Unknown service
            frame->cpu.general.eax = (uint32_t) -1;
            break;
    }
}

/* -- Exception helpers -- */

static void fb_print(const char *s) {
    uint32_t count = 0;
    while (s[count] != '\0') count++;
    framebuffer_puts(s, count, 0x0C);
}

static void fb_print_hex(uint32_t value) {
    static const char digits[] = "0123456789ABCDEF";
    char buffer[8];
    for (int32_t i = 7; i >= 0; i--) {
        buffer[i] = digits[value & 0xF];
        value >>= 4;
    }
    framebuffer_puts(buffer, 8, 0x0C);
}

__attribute__((noreturn)) static void halt_with_fault(const char *name, struct InterruptFrame *frame) {
    framebuffer_putchar('\n', 0x0C);
    fb_print(name);
    fb_print(" error_code=0x");
    fb_print_hex(frame->int_stack.error_code);
    fb_print(" eip=0x");
    fb_print_hex(frame->int_stack.eip);
    framebuffer_putchar('\n', 0x0C);
    while (true) __asm__ volatile("hlt");
}

/* -- Scheduler helpers -- */

static bool is_from_user_mode(const struct InterruptFrame *frame) {
    return (frame->int_stack.cs & 0x3) == 0x3;
}

/**
 * Build the context of the interrupted process out of the interrupt frame.
 * On inter-privilege interrupt, frame->cpu.stack.esp is the user esp (see intsetup.s).
 */
static struct Context context_from_frame(const struct InterruptFrame *frame) {
    struct Context ctx = {
        .cpu                         = frame->cpu,
        .eip                         = frame->int_stack.eip,
        .eflags                      = frame->int_stack.eflags,
        .page_directory_virtual_addr = paging_get_current_page_directory_addr(),
    };
    return ctx;
}

static void fb_print_decimal(uint32_t value) {
    char buffer[10];
    uint32_t digits = 0;
    do {
        buffer[digits++] = (char) ('0' + value % 10);
        value /= 10;
    } while (value > 0);
    while (digits > 0) framebuffer_putchar(buffer[--digits], 0x0C);
}

/**
 * Handle a CPU exception. A fault raised by a user process only terminates
 * that process, the rest of the system keeps running. A fault raised by the
 * kernel itself is fatal.
 */
static void handle_cpu_exception(const char *name, struct InterruptFrame *frame) {
    struct ProcessControlBlock *current_pcb = process_get_current_running_pcb_pointer();
    if (!is_from_user_mode(frame) || current_pcb == NULL) halt_with_fault(name, frame);

    framebuffer_putchar('\n', 0x0C);
    fb_print(current_pcb->metadata.name);
    fb_print(" (pid ");
    fb_print_decimal(current_pcb->metadata.pid);
    fb_print(") terminated: ");
    fb_print(name);
    fb_print(" eip=0x");
    fb_print_hex(frame->int_stack.eip);
    framebuffer_putchar('\n', 0x0C);

    process_destroy(current_pcb->metadata.pid);
    scheduler_switch_to_next_process();
}

void main_interrupt_handler(struct InterruptFrame *frame) {
    switch (frame->int_number) {
        case PIC1_OFFSET + IRQ_TIMER:
            // ACK first: the context switch below never comes back here
            pic_ack(IRQ_TIMER);
            if (is_from_user_mode(frame)) {
                scheduler_save_context_to_current_running_pcb(context_from_frame(frame));
                scheduler_switch_to_next_process();
            }
            break;
        case PIC1_OFFSET + IRQ_KEYBOARD:
            keyboard_isr();
            break;
        case 0x0D:
            handle_cpu_exception("GENERAL PROTECTION FAULT", frame);
            break;
        case 0x0E: {
            uint32_t cr2;
            __asm__ volatile("mov %%cr2, %0" : "=r"(cr2) : /* <Empty> */);
            framebuffer_putchar('\n', 0x0C);
            fb_print("PAGE FAULT at 0x");
            fb_print_hex(cr2);
            handle_cpu_exception("PAGE FAULT", frame);
            break;
        }
        case 0x30:
            syscall(frame);
            break;
        default:
            // Any other CPU exception: never return with iret to the faulting instruction
            if (frame->int_number < 32) handle_cpu_exception("CPU EXCEPTION", frame);
            break;
    }
}
