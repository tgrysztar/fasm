
; flat assembler  version 1.04
; Copyright (c) 1999-2000, Tomasz Grysztar
; All rights reserved.

init_memory:
	push	PAGE_READWRITE
	push	MEM_COMMIT
	push	800000h
	push	0
	call	[VirtualAlloc]
	or	eax,eax
	jz	out_of_memory
	mov	[memory_start],eax
	add	eax,800000h
	mov	[memory_end],eax
	push	PAGE_READWRITE
	push	MEM_COMMIT
	push	0C0000h
	push	0
	call	[VirtualAlloc]
	or	eax,eax
	jz	out_of_memory
	mov	[additional_memory],eax
	add	eax,0C0000h
	mov	[additional_memory_end],eax
	ret

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
	push	0
	push	bytes_count
	push	ecx
	push	edx
	push	ebx
	call	[ReadFile]
	or	eax,eax
	jz	file_error
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
display:
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

include 'win32.inc'

character db ?,0
bytes_count dd ?
