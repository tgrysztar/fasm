
; flat assembler core variables
; Copyright (c) 1999-2005, Tomasz Grysztar.
; All rights reserved.

; Initialized variables:

locals_counter db 7,'0000000'

; Uninitialized variables which have to be set up by interface:

memory_start dd ?
memory_end dd ?

additional_memory dd ?
additional_memory_end dd ?

input_file dd ?
output_file dd ?

passes_limit dw ?

; Uninitialized internal core variables:

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
label_leaf dd ?
hash_tree dd ?
org_origin dq ?
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
adjustment dq ?
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

next_pass_needed db ?
macro_status db ?
parenthesis_stack db ?
output_format db ?
code_type db ?
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
mmx_size db ?
imm_sized db ?
jump_type db ?
operand_prefix db ?
nextbyte db ?

characters rb 100h
converted rb 100h
