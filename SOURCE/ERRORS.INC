
; flat assembler core
; Copyright (c) 1999-2019, Tomasz Grysztar.
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
write_failed:
	push	_write_failed
	jmp	fatal_error

unexpected_end_of_file:
	push	_unexpected_end_of_file
	jmp	general_error
code_cannot_be_generated:
	push	_code_cannot_be_generated
	jmp	general_error
format_limitations_exceeded:
	push	_format_limitations_exceeded
	jmp	general_error
invalid_definition:
	push	_invalid_definition
    general_error:
	cmp	[symbols_file],0
	je	fatal_error
	call	dump_preprocessed_source
	jmp	fatal_error

file_not_found:
	push	_file_not_found
	jmp	error_with_source
error_reading_file:
	push	_error_reading_file
	jmp	error_with_source
invalid_file_format:
	push	_invalid_file_format
	jmp	error_with_source
invalid_macro_arguments:
	push	_invalid_macro_arguments
	jmp	error_with_source
incomplete_macro:
	push	_incomplete_macro
	jmp	error_with_source
unexpected_characters:
	push	_unexpected_characters
	jmp	error_with_source
invalid_argument:
	push	_invalid_argument
	jmp	error_with_source
illegal_instruction:
	push	_illegal_instruction
	jmp	error_with_source
invalid_operand:
	push	_invalid_operand
	jmp	error_with_source
invalid_operand_size:
	push	_invalid_operand_size
	jmp	error_with_source
operand_size_not_specified:
	push	_operand_size_not_specified
	jmp	error_with_source
operand_sizes_do_not_match:
	push	_operand_sizes_do_not_match
	jmp	error_with_source
invalid_address_size:
	push	_invalid_address_size
	jmp	error_with_source
address_sizes_do_not_agree:
	push	_address_sizes_do_not_agree
	jmp	error_with_source
disallowed_combination_of_registers:
	push	_disallowed_combination_of_registers
	jmp	error_with_source
long_immediate_not_encodable:
	push	_long_immediate_not_encodable
	jmp	error_with_source
relative_jump_out_of_range:
	push	_relative_jump_out_of_range
	jmp	error_with_source
invalid_expression:
	push	_invalid_expression
	jmp	error_with_source
invalid_address:
	push	_invalid_address
	jmp	error_with_source
invalid_value:
	push	_invalid_value
	jmp	error_with_source
value_out_of_range:
	push	_value_out_of_range
	jmp	error_with_source
undefined_symbol:
	mov	edi,message
	mov	esi,_undefined_symbol
	call	copy_asciiz
	push	message
	cmp	[error_info],0
	je	error_with_source
	mov	esi,[error_info]
	mov	esi,[esi+24]
	or	esi,esi
	jz	error_with_source
	mov	byte [edi-1],20h
	call	write_quoted_symbol_name
	jmp	error_with_source
    copy_asciiz:
	lods	byte [esi]
	stos	byte [edi]
	test	al,al
	jnz	copy_asciiz
	ret
    write_quoted_symbol_name:
	mov	al,27h
	stosb
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
	mov	esi,[error_info]
	mov	esi,[esi+24]
	or	esi,esi
	jz	finish_symbol_out_of_scope_message
	mov	byte [edi-1],20h
	call	write_quoted_symbol_name
    finish_symbol_out_of_scope_message:
	mov	byte [edi-1],20h
	mov	esi,_symbol_out_of_scope_2
	call	copy_asciiz
	push	message
	jmp	error_with_source
invalid_use_of_symbol:
	push	_invalid_use_of_symbol
	jmp	error_with_source
name_too_long:
	push	_name_too_long
	jmp	error_with_source
invalid_name:
	push	_invalid_name
	jmp	error_with_source
reserved_word_used_as_symbol:
	push	_reserved_word_used_as_symbol
	jmp	error_with_source
symbol_already_defined:
	push	_symbol_already_defined
	jmp	error_with_source
missing_end_quote:
	push	_missing_end_quote
	jmp	error_with_source
missing_end_directive:
	push	_missing_end_directive
	jmp	error_with_source
unexpected_instruction:
	push	_unexpected_instruction
	jmp	error_with_source
extra_characters_on_line:
	push	_extra_characters_on_line
	jmp	error_with_source
section_not_aligned_enough:
	push	_section_not_aligned_enough
	jmp	error_with_source
setting_already_specified:
	push	_setting_already_specified
	jmp	error_with_source
data_already_defined:
	push	_data_already_defined
	jmp	error_with_source
too_many_repeats:
	push	_too_many_repeats
	jmp	error_with_source
assertion_failed:
	push	_assertion_failed
	jmp	error_with_source
invoked_error:
	push	_invoked_error
    error_with_source:
	cmp	[symbols_file],0
	je	assembler_error
	call	dump_preprocessed_source
	call	restore_preprocessed_source
	jmp	assembler_error
