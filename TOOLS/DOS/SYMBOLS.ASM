
	format	MZ
	heap	0
	stack	8000h
	entry	loader:init

include 'loader.inc'

segment main use32

  start:

	call	get_params
	jnc	make_dump

	mov	esi,_usage
	call	display_string
	mov	ax,4C02h
	int	21h

  make_dump:
	call	symbols
	mov	ax,4C00h
	int	21h

  error:
	mov	esi,_error_prefix
	call	display_string
	pop	esi
	call	display_string
	mov	esi,_error_suffix
	call	display_string
	mov	ax,4C00h
	int	21h

  get_params:
	push	ds
	mov	ds,[psp_selector]
	mov	esi,81h
	mov	edi,params
    find_param:
	lodsb
	cmp	al,20h
	je	find_param
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
    bad_params_value:
	stc
	ret
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
	je	bad_params
	cmp	[output_file],0
	je	bad_params
	clc
	ret
    bad_params:
	stc
	ret

  include 'system.inc'

  include '..\symbols.inc'

  _usage db 'symbols dumper for flat assembler',0Dh,0Ah
	 db 'usage: symbols <input> <output>',0Dh,0Ah
	 db 0
  _error_prefix db 'error: ',0
  _error_suffix db '.',0Dh,0Ah,0

  input_file dd 0
  output_file dd 0

  input dd ?
  output_buffer dd ?
  output_handle dd ?

  psp_selector dw ?
  environment_selector dw ?

  memory_handles_count dd ?
  memory_handles rd 400h

  params rb 1000h

segment buffer_segment

  buffer = (buffer_segment-main) shl 4

  db 1000h dup ?

segment stack_segment

  stack_bottom = (stack_segment-main) shl 4

  db 4000h dup ?

  stack_top = stack_bottom + $
