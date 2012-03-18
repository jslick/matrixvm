init:
    jmp     os_start    ; skip past data

; hardware values
define TIMER_PIN            1
define TIMER_IRQ            0x00000004
define DISPLAY_PORT         8
define DISPLAY_DMA          0x00000084
define KEYBOARD_DATA_PIN    0x8
define KEYBOARD_IRQ         0x00000008
define OUTPUT_DMA           0x005eec88 + 1
define OUTPORT              2

; configuration
define TIMER_INTERVAL   1000000 ; 1Hz

define KEY_Q    0x35

start_message:
    db      "Starting..." 0x0a 0
start_message_end:

os_start:
    mov     r4, start_message
    mov     r5, start_message_end - start_message
    call    print

    ; set up timer interrupt handler
    write   TIMER_PIN, TIMER_INTERVAL   ; set up interrupt interval
    mov     r1, TIMER_IRQ
    str     r1, handle_timer

    ; set up keyboard interrupt handler
    mov     r1, KEYBOARD_IRQ
    str     r1, handle_keyboard

    sti ; enable interrupts

    call    idle_loop

os_stop:
    halt

idle_loop:
    idle
    jmp idle_loop

handle_timer_msg:
    db      "handle_timer" 0x0a 0
handle_timer:
    mov     r4, handle_timer_msg
    mov     r5, handle_timer - handle_timer_msg
    call    print

    rti

; interrupt handler for keyboard input
handle_keyboard:
    ; get keyboard data
    read    r4, KEYBOARD_DATA_PIN

    ; stop if user presses Q key
    cmp     r4, KEY_Q
    je      os_stop

    rti

; r4 = address of string
; r5 = length of string
print:
    mov     r1, r4
    mov     r2, r5
    memcpy  OUTPUT_DMA
    write   OUTPORT, 1
    ret
