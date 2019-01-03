
; flat assembler interface for Unix/libc
; Copyright (c) 1999-2019, Tomasz Grysztar.
; All rights reserved.

extrn malloc
extrn free
extrn getenv
extrn fopen
extrn fclose
extrn fread
extrn fwrite
extrn fseek
extrn ftell
extrn time
extrn exit

extrn 'write' as libc_write

init_memory:
	mov	eax,esp
	and	eax,not 0FFFh
	add	eax,1000h-10000h
	mov	[stack_limit],eax
	mov	ecx,[memory_setting]
	shl	ecx,10
	jnz	allocate_memory
	mov	ecx,1000000h
      allocate_memory:
	mov	[memory_setting],ecx
	ccall	malloc,ecx
	or	eax,eax
	jz	out_of_memory
	mov	[additional_memory],eax
	add	eax,[memory_setting]
	mov	[memory_end],eax
	mov	eax,[memory_setting]
	shr	eax,2
	add	eax,[additional_memory]
	mov	[additional_memory_end],eax
	mov	[memory_start],eax
	ret

exit_program:
	movzx	eax,al
	push	eax
	ccall	free,[additional_memory]
	pop	eax
	ccall	exit,eax
	mov	esp,[stack_frame]
	pop	ebp
	ret

get_environment_variable:
	ccall	getenv,esi
	mov	esi,eax
	or	eax,eax
	jz	no_environment_variable
      copy_variable_value:
	lodsb
	cmp	edi,[memory_end]
	jae	out_of_memory
	stosb
	or	al,al
	jnz	copy_variable_value
	dec	edi
	ret
      no_environment_variable:
	stosb
	dec	edi
	ret

open:
	push	esi edi ebp
	call	adapt_path
	ccall	fopen,buffer,open_mode
	pop	ebp edi esi
	or	eax,eax
	jz	file_error
	mov	ebx,eax
	clc
	ret
    adapt_path:
	mov	esi,edx
	mov	edi,buffer
      copy_path:
	lods	byte [esi]
	cmp	al,'\'
	jne	path_char_ok
	mov	al,'/'
      path_char_ok:
	stos	byte [edi]
	or	al,al
	jnz	copy_path
	cmp	edi,buffer+1000h
	ja	out_of_memory
	ret
create:
	push	esi edi ebp
	call	adapt_path
	ccall	fopen,buffer,create_mode
	pop	ebp edi esi
	or	eax,eax
	jz	file_error
	mov	ebx,eax
	clc
	ret
close:
	ccall	fclose,ebx
	ret
read:
	push	ebx ecx edx esi edi ebp
	ccall	fread,edx,1,ecx,ebx
	pop	ebp edi esi edx ecx ebx
	cmp	eax,ecx
	jne	file_error
	clc
	ret
    file_error:
	stc
	ret
write:
	push	ebx ecx edx esi edi ebp
	ccall	fwrite,edx,1,ecx,ebx
	pop	ebp edi esi edx ecx ebx
	cmp	eax,ecx
	jne	file_error
	clc
	ret
lseek:
	push	ebx
	movzx	eax,al
	ccall	fseek,ebx,edx,eax
	test	eax,eax
	jnz	lseek_error
	mov	ebx,[esp]
	ccall	ftell,ebx
	pop	ebx
	clc
	ret
    lseek_error:
	pop	ebx
	stc
	ret

display_string:
	lodsb
	or	al,al
	jz	string_displayed
	mov	dl,al
	call	display_character
	jmp	display_string
      string_displayed:
	ret
display_character:
	mov	[character],dl
	ccall	libc_write,[con_handle],character,1
	ret
display_number:
	push	ebx
	mov	ecx,1000000000
	xor	edx,edx
	xor	bl,bl
      display_loop:
	div	ecx
	push	edx
	cmp	ecx,1
	je	display_digit
	or	bl,bl
	jnz	display_digit
	or	al,al
	jz	digit_ok
	not	bl
      display_digit:
	mov	dl,al
	add	dl,30h
	push	ebx ecx
	call	display_character
	pop	ecx ebx
      digit_ok:
	mov	eax,ecx
	xor	edx,edx
	mov	ecx,10
	div	ecx
	mov	ecx,eax
	pop	eax
	or	ecx,ecx
	jnz	display_loop
	pop	ebx
	ret

display_user_messages:
	mov	[displayed_count],0
	call	show_display_buffer
	cmp	[displayed_count],0
	je	line_break_ok
	cmp	[last_displayed],0Ah
	je	line_break_ok
	mov	dl,0Ah
	call	display_character
      line_break_ok:
	ret
display_block:
	jecxz	block_displayed
	add	[displayed_count],ecx
	mov	al,[esi+ecx-1]
	mov	[last_displayed],al
      display_characters:
	lodsb
	mov	dl,al
	push	ecx esi
	call	display_character
	pop	esi ecx
	loop	display_characters
      block_displayed:
	ret

fatal_error:
	mov	[con_handle],2
	mov	esi,error_prefix
	call	display_string
	pop	esi
	call	display_string
	mov	esi,error_suffix
	call	display_string
	mov	al,0FFh
	jmp	exit_program
assembler_error:
	mov	[con_handle],2
	call	display_user_messages
	mov	ebx,[current_line]
	test	ebx,ebx
	jz	display_error_message
	push	dword 0
      get_error_lines:
	mov	eax,[ebx]
	cmp	byte [eax],0
	je	get_next_error_line
	push	ebx
	test	byte [ebx+7],80h
	jz	display_error_line
	mov	edx,ebx
      find_definition_origin:
	mov	edx,[edx+12]
	test	byte [edx+7],80h
	jnz	find_definition_origin
	push	edx
      get_next_error_line:
	mov	ebx,[ebx+8]
	jmp	get_error_lines
      display_error_line:
	mov	esi,[ebx]
	call	display_string
	mov	esi,line_number_start
	call	display_string
	mov	eax,[ebx+4]
	and	eax,7FFFFFFFh
	call	display_number
	mov	dl,']'
	call	display_character
	pop	esi
	cmp	ebx,esi
	je	line_number_ok
	mov	dl,20h
	call	display_character
	push	esi
	mov	esi,[esi]
	movzx	ecx,byte [esi]
	inc	esi
	call	display_block
	mov	esi,line_number_start
	call	display_string
	pop	esi
	mov	eax,[esi+4]
	and	eax,7FFFFFFFh
	call	display_number
	mov	dl,']'
	call	display_character
      line_number_ok:
	mov	esi,line_data_start
	call	display_string
	mov	esi,ebx
	mov	edx,[esi]
	call	open
	mov	al,2
	xor	edx,edx
	call	lseek
	mov	edx,[esi+8]
	sub	eax,edx
	jz	line_data_displayed
	push	eax
	xor	al,al
	call	lseek
	mov	ecx,[esp]
	mov	edx,[additional_memory]
	lea	eax,[edx+ecx]
	cmp	eax,[additional_memory_end]
	ja	out_of_memory
	call	read
	call	close
	pop	ecx
	mov	esi,[additional_memory]
      get_line_data:
	mov	al,[esi]
	cmp	al,0Ah
	je	display_line_data
	cmp	al,0Dh
	je	display_line_data
	cmp	al,1Ah
	je	display_line_data
	or	al,al
	jz	display_line_data
	inc	esi
	loop	get_line_data
      display_line_data:
	mov	ecx,esi
	mov	esi,[additional_memory]
	sub	ecx,esi
	call	display_block
      line_data_displayed:
	mov	esi,lf
	call	display_string
	pop	ebx
	or	ebx,ebx
	jnz	display_error_line
	cmp	[preprocessing_done],0
	je	display_error_message
	mov	esi,preprocessed_instruction_prefix
	call	display_string
	mov	esi,[current_line]
	add	esi,16
	mov	edi,[additional_memory]
	xor	dl,dl
      convert_instruction:
	lodsb
	cmp	al,1Ah
	je	copy_symbol
	cmp	al,22h
	je	copy_symbol
	cmp	al,3Bh
	je	instruction_converted
	stosb
	or	al,al
	jz	instruction_converted
	xor	dl,dl
	jmp	convert_instruction
      copy_symbol:
	or	dl,dl
	jz	space_ok
	mov	byte [edi],20h
	inc	edi
      space_ok:
	cmp	al,22h
	je	quoted
	lodsb
	movzx	ecx,al
	rep	movsb
	or	dl,-1
	jmp	convert_instruction
      quoted:
	mov	al,27h
	stosb
	lodsd
	mov	ecx,eax
	jecxz	quoted_copied
      copy_quoted:
	lodsb
	stosb
	cmp	al,27h
	jne	quote_ok
	stosb
      quote_ok:
	loop	copy_quoted
      quoted_copied:
	mov	al,27h
	stosb
	or	dl,-1
	jmp	convert_instruction
      instruction_converted:
	xor	al,al
	stosb
	mov	esi,[additional_memory]
	call	display_string
	mov	esi,lf
	call	display_string
      display_error_message:
	mov	esi,error_prefix
	call	display_string
	pop	esi
	call	display_string
	mov	esi,error_suffix
	call	display_string
	mov	al,2
	jmp	exit_program

make_timestamp:
	ccall	time,timestamp
	mov	eax,dword [timestamp]
	mov	edx,dword [timestamp+4]
	ret

open_mode db 'r',0
create_mode db 'w',0

error_prefix db 'error: ',0
error_suffix db '.'
lf db 0xA,0
line_number_start db ' [',0
line_data_start db ':',0xA,0
preprocessed_instruction_prefix db 'processed: ',0
