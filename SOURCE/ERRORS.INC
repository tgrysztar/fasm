
out_of_memory:
	mov	edi,_out_of_memory
	jmp	fatal_error
main_file_not_found:
	mov	edi,_file_not_found
	jmp	fatal_error
code_cannot_be_generated:
	mov	edi,_code_cannot_be_generated
	jmp	fatal_error
unexpected_end_of_file:
	mov	edi,_unexpected_end_of_file
	jmp	fatal_error
file_not_found:
	mov	edi,_file_not_found
	jmp	assembler_error
invalid_macro_arguments:
	mov	edi,_invalid_macro_arguments
	jmp	assembler_error
unexpected_characters:
	mov	edi,_unexpected_characters
	jmp	assembler_error
invalid_argument:
	mov	edi,_invalid_argument
	jmp	assembler_error
illegal_instruction:
	mov	edi,_illegal_instruction
	jmp	assembler_error
instruction_not_allowed:
	mov	edi,_instruction_not_allowed
	jmp	assembler_error
invalid_operand:
	mov	edi,_invalid_operand
	jmp	assembler_error
invalid_operand_size:
	mov	edi,_invalid_operand_size
	jmp	assembler_error
operand_size_not_specified:
	mov	edi,_operand_size_not_specified
	jmp	assembler_error
operand_sizes_do_not_match:
	mov	edi,_operand_sizes_do_not_match
	jmp	assembler_error
address_sizes_do_not_agree:
	mov	edi,_address_sizes_do_not_agree
	jmp	assembler_error
invalid_expression:
	mov	edi,_invalid_expression
	jmp	assembler_error
invalid_address:
	mov	edi,_invalid_address
	jmp	assembler_error
invalid_value:
	mov	edi,_invalid_value
	jmp	assembler_error
invalid_use_of_symbol:
	mov	edi,_invalid_use_of_symbol
	jmp	assembler_error
value_out_of_range:
	mov	edi,_value_out_of_range
	jmp	assembler_error
relative_jump_out_of_range:
	mov	edi,_relative_jump_out_of_range
	jmp	assembler_error
extra_characters_on_line:
	mov	edi,_extra_characters_on_line
	jmp	assembler_error
name_too_long:
	mov	edi,_name_too_long
	jmp	assembler_error
argument_too_long:
	mov	edi,_argument_too_long
	jmp	assembler_error
reserved_word_used_as_symbol:
	mov	edi,_reserved_word_used_as_symbol
	jmp	assembler_error
symbol_already_defined:
	mov	edi,_symbol_already_defined
	jmp	assembler_error
missing_end_quote:
	mov	edi,_missing_end_quote
	jmp	assembler_error

fatal_error:
	mov	edx,error_prefix
	mov	ah,9
	int	21h
	mov	edx,edi
	int	21h
	mov	edx,cr_lf
	int	21h
	mov	ax,4CFFh
	int	21h
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
	mov	edx,line_number_end
	mov	ah,9
	int	21h
	mov	edx,[home_line]
	add	edx,5
	int	21h
	mov	edx,cr_lf
	int	21h
	mov	edx,error_prefix
	int	21h
	mov	edx,edi
	int	21h
	mov	edx,cr_lf
	int	21h
	mov	ax,4C01h
	int	21h
      display_line_number:
	mov	ecx,ebp
	shr	ecx,24
	dec	ecx
	mov	esi,[files_list]
	inc	esi
      get_error_file:
	jecxz	error_file_found
      skip_file_name:
	lodsb
	or	al,al
	jnz	skip_file_name
	add	esi,5
	loop	get_error_file
      error_file_found:
	mov	edx,esi
	mov	ah,9
	int	21h
	mov	edx,line_number_start
	int	21h
	mov	eax,ebp
	and	eax,not 11111111b shl 24
	call	display_number
	mov	dl,']'
	mov	ah,2
	int	21h
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

error_prefix db 'error: ',0
cr_lf db 0Dh,0Ah,0
line_number_start db ' [',0
line_number_end db ':',0Dh,0Ah,0
_out_of_memory db 'out of memory',0
_code_cannot_be_generated db 'code cannot be generated',0
_unexpected_end_of_file db 'unexpected end of file',0
_file_not_found db 'file not found',0
_invalid_macro_arguments db 'invalid macro arguments',0
_unexpected_characters db 'unexpected characters',0
_invalid_argument db 'invalid argument',0
_illegal_instruction db 'illegal instruction',0
_instruction_not_allowed db 'instruction not allowed',0
_invalid_operand db 'invalid operand',0
_invalid_operand_size db 'invalid size of operand',0
_operand_size_not_specified db 'operand size not specified',0
_operand_sizes_do_not_match db 'operand sizes do not match',0
_address_sizes_do_not_agree db 'address sizes do not agree',0
_invalid_expression db 'invalid expression',0
_invalid_address db 'invalid address',0
_invalid_value db 'invalid value',0
_invalid_use_of_symbol db 'invalid use of symbol',0
_value_out_of_range db 'value out of range',0
_relative_jump_out_of_range db 'relative jump out of range',0
_extra_characters_on_line db 'extra characters on line',0
_name_too_long db 'name too long',0
_argument_too_long db 'argument too long',0
_reserved_word_used_as_symbol db 'reserved word used as symbol',0
_symbol_already_defined db 'symbol already defined',0
_missing_end_quote db 'missing end quote',0
