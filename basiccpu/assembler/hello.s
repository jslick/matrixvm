init:
    jmp     main    ; skip past data

define OUTPUT_DMA   4 + 1920 * 1080 * 3 + 4 + 1
define OUTPORT      1

S1:
    db      0x01 "Hello World!" 0x0a 0
S1_LENGTH:

main:
    mov     r1, S1
    mov     r2, S1_LENGTH-S1
    memcpy  OUTPUT_DMA
    write   OUTPORT, 1
    halt
