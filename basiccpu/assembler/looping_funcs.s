main:
    call    fork
    tst     r1
    jne     main_call_func2
    call    func1
    jmp     main_ret

main_call_func2:
    call    func2

main_ret:
    mov     r1, 0
    ret

some_idle:
    idle
    idle
    idle
    idle
    ret

func1_msg_a:
    db      "func1 - a" 0x0a 0
func1_msg_b:
    db      "func1 - b" 0x0a 0
func1:
    ; r6 is boolean
    mov     r6, 1
    mov     r5, func1 - func1_msg_b
func1_loop:
    ; toggle boolean in r1
    inc     r6
    cmp     r6, 2
    jne     func1_loop_print
    mov     r6, 0

func1_loop_print:
    ; calculate address of string to print
    mov     r4, r6
    mul     r4, func1_msg_b - func1_msg_a
    add     r4, func1_msg_a
    ; r5 is already calculated
    call    print
    call    some_idle
    jmp     func1_loop

func2_msg:
    db      "func2" 0x0a 0
func2:
    mov     r4, func2_msg
    mov     r5, func2 - func2_msg
func2_loop:
    call    print
    call    some_idle
    jmp     func2_loop
