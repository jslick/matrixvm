; Shape structure
;   magic           : 4 bytes
;   xpos            : 4 bytes
;   ypos            : 4 bytes
;   xdir            : 1 bytes
;   ydir            : 1 bytes
;   param1          : 4 bytes
;   param2          : 4 bytes
;   color           : 4 bytes
;   draw_callback   : 4 bytes
define SHAPE_MAGIC      0x01234567
define BYTES_PER_SHAPE  30
define SHAPE_CB_OFFSET  26
define MAX_SHAPES       16

num_shapes:
    dd      0
shape_list:
    space   MAX_SHAPES * BYTES_PER_SHAPE

; Insert a shape into the shape_list
; r4 = pointer to shape structure
; returns shape id
insert_shape:
    ; shape = &shape_list[num_shapes++]
    mov     r1, shape_list
    load    r2, num_shapes
    mov     r3, r2  ; save num_shapes for later
    mul     r2, BYTES_PER_SHAPE
    add     r2, r1

    ; Copy structure to shape list
    mov     r1, BYTES_PER_SHAPE
    memcpy  r2, r4, r1

    ; Increment num_shapes
    mov     r1, r3
    inc     r3
    mov     r2, num_shapes
    str     r2, r3

    ret

; Moves each shape in the shape list
move_all:
    ; TODO
    ret

;
draw_all:
    push    r6
    push    r4

    load    r6, num_shapes
    dec     r6
    mov     r4, shape_list
    ; r6 is counter
    ; r4 is pointer to current shape
draw_all_loop:
    cmp     r6, 0-1
    je      draw_all_ret

    ; NOTE:  This doesn't check the shape's magic value, so shapes cannot be
    ;        yet be deleted!

    ; get callback
    mov     r2, r4
    add     r2, SHAPE_CB_OFFSET
    load    r2, r2
    ; invoke:  callback(shape)
    call    r2

    add     r4, BYTES_PER_SHAPE
    dec     r6
    jmp     draw_all_loop

draw_all_ret:
    pop     r4
    pop     r6
    ret

main:
    call    create_a_square
main_move:
    call    move_all
    call    draw_all
    idle
    jmp     main_move

    ret

; Creates a blue 50x50 square @ (200,100), moving down & right
create_a_square:
    push    r4

    push    draw_square ; draw callback
    push    0x000080    ; color = blue
    push    0           ; param2 = unused
    push    50          ; param1 = length
    pushb   1           ; ydir
    pushb   1           ; xdir
    push    100         ; ypos
    push    200         ; xpos
    push    SHAPE_MAGIC
    mov     r4, sp
    call    insert_shape

    pop     r4
    ret

; Draw a square
; r4 = shape structure
draw_square_msg:
    db      "draw_square_msg" 0xa 0
draw_square:
    push    r4
    push    r5

    mov     r4, draw_square_msg
    mov     r5, draw_square - draw_square_msg
    call    print

    pop     r5
    pop     r4
    ret
