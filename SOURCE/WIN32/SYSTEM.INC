
; flat assembler source
; Copyright (c) 1999-2001, Tomasz Grysztar
; All rights reserved.

CREATE_NEW	       =   1
CREATE_ALWAYS	       =   2
OPEN_EXISTING	       =   3
OPEN_ALWAYS	       =   4
TRUNCATE_EXISTING      =   5

GENERIC_READ	       =   80000000h
GENERIC_WRITE	       =   40000000h

STD_INPUT_HANDLE       =   0FFFFFFF6h
STD_OUTPUT_HANDLE      =   0FFFFFFF5h
STD_ERROR_HANDLE       =   0FFFFFFF4h

MEM_COMMIT	       =   1000h
MEM_RESERVE	       =   2000h
MEM_DECOMMIT	       =   4000h
MEM_RELEASE	       =   8000h
MEM_FREE	       =   10000h
MEM_PRIVATE	       =   20000h
MEM_MAPPED	       =   40000h
MEM_RESET	       =   80000h
MEM_TOP_DOWN	       =   100000h

PAGE_NOACCESS	       =   1
PAGE_READONLY	       =   2
PAGE_READWRITE	       =   4
PAGE_WRITECOPY	       =   8
PAGE_EXECUTE	       =   10h
PAGE_EXECUTE_READ      =   20h
PAGE_EXECUTE_READWRITE =   40h
PAGE_EXECUTE_WRITECOPY =   80h
PAGE_GUARD	       =   100h
PAGE_NOCACHE	       =   200h

init_memory:
	mov	eax,1000000h
    allocate_memory:
	mov	edx,eax
	shr	edx,3
	mov	ecx,eax
	sub	ecx,edx
	mov	[memory_end],ecx
	mov	[additional_memory_end],edx
	push	PAGE_READWRITE
	push	MEM_COMMIT
	push	eax
	push	0
	call	[VirtualAlloc]
	or	eax,eax
	jz	not_enough_memory
	mov	[memory_start],eax
	add	eax,[memory_end]
	mov	[memory_end],eax
	mov	[additional_memory],eax
	add	[additional_memory_end],eax
	ret
    not_enough_memory:
	mov	eax,[additional_memory_end]
	shl	eax,2
	cmp	eax,4000h
	jb	out_of_memory
	jmp	allocate_memory

exit_program:
	movzx	eax,al
	push	eax
	call	[ExitProcess]

open:
	push	0
	push	0
	push	OPEN_EXISTING
	push	0
	push	0
	push	GENERIC_READ
	push	edx
	call	[CreateFile]
	cmp	eax,-1
	je	file_error
	mov	ebx,eax
	clc
	ret
    file_error:
	stc
	ret
create:
	push	0
	push	0
	push	CREATE_ALWAYS
	push	0
	push	0
	push	GENERIC_WRITE
	push	edx
	call	[CreateFile]
	cmp	eax,-1
	je	file_error
	mov	ebx,eax
	clc
	ret
write:
	push	0
	push	bytes_count
	push	ecx
	push	edx
	push	ebx
	call	[WriteFile]
	or	eax,eax
	jz	file_error
	ret
read:
	mov	ebp,ecx
	push	0
	push	bytes_count
	push	ecx
	push	edx
	push	ebx
	call	[ReadFile]
	or	eax,eax
	jz	file_error
	cmp	ebp,[bytes_count]
	jne	file_error
	ret
close:
	push	ebx
	call	[CloseHandle]
	ret
lseek:
	movzx	eax,al
	push	eax
	push	0
	push	edx
	push	ebx
	call	[SetFilePointer]
	ret

display_string:
	push	STD_OUTPUT_HANDLE
	call	[GetStdHandle]
	mov	ebx,eax
	mov	edi,edx
	or	ecx,-1
	xor	al,al
	repne	scasb
	neg	ecx
	sub	ecx,2
	push	0
	push	bytes_count
	push	ecx
	push	edx
	push	ebx
	call	[WriteFile]
	ret
display_block:
	push	ecx
	push	STD_OUTPUT_HANDLE
	call	[GetStdHandle]
	pop	ecx
	push	0
	push	bytes_count
	push	ecx
	push	esi
	push	eax
	call	[WriteFile]
	ret
display_character:
	push	ebx
	push	STD_OUTPUT_HANDLE
	call	[GetStdHandle]
	mov	ebx,eax
	mov	[character],dl
	push	0
	push	bytes_count
	push	1
	push	character
	push	ebx
	call	[WriteFile]
	pop	ebx
	ret
display_number:
	mov	ecx,1000000000
	xor	edx,edx
	xor	bl,bl
      display_loop:
	div	ecx
	push	edx
	cmp	ecx,1
	je	display_digit
	or	bl,bl
	jnz	display_digit
	or	al,al
	jz	digit_ok
	not	bl
      display_digit:
	mov	dl,al
	add	dl,30h
	push	ebx ecx
	call	display_character
	pop	ecx ebx
      digit_ok:
	mov	eax,ecx
	xor	edx,edx
	mov	ecx,10
	div	ecx
	mov	ecx,eax
	pop	eax
	or	ecx,ecx
	jnz	display_loop
	ret

fatal_error:
	mov	edx,error_prefix
	call	display_string
	pop	edx
	call	display_string
	mov	edx,error_suffix
	call	display_string
	mov	al,0FFh
	jmp	exit_program
assembler_error:
	mov	edx,[home_line]
	mov	ebp,[edx]
	call	display_line_number
	mov	edx,[current_line]
	cmp	edx,[home_line]
	je	line_number_ok
	mov	ebp,[edx]
	mov	dl,20h
	call	display_character
	call	display_line_number
      line_number_ok:
	mov	edx,line_number_end
	call	display_string
	mov	edx,[home_line]
	add	edx,5
	call	display_string
	mov	edx,cr_lf
	call	display_string
	mov	edx,error_prefix
	call	display_string
	pop	edx
	call	display_string
	mov	edx,error_suffix
	call	display_string
	mov	al,2
	jmp	exit_program
      display_line_number:
	mov	ecx,ebp
	shr	ecx,20
	dec	ecx
	mov	esi,[files_list]
	inc	esi
      get_error_file:
	jecxz	error_file_found
      skip_file_name:
	lods	byte [esi]
	or	al,al
	jnz	skip_file_name
	add	esi,5
	loop	get_error_file
      error_file_found:
	mov	edx,esi
	call	display_string
	mov	edx,line_number_start
	call	display_string
	mov	eax,ebp
	and	eax,0FFFFFh
	call	display_number
	mov	dl,']'
	call	display_character
	ret

character db ?,0
bytes_count dd ?

error_prefix db 'error: ',0
error_suffix db '.'
cr_lf db 0Dh,0Ah,0
line_number_start db ' [',0
line_number_end db ':',0Dh,0Ah,0

macro dm string { db string,0 }
