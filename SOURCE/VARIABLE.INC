
; flat assembler core variables
; Copyright (c) 1999-2019, Tomasz Grysztar.
; All rights reserved.

; Variables which have to be set up by interface:

memory_start dd ?
memory_end dd ?

additional_memory dd ?
additional_memory_end dd ?

stack_limit dd ?

initial_definitions dd ?
input_file dd ?
output_file dd ?
symbols_file dd ?

passes_limit dw ?

; Internal core variables:

current_pass dw ?

include_paths dd ?
free_additional_memory dd ?
source_start dd ?
code_start dd ?
code_size dd ?
real_code_size dd ?
written_size dd ?
headers_size dd ?

current_line dd ?
macro_line dd ?
macro_block dd ?
macro_block_line dd ?
macro_block_line_number dd ?
macro_symbols dd ?
struc_name dd ?
struc_label dd ?
instant_macro_start dd ?
parameters_end dd ?
default_argument_value dd ?
locals_counter rb 8
current_locals_prefix dd ?
anonymous_reverse dd ?
anonymous_forward dd ?
labels_list dd ?
label_hash dd ?
label_leaf dd ?
hash_tree dd ?
addressing_space dd ?
undefined_data_start dd ?
undefined_data_end dd ?
counter dd ?
counter_limit dd ?
error_info dd ?
error_line dd ?
error dd ?
tagged_blocks dd ?
structures_buffer dd ?
number_start dd ?
current_offset dd ?
value dq ?
fp_value rd 8
adjustment dq ?
symbol_identifier dd ?
address_symbol dd ?
address_high dd ?
uncompressed_displacement dd ?
format_flags dd ?
resolver_flags dd ?
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
image_base_high dd ?
merge_segment dd ?
resource_data dd ?
resource_size dd ?
actual_fixups_size dd ?
reserved_fixups dd ?
reserved_fixups_size dd ?
last_fixup_base dd ?
last_fixup_header dd ?
parenthesis_stack dd ?
blocks_stack dd ?
parsed_lines dd ?
logical_value_parentheses dd ?
file_extension dd ?

operand_size db ?
operand_flags db ?
operand_prefix db ?
rex_prefix db ?
opcode_prefix db ?
vex_required db ?
vex_register db ?
immediate_size db ?
mask_register db ?
broadcast_size db ?
rounding_mode db ?

base_code db ?
extended_code db ?
supplemental_code db ?
postbyte_register db ?
segment_register db ?
xop_opcode_map db ?

mmx_size db ?
jump_type db ?
push_size db ?
value_size db ?
address_size db ?
label_size db ?
size_declared db ?
address_size_declared db ?
displacement_compression db ?

value_undefined db ?
value_constant db ?
value_type db ?
value_sign db ?
fp_sign db ?
fp_format db ?
address_sign db ?
address_register db ?
compare_type db ?
logical_value_wrapping db ?
next_pass_needed db ?
output_format db ?
code_type db ?
adjustment_sign db ?
evex_mode db ?

macro_status db ?
skip_default_argument_value db ?
prefix_flags db ?
formatter_symbols_allowed db ?
decorator_symbols_allowed db ?
free_address_range db ?

characters rb 100h
converted rb 100h
message rb 180h
