
; flat assembler source
; Copyright (c) 1999-2002, Tomasz Grysztar
; All rights reserved.

O_ACCMODE  = 00003
O_RDONLY   = 00000
O_WRONLY   = 00001
O_RDWR	   = 00002
O_CREAT    = 00100
O_EXCL	   = 00200
O_NOCTTY   = 00400
O_TRUNC    = 01000
O_APPEND   = 02000
O_NONBLOCK = 04000

S_ISUID    = 04000
S_ISGID    = 02000
S_ISVTX    = 01000
S_IRUSR    = 00400
S_IWUSR    = 00200
S_IXUSR    = 00100
S_IRGRP    = 00040
S_IWGRP    = 00020
S_IXGRP    = 00010
S_IROTH    = 00004
S_IWOTH    = 00002
S_IXOTH    = 00001

init_memory:
	xor	ebx,ebx
	mov	eax,45
	int	0x80
	mov	[additional_memory],eax
	mov	ebx,buffer
	mov	eax,116
	int	0x80
    allocate_memory:
	mov	ebx,[additional_memory]
	add	ebx,dword [buffer+14h]
	mov	eax,45
	int	0x80
	mov	[memory_end],eax
	sub	eax,[additional_memory]
	jz	not_enough_memory
	shr	eax,3
	add	eax,[additional_memory]
	mov	[additional_memory_end],eax
	mov	[memory_start],eax
	ret
    not_enough_memory:
	shr	dword [buffer+14h],1
	cmp	dword [buffer+14h],4000h
	jb	out_of_memory
	jmp	allocate_memory

exit_program:
	movzx	ebx,al
	mov	eax,1
	int	0x80

open:
	push	edx esi edi ebp
	mov	ebx,edx
	mov	eax,5
	mov	ecx,O_RDONLY
	xor	edx,edx
	int	0x80
	pop	ebp edi esi edx
	test	eax,eax
	js	file_error
	mov	ebx,eax
	clc
	ret
    file_error:
	stc
	ret
create:
	push	edx esi edi ebp
	mov	ebx,edx
	mov	eax,5
	mov	ecx,O_CREAT+O_TRUNC+O_WRONLY
	mov	edx,S_IRUSR+S_IWUSR+S_IRGRP
	int	0x80
	pop	ebp edi esi edx
	test	eax,eax
	js	file_error
	mov	ebx,eax
	clc
	ret
close:
	mov	eax,6
	int	0x80
	ret
read:
	push	ecx edx esi edi ebp
	mov	eax,3
	xchg	ecx,edx
	int	0x80
	pop	ebp edi esi edx ecx
	test	eax,eax
	js	file_error
	cmp	eax,ecx
	jne	file_error
	clc
	ret
write:
	push	edx esi edi ebp
	mov	eax,4
	xchg	ecx,edx
	int	0x80
	pop	ebp edi esi edx
	test	eax,eax
	js	file_error
	clc
	ret
lseek:
	mov	ecx,edx
	xor	edx,edx
	mov	dl,al
	mov	eax,19
	int	0x80
	clc
	ret

display_string:
	push	ebx
	mov	edi,esi
	mov	edx,esi
	or	ecx,-1
	xor	al,al
	repne	scasb
	neg	ecx
	sub	ecx,2
	mov	eax,4
	mov	ebx,1
	xchg	ecx,edx
	int	0x80
	pop	ebx
	ret
display_block:
	push	ebx
	mov	eax,4
	mov	ebx,1
	mov	edx,ecx
	mov	ecx,esi
	int	0x80
	pop	ebx
	ret
display_character:
	push	ebx
	mov	[character],dl
	mov	eax,4
	mov	ebx,1
	mov	ecx,character
	mov	edx,ebx
	int	0x80
	pop	ebx
	ret
display_number:
	push	ebx
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
	pop	ebx
	ret

fatal_error:
	mov	esi,error_prefix
	call	display_string
	pop	esi
	call	display_string
	mov	esi,error_suffix
	call	display_string
	mov	al,0FFh
	jmp	exit_program
assembler_error:
	call	flush_display_buffer
	mov	ebx,[current_line]
      find_error_home:
	test	byte [ebx+7],80h
	jz	error_home_ok
	mov	ebx,[ebx+8]
	jmp	find_error_home
      error_home_ok:
	mov	esi,[ebx]
	call	display_string
	mov	esi,line_number_start
	call	display_string
	mov	eax,[ebx+4]
	call	display_number
	mov	dl,']'
	call	display_character
	cmp	ebx,[current_line]
	je	line_number_ok
	mov	dl,20h
	call	display_character
	mov	esi,[current_line]
	mov	esi,[esi]
	movzx	ecx,byte [esi]
	inc	esi
	call	display_block
	mov	esi,line_number_start
	call	display_string
	mov	esi,[current_line]
	mov	eax,[esi+4]
	and	eax,7FFFFFFFh
	call	display_number
	mov	dl,']'
	call	display_character
      line_number_ok:
	mov	esi,line_data_start
	call	display_string
	mov	esi,ebx
	mov	edx,[esi]
	call	open
	mov	al,2
	xor	edx,edx
	call	lseek
	mov	edx,[esi+8]
	sub	eax,edx
	push	eax
	xor	al,al
	call	lseek
	mov	ecx,[esp]
	mov	edx,[memory_start]
	call	read
	call	close
	pop	ecx
	mov	esi,[memory_start]
      get_line_data:
	mov	al,[esi]
	cmp	al,0Ah
	je	display_line_data
	cmp	al,0Dh
	je	display_line_data
	cmp	al,1Ah
	je	display_line_data
	or	al,al
	jz	display_line_data
	inc	esi
	loop	get_line_data
      display_line_data:
	mov	ecx,esi
	mov	esi,[memory_start]
	sub	ecx,esi
	call	display_block
	mov	esi,lf
	call	display_string
	mov	esi,error_prefix
	call	display_string
	pop	esi
	call	display_string
	mov	esi,error_suffix
	call	display_string
	jmp	exit_program

character db ?,0

error_prefix db 'error: ',0
error_suffix db '.'
lf db 0xA,0
line_number_start db ' [',0
line_data_start db ':',0xA,0

macro dm string { db string,0 }
