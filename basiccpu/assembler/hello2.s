init:
    jmp     main    ; skip past data

define DISPLAY_PORT 8
define DISPLAY_DMA  0x00000084
define OUTPUT_DMA   0x005eec88 + 1
define OUTPORT      1
define KEYBOARD_DATA_PIN 0x8

start_message:
    db      "Starting..." 0x0a 0
start_message_end:

main:
    mov     r4, start_message
    mov     r5, start_message_end - start_message
    call    print

    ; set up keyboard interrupt
    ; TODO after store instruction is implemented; for now the vector is
    ;      hardcoded into the CPU.

    sti ; enable interrupts

    ; draw vertical at (50,25) to (50,175)
    mov     r4,  50
    mov     r5,  25
    mov     r6, 150
    call    draw_vertical
    call    some_idle

    ; draw vertical at (150,25) to (150,175)
    mov     r4, 150
    mov     r5,  25
    mov     r6, 150
    call    draw_vertical
    call    some_idle

    ; draw horizontal at (50,100) to (50+100,100)
    mov     r4,  50
    mov     r5, 100
    mov     r6, 100
    call    draw_horizontal
    call    some_idle
    write   DISPLAY_PORT, 1 ; flush display

    halt

some_idle:
    idle
    idle
    idle
    idle
    ret

handle_keyboard_data:
    db      "handle_keyboard" 0x0a 0
handle_keyboard:
    mov     r4, handle_keyboard_data
    mov     r5, handle_keyboard - handle_keyboard_data
    call    print

    ; get keyboard data
    read    r4, KEYBOARD_DATA_PIN

    ; mask off mode bit (press or release)
    ; TODO

    ; mask off keycode
    ; TODO

    rti

; r4 = address of string
; r5 = length of string
print:
    mov     r1, r4
    mov     r2, r5
    memcpy  OUTPUT_DMA  ; copy string at address r1 to device memory
    write   OUTPORT, 1  ; tell device to do its thing; 2nd operand is irrelevant for this particular device
    ret

; r4 = left
; r5 = top
; r6 = length
draw_vertical:
    ; give to clrsetv these values:
    ; r1 = DISPLAY_DMA + (top * width + left)
    ; r2 = 640 (the skip interval)
    ; r3 = length
    mov     r1, r5
    mul     r1, 640
    add     r1, r4
    mul     r1, 3
    add     r1, DISPLAY_DMA

    mov     r2, 640

    mov     r3, r6

    clrsetv 0xFF0000

    ret

; r4 = left
; r5 = top
; r6 = length
draw_horizontal:
    ; set pixels from (top,left) to (top,left+length) to 0xFF0000

    ; give to clrset these values:
    ; r1 = DISPLAY_DMA + 3 * (top * width + left)
    ; r2 = length
    mov     r1, r5
    mul     r1, 640
    add     r1, r4
    mul     r1, 3
    add     r1, DISPLAY_DMA

    mov     r2, r6

    clrset  0xFF0000

    ret
