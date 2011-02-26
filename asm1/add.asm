;main:
;	mov		ax,   5
;	mov		bx, 100
;	add		ax, bx
;
;	mov		buf, ax
;
;	ret
;
;buf:
;	db " ", 10, 0

main:
	put		  5,  reg0
	put		100,  reg1
	add		reg1, reg0

	put		reg0, buf[0]

	ret

buf:
	.string ' ', 2

stdout:
	.string 64
