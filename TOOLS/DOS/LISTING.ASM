
	format	MZ
	heap	0
	stack	8000h
	entry	loader:init

include 'loader.inc'

segment main use32

  start:

	call	get_params
	jnc	make_listing

	mov	esi,_usage
	call	display_string
	mov	ax,4C02h
	int	21h

  make_listing:
	call	listing
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
	cmp	al,'a'
	je	addresses_option
	cmp	al,'A'
	je	addresses_option
	cmp	al,'b'
	je	bytes_per_line_option
	cmp	al,'B'
	je	bytes_per_line_option
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
    bytes_per_line_option:
	lodsb
	cmp	al,20h
	je	bytes_per_line_option
	cmp	al,0Dh
	je	invalid_option
	or	al,al
	jz	invalid_option
	dec	esi
	call	get_option_value
	jc	bad_params
	or	edx,edx
	jz	invalid_option
	cmp	edx,1000
	ja	invalid_option
	mov	[es:code_bytes_per_line],edx
	jmp	find_param
    addresses_option:
	lodsb
	cmp	al,20h
	je	set_addresses_option
	cmp	al,0Dh
	je	set_addresses_option
	or	al,al
	jnz	bad_params
      set_addresses_option:
	dec	esi
	mov	[es:show_addresses],1
	jmp	find_param
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

  include '..\listing.inc'

  _usage db 'listing generator for flat assembler',0Dh,0Ah
	 db 'usage: listing <input> <output>',0Dh,0Ah
	 db 'optional settings:',0Dh,0Ah
	 db ' -a           show target addresses for assembled code',0Dh,0Ah
	 db ' -b <number>  set the amount of bytes listed per line',0Dh,0Ah
	 db 0
  _error_prefix db 'error: ',0
  _error_suffix db '.',0Dh,0Ah,0

  input_file dd 0
  output_file dd 0
  code_bytes_per_line dd 16
  show_addresses db 0

  line_break db 0Dh,0Ah

  input dd ?
  assembled_code dd ?
  assembled_code_length dd ?
  code_end dd ?
  code_offset dd ?
  code_length dd ?
  output_handle dd ?
  output_buffer dd ?
  current_source_file dd ?
  current_source_line dd ?
  source dd ?
  source_length dd ?
  maximum_address_length dd ?
  address_start dd ?
  last_listed_address dd ?

  psp_selector dw ?
  environment_selector dw ?

  memory_handles_count dd ?
  memory_handles rd 400h

  params rb 1000h
  characters rb 100h

segment buffer_segment

  buffer = (buffer_segment-main) shl 4

  db 1000h dup ?

segment stack_segment

  stack_bottom = (stack_segment-main) shl 4

  db 4000h dup ?

  stack_top = stack_bottom + $
