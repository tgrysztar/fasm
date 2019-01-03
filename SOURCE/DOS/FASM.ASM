
; flat assembler interface for DOS
; Copyright (c) 1999-2019, Tomasz Grysztar.
; All rights reserved.

	format	MZ
	heap	0
	stack	8000h
	entry	main:start

include 'modes.inc'

segment main use16

start:

	mov	ax,ds
	mov	dx,[2Ch]
	push	cs cs
	pop	ds es
	mov	[psp_segment],ax
	mov	[environment_segment],dx

	mov	dx,_logo
	mov	ah,9
	int	21h

	cld

	call	go32
	use32

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

	xor	ah,ah
	int	1Ah
	mov	ax,cx
	shl	eax,16
	mov	ax,dx
	mov	[start_time],eax

	cmp	[mode],dpmi
	je	compile
	jmp	main+(first_segment shr 4):first_gate-first_segment

compile:
	and	[preprocessing_done],0
	call	preprocessor
	or	[preprocessing_done],-1
	call	parser
	call	assembler
	call	formatter

finish:
	call	display_user_messages
	movzx	eax,[current_pass]
	inc	eax
	call	display_number
	mov	esi,_passes_suffix
	call	display_string
	xor	ah,ah
	int	1Ah
	mov	ax,cx
	shl	eax,16
	mov	ax,dx
	sub	eax,[start_time]
	mov	ebx,100
	mul	ebx
	mov	ebx,182
	div	ebx
	or	eax,eax
	jz	display_bytes_count
	xor	edx,edx
	mov	ebx,10
	div	ebx
	push	edx
	call	display_number
	mov	ah,2
	mov	dl,'.'
	int	21h
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
	mov	[definitions_pointer],predefinitions
	push	ds
	mov	ds,[psp_segment]
	mov	esi,81h
	mov	edi,params
    find_param:
	lodsb
	cmp	al,20h
	je	find_param
	cmp	al,'-'
	je	option_param
	cmp	al,0Dh
	je	all_params
	or	al,al
	jz	all_params
	cmp	[es:input_file],0
	jne	get_output_file
	mov	[es:input_file],edi
	jmp	process_param
      get_output_file:
	cmp	[es:output_file],0
	jne	bad_params
	mov	[es:output_file],edi
    process_param:
	cmp	al,22h
	je	string_param
    copy_param:
	stosb
	lodsb
	cmp	al,20h
	je	param_end
	cmp	al,0Dh
	je	param_end
	or	al,al
	jz	param_end
	jmp	copy_param
    string_param:
	lodsb
	cmp	al,22h
	je	string_param_end
	cmp	al,0Dh
	je	param_end
	or	al,al
	jz	param_end
	stosb
	jmp	string_param
    option_param:
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
    invalid_option:
	pop	ds
	stc
	ret
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
	jc	bad_params_value
	cmp	al,9
	ja	bad_params_value
	imul	edx,10
	jo	bad_params_value
	add	edx,eax
	jc	bad_params_value
	jmp	get_option_digit
    option_value_ok:
	dec	esi
	clc
	ret
    bad_params_value:
	stc
	ret
    memory_option:
	lodsb
	cmp	al,20h
	je	memory_option
	cmp	al,0Dh
	je	invalid_option
	or	al,al
	jz	invalid_option
	dec	esi
	call	get_option_value
	jc	invalid_option
	or	edx,edx
	jz	invalid_option
	cmp	edx,1 shl (32-10)
	jae	invalid_option
	mov	[es:memory_setting],edx
	jmp	find_param
    passes_option:
	lodsb
	cmp	al,20h
	je	passes_option
	cmp	al,0Dh
	je	invalid_option
	or	al,al
	jz	invalid_option
	dec	esi
	call	get_option_value
	jc	bad_params
	or	edx,edx
	jz	invalid_option
	cmp	edx,10000h
	ja	invalid_option
	mov	[es:passes_limit],dx
	jmp	find_param
    definition_option:
	lodsb
	cmp	al,20h
	je	definition_option
	cmp	al,0Dh
	je	bad_params
	or	al,al
	jz	bad_params
	dec	esi
	push	edi
	mov	edi,[es:definitions_pointer]
	call	convert_definition_option
	mov	[es:definitions_pointer],edi
	pop	edi
	jc	invalid_option
	jmp	find_param
    symbols_option:
	mov	[es:symbols_file],edi
      find_symbols_file_name:
	lodsb
	cmp	al,20h
	jne	process_param
	jmp	find_symbols_file_name
    param_end:
	dec	esi
    string_param_end:
	xor	al,al
	stosb
	jmp	find_param
    all_params:
	xor	al,al
	stosb
	pop	ds
	cmp	[input_file],0
	je	no_input_file
	mov	eax,[definitions_pointer]
	mov	byte [eax],0
	mov	[initial_definitions],predefinitions
	clc
	ret
    bad_params:
	pop	ds
    no_input_file:
	stc
	ret
    convert_definition_option:
	mov	ecx,edi
	xor	al,al
	stosb
      copy_definition_name:
	lodsb
	cmp	al,'='
	je	copy_definition_value
	cmp	al,20h
	je	bad_definition_option
	cmp	al,0Dh
	je	bad_definition_option
	or	al,al
	jz	bad_definition_option
	stosb
	inc	byte [es:ecx]
	jnz	copy_definition_name
      bad_definition_option:
	stc
	ret
      copy_definition_value:
	lodsb
	cmp	al,20h
	je	definition_value_end
	cmp	al,0Dh
	je	definition_value_end
	or	al,al
	jz	definition_value_end
	cmp	al,'\'
	jne	definition_value_character
	cmp	byte [esi],20h
	jne	definition_value_character
	lodsb
      definition_value_character:
	stosb
	jmp	copy_definition_value
      definition_value_end:
	dec	esi
	xor	al,al
	stosb
	clc
	ret

include '..\version.inc'

_logo db 'flat assembler  version ',VERSION_STRING,24h
_copyright db 'Copyright (c) 1999-2019, Tomasz Grysztar',0Dh,0Ah,0

_usage db 0Dh,0Ah
       db 'usage: fasm <source> [output]',0Dh,0Ah
       db 'optional settings:',0Dh,0Ah
       db ' -m <limit>         set the limit in kilobytes for the available memory',0Dh,0Ah
       db ' -p <limit>         set the maximum allowed number of passes',0Dh,0Ah
       db ' -d <name>=<value>  define symbolic variable',0Dh,0Ah
       db ' -s <file>          dump symbolic information for debugging',0Dh,0Ah
       db 0
_memory_prefix db '  (',0
_memory_suffix db ' kilobytes memory)',0Dh,0Ah,0
_passes_suffix db ' passes, ',0
_seconds_suffix db ' seconds, ',0
_bytes_suffix db ' bytes.',0Dh,0Ah,0

error_prefix db 'error: ',0
error_suffix db '.'
cr_lf db 0Dh,0Ah,0
line_number_start db ' [',0
line_data_start db ':',0Dh,0Ah,0
preprocessed_instruction_prefix db 'processed: ',0

include 'dpmi.inc'

align 16
first_segment:

include '..\preproce.inc'
include '..\parser.inc'
include '..\exprpars.inc'

align 16
second_segment:

include '..\exprcalc.inc'
include '..\errors.inc'
include '..\symbdump.inc'

include 'system.inc'

first_gate:
	and	[preprocessing_done],0
	call	preprocessor
	or	[preprocessing_done],-1
	call	parser
	jmp	main+(second_segment shr 4):second_gate-second_segment
first_segment_top = $ - first_segment

include '..\assemble.inc'
include '..\formats.inc'
include '..\x86_64.inc'
include '..\avx.inc'

second_gate:
	call	assembler
	call	formatter
	jmp	main:finish

second_segment_top = $ - second_segment

if first_segment_top>=10000h | second_segment_top>=10000h
 if UNREAL_ENABLED>0
  UNREAL_ENABLED = -1
 else
  UNREAL_ENABLED = 0
 end if
else
 if UNREAL_ENABLED<0
  UNREAL_ENABLED = -1
 else
  UNREAL_ENABLED = 1
 end if
end if

include '..\tables.inc'
include '..\messages.inc'

align 4

include '..\variable.inc'

memory_setting dd ?
start_time dd ?
definitions_pointer dd ?
params rb 100h
predefinitions rb 100h

mode dw ?
real_mode_segment dw ?
displayed_count dd ?
last_displayed rb 2
preprocessing_done db ?

segment buffer

rb 1000h
