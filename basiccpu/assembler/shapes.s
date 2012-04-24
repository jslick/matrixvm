;;; Shape functions ;;;

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
    write   DISPLAY_PORT, 1 ; flush display
    pop     r4
    pop     r6
    ret

; r4 = left
; r5 = top
; r6 = length
; r7 = color
draw_horizontal:
    push    r6
    push    r4

    ; give to clrset these values:
    ; r1 = DISPLAY_DMA + 3 * (top * width + left)
    ; r2 = length

    ; Verify our y position is in bounds, if not do not draw anything
    tst     r5
    jl      hori_ret
    cmp     r4, 639
    jg      hori_ret

    ; Now check the x position. Even if off screen, we may still have some part
    ; of this line that can still be displayed
    tst     r4
    jge     right_check
    add     r6, r4
    tst     r6
    jle     hori_ret
    mov     r4, 0           ; We want to start drawing from 0 (left of screen)
                            ; but the line we draw will be shorter now
    jmp     hori_draw

right_check:
    cmp     r4, 639         ; Remember, 0 indexed
    jge     hori_ret        ; We are out of bounds
    ; We need to check the length
    mov     r1, r4          ; We are in bounds
    add     r1, r6          ; Find ending ypos
    cmp     r1, 639
    jge     shorten_hline   ; Oops, the end is out of bounds, need to shorten it by the amount larger than 639
    jmp     hori_draw

shorten_hline:
    sub     r1, 639
    sub     r6, r1

hori_draw:
    mov     r1, r5
    mul     r1, 640
    add     r1, r4
    mul     r1, 3
    add     r1, DISPLAY_DMA

    mov     r2, r6

    clrset  r7

hori_ret:
    pop     r4
    pop     r6
    ret

;;; Main ;;;

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

    push    draw_square_wrapper ; draw callback
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

; Called as a shape callback
; r4 = shape structure
draw_square_wrapper:
    push    r4
    push    r5
    push    r6
    push    r7

    ; In essence:  draw_square(shape->xpos, shape->pos, shape->param1, shape->color)

    mov     r2, r4
    ; r4 = shape->xpos
    add     r2, 4
    load    r4, r2
    ; r5 = shape->ypos
    add     r2, 4
    load    r5, r2
    ; r6 = shape->param1
    add     r2, 6
    load    r6, r2
    ; r7 = shape->color
    add     r2, 8
    load    r7, r2

    call    draw_square

    pop     r7
    pop     r6
    pop     r5
    pop     r4
    ret

; Draw a square
; r4 = left
; r5 = top
; r6 = sidelength
; r7 = color
draw_square:
    mov     r3, r6              ; Set counter to the sidelength
    call    draw_rect

    ret

; r4 = left
; r5 = top
; r6 = xlength
; r7 = color
; r3 = ylength
draw_rect:
    push    r5
rectloop:
    push    r3
    call    draw_horizontal
    inc     r5
    pop     r3
    dec     r3
    tst     r3
    jne     rectloop

    pop     r5
    ret
