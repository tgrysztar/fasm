
; flat assembler interface for DOS
; Copyright (c) 1999-2005, Tomasz Grysztar.
; All rights reserved.

	format	MZ
	heap	0
	entry	main:start

include 'modes.inc'

segment main use16

start:

	mov	ax,ds
	mov	dx,[2Ch]
	push	cs cs
	pop	ds es
	mov	[psp_segment],ax
	mov	[environment_segment],dx

	mov	dx,_logo
	mov	ah,9
	int	21h

	cld

	call	go32
	use32

	call	get_params
	jc	information

	call	init_memory

	xor	ah,ah
	int	1Ah
	mov	ax,cx
	shl	eax,16
	mov	ax,dx
	mov	[start_time],eax

	call	preprocessor
	call	parser
	call	assembler
	call	formatter

	call	display_user_messages
	movzx	eax,[current_pass]
	inc	eax
	call	display_number
	mov	ah,9
	mov	dx,_passes_suffix
	call	dos_int
	xor	ah,ah
	int	1Ah
	mov	ax,cx
	shl	eax,16
	mov	ax,dx
	sub	eax,[start_time]
	mov	ebx,100
	mul	ebx
	mov	ebx,182
	div	ebx
	or	eax,eax
	jz	display_bytes_count
	xor	edx,edx
	mov	ebx,10
	div	ebx
	push	edx
	call	display_number
	mov	ah,2
	mov	dl,'.'
	int	21h
	pop	eax
	call	display_number
	mov	ah,9
	mov	dx,_seconds_suffix
	call	dos_int
      display_bytes_count:
	mov	eax,[written_size]
	call	display_number
	mov	ah,9
	mov	dx,_bytes_suffix
	call	dos_int
	xor	al,al
	jmp	exit_program

information:
	mov	dx,_usage
	mov	ah,9
	call	dos_int
	mov	al,1
	jmp	exit_program

include 'system.inc'

include '..\version.inc'

_copyright db 'Copyright (c) 1999-2005, Tomasz Grysztar',0Dh,0Ah,24h

_logo db 'flat assembler  version ',VERSION_STRING,0Dh,0Ah,24h
_usage db 'usage: fasm source [output]',0Dh,0Ah,24h

_passes_suffix db ' passes, ',24h
_seconds_suffix db ' seconds, ',24h
_bytes_suffix db ' bytes.',0Dh,0Ah,24h

include '..\errors.inc'
include '..\expressi.inc'
include '..\preproce.inc'
include '..\parser.inc'
include '..\assemble.inc'
include '..\formats.inc'
include '..\x86_64.inc'

align 4

include '..\variable.inc'

memory_setting dd ?
start_time dd ?
params rb 100h

segment buffer

rb 1000h
