
; flat assembler interface for DOS
; Copyright (c) 1999-2003, Tomasz Grysztar.
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
	call	init_memory

	call	get_params
	cmp	[params],0
	je	information
	lea	eax,[params+1]
	mov	[input_file],eax
	movzx	ecx,byte [eax-1]
	add	eax,ecx
	cmp	byte [eax],0
	je	information
	inc	eax
	mov	[output_file],eax
	movzx	ecx,byte [eax-1]
	add	eax,ecx
	cmp	byte [eax],0
	jne	information

	mov	edi,characters
	mov	ecx,100h
	xor	al,al
      make_characters_table:
	stosb
	inc	al
	loop	make_characters_table
	mov	esi,characters+'a'
	mov	edi,characters+'A'
	mov	ecx,26
	rep	movsb
	mov	edi,characters
	mov	esi,symbol_characters+1
	movzx	ecx,byte [esi-1]
	xor	ebx,ebx
      convert_table:
	lodsb
	mov	bl,al
	mov	byte [edi+ebx],0
	loop	convert_table
	mov	[display_buffer],0

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
include '..\errors.inc'
include '..\expressi.inc'
include '..\preproce.inc'
include '..\parser.inc'
include '..\assemble.inc'
include '..\formats.inc'
include '..\x86.inc'

_copyright db 'Copyright (c) 1999-2002, Tomasz Grysztar',0Dh,0Ah,24h

_logo db 'flat assembler  version ',VERSION_STRING,0Dh,0Ah,24h
_usage db 'usage: fasm source output',0Dh,0Ah,24h

_passes_suffix db ' passes, ',24h
_seconds_suffix db ' seconds, ',24h
_bytes_suffix db ' bytes.',0Dh,0Ah,24h

_counter db 8,'00000000'

align 4

program_base dd ?
buffer_address dd ?
memory_start dd ?
memory_end dd ?
additional_memory dd ?
additional_memory_end dd ?
free_additional_memory dd ?
input_file dd ?
output_file dd ?
source_start dd ?
code_start dd ?
code_size dd ?
real_code_size dd ?
start_time dd ?
written_size dd ?

current_line dd ?
macros_list dd ?
macro_constants dd ?
macro_line dd ?
macro_block dd ?
macro_block_line dd ?
macro_block_line_number dd ?
macro_embed_level dd ?
struc_name dd ?
current_locals_prefix dd ?
fix_constants_count dd ?
equ_constants_count dd ?
anonymous_reverse dd ?
anonymous_forward dd ?
labels_list dd ?
label_hash dd ?
org_origin dd ?
org_registers dd ?
org_start dd ?
org_symbol dd ?
undefined_data_start dd ?
undefined_data_end dd ?
counter dd ?
counter_limit dd ?
error_line dd ?
error dd ?
display_buffer dd ?
structures_buffer dd ?
number_start dd ?
current_offset dd ?
value dq ?
fp_value rd 8
symbol_identifier dd ?
address_symbol dd ?
format_flags dd ?
symbols_stream dd ?
number_of_relocations dd ?
number_of_sections dd ?
stub_size dd ?
stub_file dd ?
current_section dd ?
machine dw ?
subsystem dw ?
subsystem_version dd ?
image_base dd ?
resource_data dd ?
resource_size dd ?

macro_status db ?
parenthesis_stack db ?
output_format db ?
code_type db ?
current_pass dw ?
next_pass_needed db ?
reloc_labels db ?
times_working db ?
virtual_data db ?
fp_sign db ?
fp_format db ?
value_size db ?
forced_size db ?
value_undefined db ?
value_type db ?
address_size db ?
compare_type db ?
base_code db ?
extended_code db ?
postbyte_register db ?
segment_register db ?
operand_size db ?
imm_sized db ?
jump_type db ?
mmx_size db ?
mmx_prefix db ?
nextbyte db ?

characters rb 100h
params rb 100h
converted rb 100h

segment buffer

rb 1000h
