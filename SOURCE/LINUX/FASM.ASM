
; flat assembler interface for Linux
; Copyright (c) 1999-2005, Tomasz Grysztar.
; All rights reserved.

	format	ELF executable
	entry	start

section readable executable

start:
	mov	esi,_logo
	call	display_string

	mov	[command_line],esp
	pop	eax
	lea	esp,[esp+eax*4]
	pop	eax
	pop	[environment]
	call	get_params
	jc	information

	call	init_memory

	mov	eax,78
	mov	ebx,buffer
	xor	ecx,ecx
	int	0x80
	mov	eax,dword [buffer]
	mov	ecx,1000
	mul	ecx
	mov	ebx,eax
	mov	eax,dword [buffer+4]
	div	ecx
	add	eax,ebx
	mov	[start_time],eax

	call	preprocessor
	call	parser
	call	assembler
	call	formatter

	call	display_user_messages
	movzx	eax,[current_pass]
	inc	eax
	call	display_number
	mov	esi,_passes_suffix
	call	display_string
	mov	eax,78
	mov	ebx,buffer
	xor	ecx,ecx
	int	0x80
	mov	eax,dword [buffer]
	mov	ecx,1000
	mul	ecx
	mov	ebx,eax
	mov	eax,dword [buffer+4]
	div	ecx
	add	eax,ebx
	sub	eax,[start_time]
	jnc	time_ok
	add	eax,3600000
      time_ok:
	xor	edx,edx
	mov	ebx,100
	div	ebx
	or	eax,eax
	jz	display_bytes_count
	xor	edx,edx
	mov	ebx,10
	div	ebx
	push	edx
	call	display_number
	mov	dl,'.'
	call	display_character
	pop	eax
	call	display_number
	mov	esi,_seconds_suffix
	call	display_string
      display_bytes_count:
	mov	eax,[written_size]
	call	display_number
	mov	esi,_bytes_suffix
	call	display_string
	xor	al,al
	jmp	exit_program

information:
	mov	esi,_usage
	call	display_string
	mov	al,1
	jmp	exit_program

include 'system.inc'

include '..\version.inc'
include '..\errors.inc'
include '..\expressi.inc'
include '..\preproce.inc'
include '..\parser.inc'
include '..\assemble.inc'
include '..\formats.inc'
include '..\x86.inc'

section readable writeable

_copyright db 'Copyright (c) 1999-2005, Tomasz Grysztar',0xA,0

_logo db 'flat assembler  version ',VERSION_STRING,0xA,0
_usage db 'usage: fasm source [output]',0xA,0

_passes_suffix db ' passes, ',0
_seconds_suffix db ' seconds, ',0
_bytes_suffix db ' bytes.',0xA,0

align 4

include '..\variable.inc'

command_line dd ?
memory_setting dd ?
environment dd ?
start_time dd ?
displayed_count dd ?
last_displayed db ?
character db ?

buffer rb 1000h
