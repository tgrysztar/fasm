
; flat assembler interface for Win32
; Copyright (c) 1999-2004, Tomasz Grysztar.
; All rights reserved.

	format	PE console

start:

	mov	esi,_logo
	call	display_string

	call	get_params
	jc	information

	call	init_memory

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
	xor	eax,eax
      convert_table:
	lodsb
	mov	byte [edi+eax],0
	loop	convert_table
	mov	[display_buffer],0

	call	[GetTickCount]
	mov	[start_time],eax

	call	preprocessor
	call	parser
	call	assembler
	call	formatter

	call	display_user_messages
	movzx	eax,[current_pass]
	inc	eax
	call	display_number
	mov	esi,_passes_suffix
	call	display_string
	call	[GetTickCount]
	sub	eax,[start_time]
	xor	edx,edx
	mov	ebx,100
	div	ebx
	or	eax,eax
	jz	display_bytes_count
	xor	edx,edx
	mov	ebx,10
	div	ebx
	push	edx
	call	display_number
	mov	dl,'.'
	call	display_character
	pop	eax
	call	display_number
	mov	esi,_seconds_suffix
	call	display_string
      display_bytes_count:
	mov	eax,[written_size]
	call	display_number
	mov	esi,_bytes_suffix
	call	display_string
	xor	al,al
	jmp	exit_program

information:
	mov	esi,_usage
	call	display_string
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

_copyright db 'Copyright (c) 1999-2002, Tomasz Grysztar',0Dh,0Ah,0

_logo db 'flat assembler  version ',VERSION_STRING,0Dh,0Ah,0
_usage db 'usage: fasm source [output]',0Dh,0Ah,0

_passes_suffix db ' passes, ',0
_seconds_suffix db ' seconds, ',0
_bytes_suffix db ' bytes.',0Dh,0Ah,0

_include db 'INCLUDE',0

_counter db 7,'0000000'

align 4

memory_setting dd ?
memory_start dd ?
memory_end dd ?
additional_memory dd ?
additional_memory_end dd ?
free_additional_memory dd ?
input_file dd ?
output_file dd ?
include_paths dd ?
source_start dd ?
code_start dd ?
code_size dd ?
real_code_size dd ?
start_time dd ?
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
fix_constants_count dd ?
equ_constants_count dd ?
anonymous_reverse dd ?
anonymous_forward dd ?
current_locals_prefix dd ?
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

passes_limit dw ?
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
mmx_size db ?
imm_sized db ?
jump_type db ?
operand_prefix db ?
nextbyte db ?

characters rb 100h
params rb 1000h
converted rb 100h
buffer rb 4000h

stack 4000h,4000h

section '.idata' import data readable writeable

  dd 0,0,0,rva kernel_name,rva kernel_table
  dd 0,0,0,0,0

  kernel_table:
    ExitProcess dd rva _ExitProcess
    CreateFile dd rva _CreateFileA
    ReadFile dd rva _ReadFile
    WriteFile dd rva _WriteFile
    CloseHandle dd rva _CloseHandle
    SetFilePointer dd rva _SetFilePointer
    GetCommandLine dd rva _GetCommandLineA
    GetEnvironmentVariable dd rva _GetEnvironmentVariable
    GetStdHandle dd rva _GetStdHandle
    VirtualAlloc dd rva _VirtualAlloc
    GetTickCount dd rva _GetTickCount
    GetSystemTime dd rva _GetSystemTime
    GlobalMemoryStatus dd rva _GlobalMemoryStatus
    dd 0

  kernel_name db 'KERNEL32.DLL',0

  _ExitProcess dw 0
    db 'ExitProcess',0
  _CreateFileA dw 0
    db 'CreateFileA',0
  _ReadFile dw 0
    db 'ReadFile',0
  _WriteFile dw 0
    db 'WriteFile',0
  _CloseHandle dw 0
    db 'CloseHandle',0
  _SetFilePointer dw 0
    db 'SetFilePointer',0
  _GetCommandLineA dw 0
    db 'GetCommandLineA',0
  _GetEnvironmentVariable dw 0
    db 'GetEnvironmentVariableA',0
  _GetStdHandle dw 0
    db 'GetStdHandle',0
  _VirtualAlloc dw 0
    db 'VirtualAlloc',0
  _GetTickCount dw 0
    db 'GetTickCount',0
  _GetSystemTime dw 0
    db 'GetSystemTime',0
  _GlobalMemoryStatus dw 0
    db 'GlobalMemoryStatus',0

section '.reloc' fixups data readable discardable
