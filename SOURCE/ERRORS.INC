
; flat assembler core
; Copyright (c) 1999-2005, Tomasz Grysztar.
; All rights reserved.

out_of_memory:
	call	fatal_error
	dm	"out of memory"
main_file_not_found:
	call	fatal_error
	dm	"source file not found"
unexpected_end_of_file:
	call	fatal_error
	dm	"unexpected end of file"
code_cannot_be_generated:
	call	fatal_error
	dm	"code cannot be generated"
format_limitations_exceeded:
	call	fatal_error
	dm	"format limitations exceeded"
write_failed:
	call	fatal_error
	dm	"write failed"
file_not_found:
	call	assembler_error
	dm	"file not found"
error_reading_file:
	call	assembler_error
	dm	"error reading file"
invalid_file_format:
	call	assembler_error
	dm	"invalid file format"
invalid_macro_arguments:
	call	assembler_error
	dm	"invalid macro arguments"
incomplete_macro:
	call	assembler_error
	dm	"incomplete macro"
unexpected_characters:
	call	assembler_error
	dm	"unexpected characters"
invalid_argument:
	call	assembler_error
	dm	"invalid argument"
illegal_instruction:
	call	assembler_error
	dm	"illegal instruction"
invalid_operand:
	call	assembler_error
	dm	"invalid operand"
invalid_operand_size:
	call	assembler_error
	dm	"invalid size of operand"
operand_size_not_specified:
	call	assembler_error
	dm	"operand size not specified"
operand_sizes_do_not_match:
	call	assembler_error
	dm	"operand sizes do not match"
invalid_address_size:
	call	assembler_error
	dm	"invalid size of address value"
address_sizes_do_not_agree:
	call	assembler_error
	dm	"address sizes do not agree"
prefix_conflict:
	call	assembler_error
	dm	"disallowed combination of registers"
long_immediate_not_encodable:
	call	assembler_error
	dm	"not encodable with long immediate"
relative_jump_out_of_range:
	call	assembler_error
	dm	"relative jump out of range"
invalid_expression:
	call	assembler_error
	dm	"invalid expression"
invalid_address:
	call	assembler_error
	dm	"invalid address"
invalid_value:
	call	assembler_error
	dm	"invalid value"
value_out_of_range:
	call	assembler_error
	dm	"value out of range"
undefined_symbol:
	call	assembler_error
	dm	"undefined symbol"
invalid_use_of_symbol:
	call	assembler_error
	dm	"invalid use of symbol"
name_too_long:
	call	assembler_error
	dm	"name too long"
invalid_name:
	call	assembler_error
	dm	"invalid name"
reserved_word_used_as_symbol:
	call	assembler_error
	dm	"reserved word used as symbol"
symbol_already_defined:
	call	assembler_error
	dm	"symbol already defined"
missing_end_quote:
	call	assembler_error
	dm	"missing end quote"
missing_end_directive:
	call	assembler_error
	dm	"missing end directive"
unexpected_instruction:
	call	assembler_error
	dm	"unexpected instruction"
extra_characters_on_line:
	call	assembler_error
	dm	"extra characters on line"
section_not_aligned_enough:
	call	assembler_error
	dm	"section is not aligned enough"
setting_already_specified:
	call	assembler_error
	dm	"setting already specified"
data_already_defined:
	call	assembler_error
	dm	"data already defined"
