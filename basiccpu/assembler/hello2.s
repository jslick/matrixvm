init:
    jmp     main    ; skip past data

S1:
    db      "Testing" 0x0a 0
S1_LENGTH:
S2:
    db      "Calling draw_h" 0x0a 0
S2_LENGTH:
S3:
    db      "In draw_h; and halting" 0x0a 0
S3_LENGTH:

main:
    mov     r4, S1
    mov     r5, S1_LENGTH-S1
    call    print

    mov     r4, S2
    mov     r5, S2_LENGTH-S2
    call    print
    call    draw_h

    halt

; r4 = address of string
; r5 = length of string
print:
    mov     r1, r4
    mov     r2, r5
    memcpy  OUTPUT_DMA  ; copy string at address r1 to device memory
    write   OUTPORT, 1  ; tell device to do its thing; 2nd operand is irrelevant for this particular device
    ret

draw_h:
    mov     r4, S3
    mov     r5, S3_LENGTH-S3
    call    print

    halt
    ret     ; halt and then return, wth?
