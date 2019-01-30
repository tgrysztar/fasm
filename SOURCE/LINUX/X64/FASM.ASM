
; flat assembler interface for Linux x64
; Copyright (c) 1999-2019, Tomasz Grysztar.
; All rights reserved.

	format	ELF64 executable 3 at 400000h
	entry	start

	include 'modes.inc'

segment readable executable

start:

	mov	[con_handle],1
	mov	esi,_logo
	call	display_string

	mov	[command_line],rsp
	mov	rcx,[rsp]
	mov	rbx,[rsp+8+rcx*8+8]
	mov	[environment],rbx
	call	get_params
	jc	information

	call	init_memory

	mov	esi,_memory_prefix
	call	display_string
	mov	eax,[memory_end]
	sub	eax,[memory_start]
	add	eax,[additional_memory_end]
	sub	eax,[additional_memory]
	shr	eax,10
	call	display_number
	mov	esi,_memory_suffix
	call	display_string

	mov	eax,96
	mov	edi,buffer
	xor	esi,esi
	syscall
	mov	eax,dword [buffer]
	mov	ecx,1000
	mul	ecx
	mov	ebx,eax
	mov	eax,dword [buffer+4]
	div	ecx
	add	eax,ebx
	mov	[start_time],eax

	and	[preprocessing_done],0
	call	preprocessor
	or	[preprocessing_done],-1
	call	parser
	call	assembler
	call	formatter

	call	display_user_messages
	movzx	eax,[current_pass]
	inc	eax
	call	display_number
	mov	esi,_passes_suffix
	call	display_string
	mov	eax,96
	mov	edi,buffer
	xor	esi,esi
	syscall
	mov	eax,dword [buffer]
	mov	ecx,1000
	mul	ecx
	mov	ebx,eax
	mov	eax,dword [buffer+4]
	div	ecx
	add	eax,ebx
	sub	eax,[start_time]
	jnc	time_ok
	add	eax,3600000
      time_ok:
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

get_params:
	mov	rbx,[command_line]
	mov	[input_file],0
	mov	[output_file],0
	mov	[symbols_file],0
	mov	[memory_setting],0
	mov	[passes_limit],100
	mov	rcx,[rbx]
	add	rbx,8*2
	dec	rcx
	jz	bad_params
	mov	[definitions_pointer],predefinitions
	mov	[path_pointer],paths
      get_param:
	mov	rsi,[rbx]
	mov	al,[rsi]
	cmp	al,'-'
	je	option_param
	cmp	[input_file],0
	jne	get_output_file
	call	collect_path
	mov	[input_file],edx
	jmp	next_param
      get_output_file:
	cmp	[output_file],0
	jne	bad_params
	call	collect_path
	mov	[output_file],edx
	jmp	next_param
      option_param:
	inc	rsi
	lodsb
	cmp	al,'m'
	je	memory_option
	cmp	al,'M'
	je	memory_option
	cmp	al,'p'
	je	passes_option
	cmp	al,'P'
	je	passes_option
	cmp	al,'d'
	je	definition_option
	cmp	al,'D'
	je	definition_option
	cmp	al,'s'
	je	symbols_option
	cmp	al,'S'
	je	symbols_option
      bad_params:
	stc
	ret
      memory_option:
	cmp	byte [rsi],0
	jne	get_memory_setting
	dec	rcx
	jz	bad_params
	add	rbx,8
	mov	rsi,[rbx]
      get_memory_setting:
	call	get_option_value
	or	edx,edx
	jz	bad_params
	cmp	edx,1 shl (32-10)
	jae	bad_params
	mov	[memory_setting],edx
	jmp	next_param
      passes_option:
	cmp	byte [rsi],0
	jne	get_passes_setting
	dec	rcx
	jz	bad_params
	add	rbx,8
	mov	rsi,[rbx]
      get_passes_setting:
	call	get_option_value
	or	edx,edx
	jz	bad_params
	cmp	edx,10000h
	ja	bad_params
	mov	[passes_limit],dx
      next_param:
	add	rbx,8
	dec	rcx
	jnz	get_param
	cmp	[input_file],0
	je	bad_params
	mov	eax,[definitions_pointer]
	mov	byte [eax],0
	mov	[initial_definitions],predefinitions
	clc
	ret
      definition_option:
	cmp	byte [rsi],0
	jne	get_definition
	dec	rcx
	jz	bad_params
	add	rbx,8
	mov	rsi,[rbx]
      get_definition:
	mov	r12d,edi
	mov	edi,[definitions_pointer]
	call	convert_definition_option
	mov	[definitions_pointer],edi
	mov	edi,r12d
	jc	bad_params
	jmp	next_param
      symbols_option:
	cmp	byte [rsi],0
	jne	get_symbols_setting
	dec	rcx
	jz	bad_params
	add	rbx,8
	mov	rsi,[rbx]
      get_symbols_setting:
	call	collect_path
	mov	[symbols_file],edx
	jmp	next_param
      get_option_value:
	xor	eax,eax
	mov	edx,eax
      get_option_digit:
	lodsb
	cmp	al,20h
	je	option_value_ok
	or	al,al
	jz	option_value_ok
	sub	al,30h
	jc	invalid_option_value
	cmp	al,9
	ja	invalid_option_value
	imul	edx,10
	jo	invalid_option_value
	add	edx,eax
	jc	invalid_option_value
	jmp	get_option_digit
      option_value_ok:
	dec	rsi
	clc
	ret
      invalid_option_value:
	stc
	ret
      convert_definition_option:
	mov	edx,edi
	cmp	edi,predefinitions+1000h
	jae	bad_definition_option
	xor	al,al
	stosb
      copy_definition_name:
	lodsb
	cmp	al,'='
	je	copy_definition_value
	cmp	al,20h
	je	bad_definition_option
	or	al,al
	jz	bad_definition_option
	cmp	edi,predefinitions+1000h
	jae	bad_definition_option
	stosb
	inc	byte [edx]
	jnz	copy_definition_name
      bad_definition_option:
	stc
	ret
      copy_definition_value:
	lodsb
	cmp	al,20h
	je	definition_value_end
	or	al,al
	jz	definition_value_end
	cmp	edi,predefinitions+1000h
	jae	bad_definition_option
	stosb
	jmp	copy_definition_value
      definition_value_end:
	dec	rsi
	cmp	edi,predefinitions+1000h
	jae	bad_definition_option
	xor	al,al
	stosb
	clc
	ret
collect_path:
	mov	edi,[path_pointer]
	mov	edx,edi
     copy_path_to_low_memory:
	lodsb
	stosb
	test	al,al
	jnz	copy_path_to_low_memory
	mov	[path_pointer],edi
	retn

include 'system.inc'

include '..\..\version.inc'

_copyright db 'Copyright (c) 1999-2019, Tomasz Grysztar',0xA,0

_logo db 'flat assembler  version ',VERSION_STRING,0
_usage db 0xA
       db 'usage: fasm <source> [output]',0xA
       db 'optional settings:',0xA
       db ' -m <limit>         set the limit in kilobytes for the available memory',0xA
       db ' -p <limit>         set the maximum allowed number of passes',0xA
       db ' -d <name>=<value>  define symbolic variable',0xA
       db ' -s <file>          dump symbolic information for debugging',0xA
       db 0
_memory_prefix db '  (',0
_memory_suffix db ' kilobytes memory, x64)',0xA,0
_passes_suffix db ' passes, ',0
_seconds_suffix db ' seconds, ',0
_bytes_suffix db ' bytes.',0xA,0
_no_low_memory db 'failed to allocate memory within 32-bit addressing range',0

include '..\..\errors.inc'
include '..\..\symbdump.inc'
include '..\..\preproce.inc'
include '..\..\parser.inc'
include '..\..\exprpars.inc'
include '..\..\assemble.inc'
include '..\..\exprcalc.inc'
include '..\..\x86_64.inc'
include '..\..\avx.inc'
include '..\..\formats.inc'

include '..\..\tables.inc'
include '..\..\messages.inc'

segment readable writeable

align 4

include '..\..\variable.inc'

command_line dq ?
memory_setting dd ?
path_pointer dd ?
definitions_pointer dd ?
environment dq ?
timestamp dq ?
start_time dd ?
con_handle dd ?
displayed_count dd ?
last_displayed db ?
character db ?
preprocessing_done db ?

buffer rb 1000h
predefinitions rb 1000h
paths rb 10000h
