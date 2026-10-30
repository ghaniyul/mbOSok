global _start
extern main

; Process entrypoint.
; The M3 kernel does not push argc/argv, main() must not use them.
section .text
_start:
    mov ebp, esp
    call main

    ; Exit syscall (10) is Milestone 4, unreachable in M3 since main never returns
    mov ebx, eax
    mov eax, 10
    int 0x30

    ; Should never be reached
    cli
    hlt

section .note.GNU-stack noalloc noexec nowrite progbits
