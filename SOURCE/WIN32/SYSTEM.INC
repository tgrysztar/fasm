
; flat assembler  version 1.06
; Copyright (c) 1999-2000, Tomasz Grysztar
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

character db ?,0
bytes_count dd ?
