
	format	ELF
	public	main

include 'ccall.inc'

section '.text' executable align 16

  main:
	mov	ecx,[esp+4]
	mov	[argc],ecx
	mov	ebx,[esp+8]
	mov	[argv],ebx

	mov	[display_handle],1

	call	get_params
	jnc	make_dump

	mov	esi,_usage
	call	display_string
	ccall	exit,2

  make_dump:
	call	preprocessed_source
	ccall	exit,0

  error:
	mov	[display_handle],2
	mov	esi,_error_prefix
	call	display_string
	pop	esi
	call	display_string
	mov	esi,_error_suffix
	call	display_string
	ccall	exit,0

  get_params:
	mov	ecx,[argc]
	mov	ebx,[argv]
	add	ebx,4
	dec	ecx
	jz	bad_params
      get_param:
	mov	esi,[ebx]
	mov	al,[esi]
	cmp	[input_file],0
	jne	get_output_file
	mov	[input_file],esi
	jmp	next_param
      get_output_file:
	cmp	[output_file],0
	jne	bad_params
	mov	[output_file],esi
	jmp	next_param
      bad_params:
	stc
	ret
      next_param:
	add	ebx,4
	dec	ecx
	jnz	get_param
	cmp	[input_file],0
	je	bad_params
	cmp	[output_file],0
	je	bad_params
	clc
	ret

  include 'system.inc'

  include '..\prepsrc.inc'

section '.data' writeable align 4

  input_file dd 0
  output_file dd 0

  _usage db 'preprocessed source dumper for flat assembler',0Dh,0Ah
	 db 'usage: prepsrc <input> <output>',0Dh,0Ah
	 db 0
  _error_prefix db 'error: ',0
  _error_suffix db '.',0Dh,0Ah,0

section '.bss' writeable align 4

  argc dd ?
  argv dd ?

  display_handle dd ?
  character db ?

  params rb 1000h
  buffer rb 1000h
