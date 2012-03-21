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

; Each task has these fields:
; magic             : 4 bytes
; pid               : 2 bytes
; ppid              : 2 bytes
; saved registers   : 4 bytes * 15
;  * r1 - r6
;  * reserved regs
;  * sp, ip, lr, dl, st
; = 64 bytes
define TASK_MAGIC       0xD000000D
define MAX_TASKS        128
define BYTES_PER_TASK    68
define TASK_REGS_OFFSET   8 ; wouldn't need this if defines could accept expressions

define STACK_PER_TASK   4 * 1024

; number of timer interrupts between task scheduling + 1
define SCHEDULE_INTERVAL    2

current_task:
    dd      0
num_tasks:
    dd      0
schedule_countdown:
    db      0
task_list:
    space   MAX_TASKS * BYTES_PER_TASK

; new tasks will be an offset from the base_stack
base_stack:
    dd      0

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

    ; set up tasks
    ; initialize schedule_countdowns
    mov     r1, schedule_countdown
    strb    r1, SCHEDULE_INTERVAL
    ; store current sp as base_stack
    mov     r2, base_stack
    str     r2, sp

    ; create idle task
    mov     r4, idle_task
    mov     r5, 0
    call    create_task
    ; start idle task
    mov     r4, r1
    call    switch_to
    ; never return

    halt

    sti ; enable interrupts

    call    main
    halt

; save register of current task
; r4 = address of start of registers
save_current_registers:
    call    get_current_task
    add     r1, TASK_REGS_OFFSET    ; skip to registers
    mov     r2, 60
    memcpy  r1, r4, r2
    ret

; Switch to a task
; r4 = task pid
switch_to:
    ; TODO:  flush status

    ; get task structure
    call    get_task
    ; skip to registers
    add     r1, TASK_REGS_OFFSET

    ; update current_task
    mov     r2, current_task
    str     r2, r4

    rstr    r1

schedule_msg:
    db      "schedule" 0x0a 0
schedule:
    mov     r4, schedule_msg
    mov     r5, schedule - schedule_msg
    call    print

    ; simple scheduler:  just pick the next task
    load    r4, current_task
    inc     r4

schedule_test_magic:
    ; check to see if this (current_task+1) is a task; if not, start over at task 1
    call    get_task
    load    r1, r1
    cmp     r1, TASK_MAGIC
    jne     schedule_task_1

    call    switch_to
    ; never return
    halt    ; but just in case, we don't really want to fall through if it does happen
schedule_task_1:
    mov     r4, 1
    jmp     schedule_test_magic

idle_task:
    call    fork
    tst     r1
    jne     idle_loop
    call    init_task
    halt
idle_loop:
    idle
    jmp idle_loop

init_task:
    sti ; enable interrupts
    call    main
    ret

; TODO:  flush status
handle_timer_msg:
    db      "handle_timer" 0x0a 0
handle_timer:
    mov     r4, handle_timer_msg
    mov     r5, handle_timer - handle_timer_msg
    call    print

    ; decrement schedule_countdown
    ; TODO:  need to make this atomic
    loadb   r1, schedule_countdown
    dec     r1
    mov     r2, schedule_countdown
    ; if schedule_countdown becomes 0, it's time to schedule a new task
    je      handle_timer_schedule
    strb    r2, r1
handle_timer_return:
    rti
handle_timer_schedule:
    ; reset schedule_countdown
    mov     r1, SCHEDULE_INTERVAL
    strb    r2, r1

    ; save current tasks's registers
    mov     r4, sp
    call    save_current_registers

    ; schedule a new task
    call    schedule
    ; never return
    halt

; interrupt handler for keyboard input
handle_keyboard:
    ; get keyboard data
    read    r1, KEYBOARD_DATA_PIN

    ; stop if user presses Q key
    cmp     r1, KEY_Q
    je      os_stop

    rti

os_stop:
    halt

; TODO:  priority field(s)
; r4 = ip
; r5 = ppid
; return id of new task
create_task:
    ; TODO:  disable preemption
    push    r4

    ; calculate new task's stack
    ; sp = base_stack - index * STACK_PER_TASK
    load    r2, num_tasks
    mov     r1, r2  ; r1 = new task id
    mul     r2, STACK_PER_TASK
    load    r3, base_stack
    sub     r3, r2

    ; TODO:  lr should point to cleanup task routine
    push    st
    push    dl
    push    r4  ; push ip
    push     0  ; push lr
    push    r3  ; push sp
    push     0  ; reserved reg
    push     0  ; reserved reg
    push     0  ; reserved reg
    push     0  ; reserved reg
    push     0  ; push r6
    push     0  ; push r5
    push     0  ; push r4
    push     0  ; push r3
    push     0  ; push r2
    push     0  ; push r1 ; after fork, child sees r1=0
    pushw   r5  ; push ppid
    pushw   r1  ; push pid
    push    TASK_MAGIC  ; this is a task

    ; get new task
    mov     r4, r1
    call    get_task
    ; copy data from sp to sp+BYTES_PER_TASK to memory[this_task]
    mov     r2, BYTES_PER_TASK
    memcpy  r1, sp, r2
    ; restore new task id
    mov     r1, r4
    ; pop all
    add     sp, r2

    ; increment num_tasks
    mov     r2, r1
    inc     r2
    mov     r3, num_tasks
    str     r3, r2

    pop     r4
    ret     ; return r1=pid

; r4 = index (pid)
get_task:
    ; task = task_list + index * BYTES_PER_TASK
    mov     r1, r4
    mul     r1, BYTES_PER_TASK
    add     r1, task_list

    ret

get_current_task:
    push    r4
    load    r4, current_task
    call    get_task
    pop     r4

    ret

; return r1  = 0 for parent
; return r1 != 0 for child
fork:
    push    r4
    push    r5
    mov     r4, lr              ; ip = calling function
    load    r5, current_task    ; ppid = current_task
    call    create_task
    pop     r5
    pop     r4

    ; save registers
    push    st
    push    dl
    push    fork_ret    ; ip
    push    lr
    push    sp
    push     0
    push     0
    push     0
    push     0
    push    r6
    push    r5
    push    r4
    push    r3
    push    r2
    push    r1  ; return value = child task id

    mov     r4, sp
    push    r1      ; save child task id
    call    save_current_registers

    ; switch to child
    pop     r4      ; pop child task id into r4
    call    switch_to

fork_ret:
    add     sp, 16
    ret

; r4 = address of string
; r5 = length of string
print:
    mov     r1, OUTPUT_DMA
    memcpy  r1, r4, r5
    write   OUTPORT, 1
    ret

; r4 = num
print_hex:
    define  print_hex_0x 0x3078 ; 0x
    mov     r1, sp
    pushw   print_hex_0x

    add     sp, 2
    ret
