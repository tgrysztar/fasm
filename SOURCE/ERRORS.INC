
; flat assembler  version 1.09
; Copyright (c) 1999-2000, Tomasz Grysztar
; All rights reserved.

processor_error:
	call	fatal_error
	db	'required 80386 or better',24h
pm_error:
	call	fatal_error
	db	'processor is in protected mode',24h
out_of_memory:
	call	fatal_error
	db	'out of memory',24h
main_file_not_found:
	call	fatal_error
	db	'source file not found',24h
write_failed:
	call	fatal_error
	db	'write failed',24h
code_cannot_be_generated:
	call	fatal_error
	db	'code cannot be generated',24h
unexpected_end_of_file:
	call	fatal_error
	db	'unexpected end of file',24h
file_not_found:
	call	assembler_error
	db	'file not found',24h
error_reading_file:
	call	assembler_error
	db	'error reading file',24h
invalid_macro_arguments:
	call	assembler_error
	db	'invalid macro arguments',24h
unexpected_characters:
	call	assembler_error
	db	'unexpected characters',24h
invalid_argument:
	call	assembler_error
	db	'invalid argument',24h
illegal_instruction:
	call	assembler_error
	db	'illegal instruction',24h
unexpected_instruction:
	call	assembler_error
	db	'unexpected instruction',24h
invalid_operand:
	call	assembler_error
	db	'invalid operand',24h
invalid_operand_size:
	call	assembler_error
	db	'invalid size of operand',24h
operand_size_not_specified:
	call	assembler_error
	db	'operand size not specified',24h
operand_sizes_do_not_match:
	call	assembler_error
	db	'operand sizes do not match',24h
invalid_address_size:
	call	assembler_error
	db	'invalid size of address value',24h
address_sizes_do_not_agree:
	call	assembler_error
	db	'address sizes do not agree',24h
invalid_expression:
	call	assembler_error
	db	'invalid expression',24h
invalid_address:
	call	assembler_error
	db	'invalid address',24h
invalid_value:
	call	assembler_error
	db	'invalid value',24h
value_out_of_range:
	call	assembler_error
	db	'value out of range',24h
invalid_use_of_symbol:
	call	assembler_error
	db	'invalid use of symbol',24h
relative_jump_out_of_range:
	call	assembler_error
	db	'relative jump out of range',24h
extra_characters_on_line:
	call	assembler_error
	db	'extra characters on line',24h
name_too_long:
	call	assembler_error
	db	'name too long',24h
argument_too_long:
	call	assembler_error
	db	'argument too long',24h
invalid_name:
	call	assembler_error
	db	'invalid name',24h
reserved_word_used_as_symbol:
	call	assembler_error
	db	'reserved word used as symbol',24h
symbol_already_defined:
	call	assembler_error
	db	'symbol already defined',24h
missing_end_quote:
	call	assembler_error
	db	'missing end quote',24h

fatal_error:
	mov	dx,error_prefix
	mov	ah,9
	int	21h
	pop	dx
	int	21h
	mov	dx,error_suffix
	int	21h
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
	mov	ah,2
	int	21h
	call	display_line_number
      line_number_ok:
	mov	dx,line_number_end
	mov	ah,9
	int	21h
	mov	esi,[home_line]
	add	esi,5
	call	display_asciiz
	mov	ah,9
	mov	dx,cr_lf
	int	21h
	mov	dx,error_prefix
	int	21h
	pop	dx
	int	21h
	mov	dx,error_suffix
	int	21h
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
	call	display_asciiz
	mov	ah,9
	mov	dx,line_number_start
	int	21h
	mov	eax,ebp
	and	eax,0FFFFFh
	call	display_number
	mov	dl,']'
	mov	ah,2
	int	21h
	ret
display_asciiz:
	lods	byte [esi]
	or	al,al
	jz	asciiz_end
	mov	dl,al
	mov	ah,2
	int	21h
	jmp	display_asciiz
      asciiz_end:
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
	mov	ah,2
	int	21h
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

error_prefix db 'error: ',24h
error_suffix db '.'
cr_lf db 0Dh,0Ah,24h
line_number_start db ' [',24h
line_number_end db ':',0Dh,0Ah,24h
