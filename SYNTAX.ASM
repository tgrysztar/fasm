
; there is no documentation yet, so here are some examples of syntax:
; (note: all instructions and labels are case sensitive)

; - memory addressing

	mov	[cs:bx+si+10],al	; full 16-bit address
	mov	[ebx*5],al		; simple 32-bit address

; - defining size of arguments

	push	dword 0 		; not very complicated
	mov	word [esi],4		; argument size must be defined here
	mov	[esi],word 4		; also correct

; - data instructions

	dw	1,2			; two words
	rb	5			; reserve five bytes
	file	'syntax.asm'		; include file contents

; - labels and constants

alfa:					; simple label
	label	beta			; as above
	label	gamma byte		; low byte of delta dword
delta	dd	0			; data label
epsilon = alfa + 1			; constant definition

; - jump and call instructions

	jmp	alfa			; simple jump
	jmp	near byte beta		; short jump
	jmp	near dword beta 	; force dword size
	jmp	10h:50h 		; far jump
	call	far pword [1000h]	; 32-bit far call
	call	far dword [delta]	; 16-bit far call
	call	near dword [delta]	; 32-bit near call

; - conditional jumps

	je	alfa			; will be optimized
	jge	byte alfa		; force byte (error if out of range)
	jb	dword alfa		; force dword (no optimization)

; - values

	db	12			; decimal
	db	101111b 		; binary
	db	ABh			; hexadecimal (case sensitive!)

; - calculations

	db	not (8 or 4)		; logical
	dw	2+4*5+10 mod 3		; arithmetic
	dq	8000000001*45000h	; 64-bit

; - macroinstructions

times 5 db 90h				; five bytes with value 90h
macro alfa number { db number } 	; simple macro definition
macro beta number			; also correct
 {
	db	number
 }
