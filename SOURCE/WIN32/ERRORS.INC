
; flat assembler  version 1.09
; Copyright (c) 1999-2000, Tomasz Grysztar
; All rights reserved.

out_of_memory:
	call	fatal_error
	db	'out of memory',0
main_file_not_found:
	call	fatal_error
	db	'source file not found',0
write_failed:
	call	fatal_error
	db	'write failed',0
code_cannot_be_generated:
	call	fatal_error
	db	'code cannot be generated',0
unexpected_end_of_file:
	call	fatal_error
	db	'unexpected end of file',0
file_not_found:
	call	assembler_error
	db	'file not found',0
error_reading_file:
	call	assembler_error
	db	'error reading file',0
invalid_macro_arguments:
	call	assembler_error
	db	'invalid macro arguments',0
unexpected_characters:
	call	assembler_error
	db	'unexpected characters',0
invalid_argument:
	call	assembler_error
	db	'invalid argument',0
illegal_instruction:
	call	assembler_error
	db	'illegal instruction',0
unexpected_instruction:
	call	assembler_error
	db	'unexpected instruction',0
invalid_operand:
	call	assembler_error
	db	'invalid operand',0
invalid_operand_size:
	call	assembler_error
	db	'invalid size of operand',0
operand_size_not_specified:
	call	assembler_error
	db	'operand size not specified',0
operand_sizes_do_not_match:
	call	assembler_error
	db	'operand sizes do not match',0
invalid_address_size:
	call	assembler_error
	db	'invalid size of address value',0
address_sizes_do_not_agree:
	call	assembler_error
	db	'address sizes do not agree',0
invalid_expression:
	call	assembler_error
	db	'invalid expression',0
invalid_address:
	call	assembler_error
	db	'invalid address',0
invalid_value:
	call	assembler_error
	db	'invalid value',0
value_out_of_range:
	call	assembler_error
	db	'value out of range',0
invalid_use_of_symbol:
	call	assembler_error
	db	'invalid use of symbol',0
relative_jump_out_of_range:
	call	assembler_error
	db	'relative jump out of range',0
extra_characters_on_line:
	call	assembler_error
	db	'extra characters on line',0
name_too_long:
	call	assembler_error
	db	'name too long',0
argument_too_long:
	call	assembler_error
	db	'argument too long',0
invalid_name:
	call	assembler_error
	db	'invalid name',0
reserved_word_used_as_symbol:
	call	assembler_error
	db	'reserved word used as symbol',0
symbol_already_defined:
	call	assembler_error
	db	'symbol already defined',0
missing_end_quote:
	call	assembler_error
	db	'missing end quote',0

fatal_error:
	mov	edx,error_prefix
	call	display
	pop	edx
	call	display
	mov	edx,error_suffix
	call	display
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
	call	display
	mov	edx,[home_line]
	add	edx,5
	call	display
	mov	edx,cr_lf
	call	display
	mov	edx,error_prefix
	call	display
	pop	edx
	call	display
	mov	edx,error_suffix
	call	display
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
	call	display
	mov	edx,line_number_start
	call	display
	mov	eax,ebp
	and	eax,0FFFFFh
	call	display_number
	mov	dl,']'
	call	display_character
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

error_prefix db 'error: ',0
error_suffix db '.'
cr_lf db 0Dh,0Ah,0
line_number_start db ' [',0
line_number_end db ':',0Dh,0Ah,0
