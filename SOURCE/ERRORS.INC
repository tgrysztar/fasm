
; flat assembler core
; Copyright (c) 1999-2009, Tomasz Grysztar.
; All rights reserved.

out_of_memory:
	push	_out_of_memory
	jmp	fatal_error
stack_overflow:
	push	_stack_overflow
	jmp	fatal_error
main_file_not_found:
	push	_main_file_not_found
	jmp	fatal_error
unexpected_end_of_file:
	push	_unexpected_end_of_file
	jmp	fatal_error
code_cannot_be_generated:
	push	_code_cannot_be_generated
	jmp	fatal_error
format_limitations_exceeded:
	push	_format_limitations_exceeded
	jmp	fatal_error
invalid_definition:
	push	_invalid_definition
	jmp	fatal_error
write_failed:
	push	_write_failed
	jmp	fatal_error

file_not_found:
	push	_file_not_found
	jmp	assembler_error
error_reading_file:
	push	_error_reading_file
	jmp	assembler_error
invalid_file_format:
	push	_invalid_file_format
	jmp	assembler_error
invalid_macro_arguments:
	push	_invalid_macro_arguments
	jmp	assembler_error
incomplete_macro:
	push	_incomplete_macro
	jmp	assembler_error
unexpected_characters:
	push	_unexpected_characters
	jmp	assembler_error
invalid_argument:
	push	_invalid_argument
	jmp	assembler_error
illegal_instruction:
	push	_illegal_instruction
	jmp	assembler_error
invalid_operand:
	push	_invalid_operand
	jmp	assembler_error
invalid_operand_size:
	push	_invalid_operand_size
	jmp	assembler_error
operand_size_not_specified:
	push	_operand_size_not_specified
	jmp	assembler_error
operand_sizes_do_not_match:
	push	_operand_sizes_do_not_match
	jmp	assembler_error
invalid_address_size:
	push	_invalid_address_size
	jmp	assembler_error
address_sizes_do_not_agree:
	push	_address_sizes_do_not_agree
	jmp	assembler_error
prefix_conflict:
	push	_prefix_conflict
	jmp	assembler_error
long_immediate_not_encodable:
	push	_long_immediate_not_encodable
	jmp	assembler_error
relative_jump_out_of_range:
	push	_relative_jump_out_of_range
	jmp	assembler_error
invalid_expression:
	push	_invalid_expression
	jmp	assembler_error
invalid_address:
	push	_invalid_address
	jmp	assembler_error
invalid_value:
	push	_invalid_value
	jmp	assembler_error
value_out_of_range:
	push	_value_out_of_range
	jmp	assembler_error
undefined_symbol:
	mov	edi,message
	mov	esi,_undefined_symbol
	call	copy_asciiz
	push	message
	cmp	[error_info],0
	je	assembler_error
	mov	byte [edi-1],20h
	call	write_quoted_symbol_name
	jmp	assembler_error
    copy_asciiz:
	lods	byte [esi]
	stos	byte [edi]
	test	al,al
	jnz	copy_asciiz
	ret
    write_quoted_symbol_name:
	mov	al,27h
	stosb
	mov	esi,[error_info]
	movzx	ecx,byte [esi-1]
	rep	movs byte [edi],[esi]
	mov	ax,27h
	stosw
	ret
symbol_out_of_scope:
	mov	edi,message
	mov	esi,_symbol_out_of_scope_1
	call	copy_asciiz
	cmp	[error_info],0
	je	finish_symbol_out_of_scope_message
	mov	byte [edi-1],20h
	call	write_quoted_symbol_name
    finish_symbol_out_of_scope_message:
	mov	byte [edi-1],20h
	mov	esi,_symbol_out_of_scope_2
	call	copy_asciiz
	push	message
	jmp	assembler_error
invalid_use_of_symbol:
	push	_invalid_use_of_symbol
	jmp	assembler_error
name_too_long:
	push	_name_too_long
	jmp	assembler_error
invalid_name:
	push	_invalid_name
	jmp	assembler_error
reserved_word_used_as_symbol:
	push	_reserved_word_used_as_symbol
	jmp	assembler_error
symbol_already_defined:
	push	_symbol_already_defined
	jmp	assembler_error
missing_end_quote:
	push	_missing_end_quote
	jmp	assembler_error
missing_end_directive:
	push	_missing_end_directive
	jmp	assembler_error
unexpected_instruction:
	push	_unexpected_instruction
	jmp	assembler_error
extra_characters_on_line:
	push	_extra_characters_on_line
	jmp	assembler_error
section_not_aligned_enough:
	push	_section_not_aligned_enough
	jmp	assembler_error
setting_already_specified:
	push	_setting_already_specified
	jmp	assembler_error
data_already_defined:
	push	_data_already_defined
	jmp	assembler_error
too_many_repeats:
	push	_too_many_repeats
	jmp	assembler_error
invoked_error:
	push	_invoked_error
	jmp	assembler_error