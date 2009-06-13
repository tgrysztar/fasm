
format PE console 4.0
entry start

include 'win32a.inc'

section '.data' data readable writeable

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

  display_handle dd ?
  bytes_count dd ?

  params rb 1000h

section '.text' code readable executable

  start:

	mov	[display_handle],STD_OUTPUT_HANDLE

	call	get_params
	jnc	make_dump

	mov	esi,_usage
	call	display_string
	invoke	ExitProcess,2

  make_dump:
	call	symbols
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
    bad_params:
	stc
	ret
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

  include '..\symbols.inc'

section '.idata' import data readable writeable

  library kernel32,'KERNEL32.DLL'

  include 'api\kernel32.inc'
