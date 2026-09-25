#include "header/cpu/gdt.h"

struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        // Null descriptor
        {
            .segment_low  = 0,
            .base_low     = 0,
            .base_mid     = 0,
            .type_bit     = 0,
            .non_system   = 0,
            .dpl          = 0,
            .present      = 0,
            .segment_high = 0,
            .avl          = 0,
            .long_mode    = 0,
            .default_op   = 0,
            .granularity  = 0,
            .base_high    = 0
        },
        // Kernel code segment
        {
            .segment_low  = 0xFFFF,
            .base_low     = 0,
            .base_mid     = 0,
            .type_bit     = 0xA,    // Code, not accessed, readable, not conforming
            .non_system   = 1,      // Code or Data Segment
            .dpl          = 0,      // PL0 / Kernel
            .present      = 1,
            .segment_high = 0xF,
            .avl          = 0,
            .long_mode    = 0,
            .default_op   = 1,      // 32-bit operand
            .granularity  = 1,      // 4 KB
            .base_high    = 0
        },
        // Kernel data segment
        {
            .segment_low  = 0xFFFF,
            .base_low     = 0,
            .base_mid     = 0,
            .type_bit     = 0x2,    // Data, not accessed, writable, direction up
            .non_system   = 1,
            .dpl          = 0,
            .present      = 1,
            .segment_high = 0xF,
            .avl          = 0,
            .long_mode    = 0,
            .default_op   = 1,
            .granularity  = 1,
            .base_high    = 0
        }
    }
};

struct GDTR _gdt_gdtr = {
    .size    = sizeof(struct GlobalDescriptorTable) - 1,
    .address = &global_descriptor_table
};
