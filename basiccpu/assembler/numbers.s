define PROG_OFFSET 7000000

main_message:
    db      "Dumping interesting (non-zero) memory:" 0x0a 0
main:
    ; disable timer
    write   TIMER_PIN, 0

    mov     r4, main_message
    mov     r5, main - main_message
    call    print

    ; converting string will be stored on the stack here:
    sub     sp, 11
    mov     r2, 11
    mov     r1, 0
    memset  sp, r1, r2

    ; r5 holds address of stack string
    mov     r5, sp

    ; start dumping memory from address 0
    mov     r7, 0

main_loop:
    load    r4, r7
    ; if r4 is just 0, skip to next address
    tst     r4
    je      main_loop_inc

    ; convert r4 to hex string
    call    convert_hex_to_string
    mov     r4, r5
    call    print_result

main_loop_inc:
    ; check if we've reached end of memory
    load    r1, base_stack
    cmp     r1, r7
    je      main_exit
    add     r7, 4   ; move to next address in memory
    jmp     main_loop

main_exit:
    ret

; r4 = number to convert
; r5 = location to store result
convert_hex_to_string:
    push    r4
    push    r5
    push    r6
    push    r7

    ; r7 = mask
    mov     r7, 0xF0000000
    ; r6 = shifter:  the amount to right shift r4 & r7
    mov     r6, 28
convert_h_to_s_findchar:
    ; r1 = (r7 & r4) >> r6.  This gives us one nibble of r4 in the least
    ; significant 4 bits
    mov     r1, r7
    and     r1, r4
    shr     r1, r6

    ; convert the nibble (r1) to a character
    push    r4  ; save the number
    mov     r4, r1
    call    nibble_to_char
    pop     r4  ; restore the number

    strb    r5, r1  ; store result in string
    inc     r5      ; increment pointer to string

    ; shift the mask (r7)
    shr     r7, 4
    ; if the mask is 0, we are done, otherwise continue
    je      convert_h_to_s_ret
    ; reduce the shifter (r6)
    sub     r6, 4
    jmp     convert_h_to_s_findchar ; loop

convert_h_to_s_ret:
    pop     r7
    pop     r6
    pop     r5
    pop     r4
    ret

; convert a nibble to a character
; r4 = nibble (least significant 4 bits of r4)
nibble_to_char:
    mov     r1, r4
    cmp     r4, 0xa
    jge     nib_to_ch_letter
    add     r1, '0'
    ret
nib_to_ch_letter:
    add     r1, 'A' - 0xa
    ret

; print the string at r4
; r4 = location of string
print_result_0x:
    db      "0x" 0
print_result_space:
    db      0x20 0  ; a space, then zero
print_result:
    push    r4
    push    r5

    push    r4
    ; print 0x
    mov     r4, print_result_0x
    mov     r5, 3
    call    print

    ; print param string
    pop     r4
    mov     r5, 11
    call    print

    ; print a space
    mov     r4, print_result_space
    mov     r5, 2
    call    print

    pop     r5
    pop     r4
    ret
