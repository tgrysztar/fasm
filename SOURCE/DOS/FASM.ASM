
; flat assembler interface for DOS
; Copyright (c) 1999-2003, Tomasz Grysztar.
; All rights reserved.

	org	100h

	macro	align value { rb (value-1) - ($ + value-1) mod value }

start:

	mov	ah,4Ah
	mov	bx,1010h
	int	21h
	mov	dx,_logo
	mov	ah,9
	int	21h

	cld

	call	init_flatrm
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

	mov	di,characters
	mov	cx,100h
	xor	al,al
      make_characters_table:
	stosb
	inc	al
	loop	make_characters_table
	mov	si,characters+'a'
	mov	di,characters+'A'
	mov	cx,26
	rep	movsb
	mov	di,characters
	mov	si,symbol_characters+1
	movzx	cx,byte [si-1]
	xor	bx,bx
      convert_table:
	lodsb
	mov	bl,al
	mov	byte [di+bx],0
	loop	convert_table

	mov	ebx,46Ch
	sub	ebx,[program_base]
	mov	eax,[ebx]
	mov	[start_time],eax

	call	preprocessor
	call	parser
	call	assembler
	call	formatter

	movzx	eax,[current_pass]
	inc	al
	call	display_number
	mov	ah,9
	mov	dx,_passes_suffix
	int	21h
	mov	ebx,46Ch
	sub	ebx,[program_base]
	mov	eax,[ebx]
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
	int	21h
      display_bytes_count:
	mov	eax,[written_size]
	call	display_number
	mov	ah,9
	mov	dx,_bytes_suffix
	int	21h
	xor	al,al
	jmp	exit_program

information:
	mov	dx,_usage
	mov	ah,9
	int	21h
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
macro_block dd ?
macro_block_line_number dd ?
struc_name dd ?
current_locals_prefix dd ?
anonymous_reverse dd ?
anonymous_forward dd ?
labels_list dd ?
label_hash dd ?
org_origin dd ?
org_sib dd ?
org_start dd ?
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

macro_status db ?
parenthesis_stack db ?
output_format db ?
code_type db ?
current_pass db ?
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
buffer rb 1000h

rb 400h
if $ > 10000h
  display 'warning: 64k limit exceeded.'
end if
