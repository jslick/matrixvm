init:
    jmp     main    ; skip past data

define OUTPUT_DMA   0x005eec88
define OUTPORT      2

S1:
    db      0x01 "Hello World!" 0x0a 0
S1_LENGTH:

main:
    mov     r1, S1
    mov     r2, S1_LENGTH-S1
    mov     r3, OUTPUT_DMA
    memcpy  r3, r1, r2
    write   OUTPORT, 1
    halt
