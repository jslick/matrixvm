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

func1_msg:
    db      "func1" 0x0a 0
func1:
    mov     r4, func1_msg
    mov     r5, func1 - func1_msg
func1_loop:
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
