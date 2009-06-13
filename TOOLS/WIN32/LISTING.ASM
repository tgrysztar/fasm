
format PE console 4.0
entry start

include 'win32a.inc'

section '.data' data readable writeable

  _usage db 'listing generator for flat assembler',0Dh,0Ah
	 db 'usage: listing <input> <output>',0Dh,0Ah
	 db 'optional settings:',0Dh,0Ah
	 db ' -a           show target addresses for assembled code',0Dh,0Ah
	 db ' -b <number>  set the amount of bytes listed per line',0Dh,0Ah
	 db 0
  _error_prefix db 'error: ',0
  _error_suffix db '.',0Dh,0Ah,0

  line_break db 0Dh,0Ah

  input_file dd 0
  output_file dd 0
  code_bytes_per_line dd 16
  show_addresses db 0

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

  display_handle dd ?
  bytes_count dd ?

  params rb 1000h
  characters rb 100h

section '.text' code readable executable

  start:

	mov	[display_handle],STD_OUTPUT_HANDLE

	call	get_params
	jnc	make_listing

	mov	esi,_usage
	call	display_string
	invoke	ExitProcess,2

  make_listing:
	call	listing
	invoke	ExitProcess,0

  error:
	mov	[display_handle],STD_ERROR_HANDLE
	mov	esi,_error_prefix
	call	display_string
	pop	esi
	call	display_string
	mov	esi,_error_suffix
	call	display_string
	invoke	ExitProcess,1

  get_params:
	invoke	GetCommandLine
	mov	esi,eax
	mov	edi,params
    find_command_start:
	lodsb
	cmp	al,20h
	je	find_command_start
	cmp	al,22h
	je	skip_quoted_name
    skip_name:
	lodsb
	cmp	al,20h
	je	find_param
	or	al,al
	jz	all_params
	jmp	skip_name
    skip_quoted_name:
	lodsb
	cmp	al,22h
	je	find_param
	or	al,al
	jz	all_params
	jmp	skip_quoted_name
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
	cmp	[input_file],0
	jne	get_output_file
	mov	[input_file],edi
	jmp	process_param
      get_output_file:
	cmp	[output_file],0
	jne	bad_params
	mov	[output_file],edi
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
    bad_params:
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
    bytes_per_line_option:
	lodsb
	cmp	al,20h
	je	bytes_per_line_option
	cmp	al,0Dh
	je	bad_params
	or	al,al
	jz	bad_params
	dec	esi
	call	get_option_value
	or	edx,edx
	jz	bad_params
	cmp	edx,1000
	ja	bad_params
	mov	[code_bytes_per_line],edx
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
	mov	[show_addresses],1
	jmp	find_param
    param_end:
	dec	esi
    string_param_end:
	xor	al,al
	stosb
	jmp	find_param
    all_params:
	cmp	[input_file],0
	je	bad_params
	cmp	[output_file],0
	je	bad_params
	clc
	ret

  include 'system.inc'

  include '..\listing.inc'

section '.idata' import data readable writeable

  library kernel32,'KERNEL32.DLL'

  include 'api\kernel32.inc'
