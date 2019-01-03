
; flat assembler interface for Unix/libc
; Copyright (c) 1999-2019, Tomasz Grysztar.
; All rights reserved.

	format	ELF
	public	main

macro ccall proc,[arg]
  { common
     local size 
     size = 0 
     mov ebp,esp
     if ~ arg eq
    forward
     size = size + 4
    common
     sub esp,size
     end if
     and esp,-16
     if ~ arg eq
     add esp,size
    reverse
     pushd arg
    common
     end if
     call proc 
     mov esp,ebp }

extrn gettimeofday

section '.text' executable align 16

main:
	mov	ecx,[esp+4]
	mov	[argc],ecx
	mov	ebx,[esp+8]
	mov	[argv],ebx
	push	ebp
	mov	[stack_frame],esp

	mov	[con_handle],1
	mov	esi,_logo
	call	display_string

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

	ccall	gettimeofday,buffer,0
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
	ccall	gettimeofday,buffer,0
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
	mov	[input_file],0
	mov	[output_file],0
	mov	[symbols_file],0
	mov	[memory_setting],0
	mov	[passes_limit],100

	mov	ecx,[argc]
	mov	ebx,[argv]
	add	ebx,4
	dec	ecx
	jz	bad_params
	mov	[definitions_pointer],predefinitions
      get_param:
	mov	esi,[ebx]
	mov	al,[esi]
	cmp	al,'-'
	je	option_param
	cmp	[input_file],0
	jne	get_output_file
	mov	[input_file],esi
	jmp	next_param
      get_output_file:
	cmp	[output_file],0
	jne	bad_params
	mov	[output_file],esi
	jmp	next_param
      option_param:
	inc	esi
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
	cmp	byte [esi],0
	jne	get_memory_setting
	dec	ecx
	jz	bad_params
	add	ebx,4
	mov	esi,[ebx]
      get_memory_setting:
	call	get_option_value
	or	edx,edx
	jz	bad_params
	cmp	edx,1 shl (32-10)
	jae	bad_params
	mov	[memory_setting],edx
	jmp	next_param
      passes_option:
	cmp	byte [esi],0
	jne	get_passes_setting
	dec	ecx
	jz	bad_params
	add	ebx,4
	mov	esi,[ebx]
      get_passes_setting:
	call	get_option_value
	or	edx,edx
	jz	bad_params
	cmp	edx,10000h
	ja	bad_params
	mov	[passes_limit],dx
      next_param:
	add	ebx,4
	dec	ecx
	jnz	get_param
	cmp	[input_file],0
	je	bad_params
	mov	eax,[definitions_pointer]
	mov	byte [eax],0
	mov	[initial_definitions],predefinitions
	clc
	ret
      definition_option:
	cmp	byte [esi],0
	jne	get_definition
	dec	ecx
	jz	bad_params
	add	ebx,4
	mov	esi,[ebx]
      get_definition:
	push	edi
	mov	edi,[definitions_pointer]
	call	convert_definition_option
	mov	[definitions_pointer],edi
	pop	edi
	jc	bad_params
	jmp	next_param
      symbols_option:
	cmp	byte [esi],0
	jne	get_symbols_setting
	dec	ecx
	jz	bad_params
	add	ebx,4
	mov	esi,[ebx]
      get_symbols_setting:
	mov	[symbols_file],esi
	jmp	next_param
      get_option_value:
	xor	eax,eax
	mov	edx,eax
      get_option_digit:
	lodsb
	cmp	al,20h
	je	option_value_ok
	cmp	al,0Dh
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
	dec	esi
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
	dec	esi
	cmp	edi,predefinitions+1000h
	jae	bad_definition_option
	xor	al,al
	stosb
	clc
	ret

include 'system.inc'

include '..\version.inc'

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
_memory_suffix db ' kilobytes memory)',0xA,0
_passes_suffix db ' passes, ',0
_seconds_suffix db ' seconds, ',0
_bytes_suffix db ' bytes.',0xA,0

include '..\errors.inc'
include '..\symbdump.inc'
include '..\preproce.inc'
include '..\parser.inc'
include '..\exprpars.inc'
include '..\assemble.inc'
include '..\exprcalc.inc'
include '..\formats.inc'
include '..\x86_64.inc'
include '..\avx.inc'

include '..\tables.inc'
include '..\messages.inc'

section '.bss' writeable align 4

include '..\variable.inc'

argc dd ?
argv dd ?
stack_frame dd ?
memory_setting dd ?
definitions_pointer dd ?
start_time dd ?
timestamp dq ?
con_handle dd ?
displayed_count dd ?
last_displayed db ?
character db ?
preprocessing_done db ?

predefinitions rb 1000h
buffer rb 1000h
