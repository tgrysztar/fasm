
; flat assembler interface for DOS
; Copyright (c) 1999-2019, Tomasz Grysztar.
; All rights reserved.

go32:
	use16
	call	modes:real32
	use32
	retw

program_base dd ?
buffer_address dd ?
psp_segment dw ?
environment_segment dw ?

if UNREAL_ENABLED>0

init_memory:
	mov	[stack_limit],0
	cmp	[mode],dpmi
	je	init_dpmi_memory
	call	modes:init_real32_memory
	ret
dos_int:
	cmp	[mode],dpmi
	je	dpmi_dos_int
	stc
	int	21h
	ret
dos_int_with_buffer:
	cmp	[mode],dpmi
	je	dpmi_dos_int_with_buffer
	push	ds buffer
	pop	ds
	stc
	int	21h
	pop	ds
	ret
exit_program:
	cmp	[mode],dpmi
	je	exit_state_ok
	push	eax
	call	modes:free_real32_memory
	pop	eax
    exit_state_ok:
	mov	ah,4Ch
	int	21h

else

init_memory:
	mov	[stack_limit],0
	jmp	init_dpmi_memory
dos_int:
	jmp	dpmi_dos_int
dos_int_with_buffer:
	jmp	dpmi_dos_int_with_buffer
exit_program:
	mov	ah,4Ch
	int	21h

end if

get_environment_variable:
	mov	ebx,esi
	push	ds
	mov	ds,[environment_segment]
	xor	esi,esi
      compare_variable_names:
	mov	edx,ebx
      compare_character:
	lodsb
	mov	ah,[es:edx]
	inc	edx
	cmp	al,'='
	je	end_of_variable_name
	or	ah,ah
	jz	next_variable
	sub	ah,al
	jz	compare_character
	cmp	ah,20h
	jne	next_variable
	cmp	al,41h
	jb	next_variable
	cmp	al,5Ah
	jna	compare_character
      next_variable:
	lodsb
	or	al,al
	jnz	next_variable
	cmp	byte [esi],0
	jne	compare_variable_names
	pop	ds
	ret
      end_of_variable_name:
	or	ah,ah
	jnz	next_variable
      copy_variable_value:
	lodsb
	cmp	edi,[es:memory_end]
	jae	out_of_memory
	stosb
	or	al,al
	jnz	copy_variable_value
	dec	edi
	pop	ds
	ret

open:
	push	esi edi
	call	adapt_path
	mov	ax,716Ch
	mov	bx,100000b
	mov	dx,1
	xor	cx,cx
	xor	si,si
	call	dos_int_with_buffer
	jnc	open_done
	cmp	ax,7100h
	je	old_open
	stc
	jmp	open_done
      old_open:
	mov	ax,3D00h
	xor	dx,dx
	call	dos_int_with_buffer
      open_done:
	mov	bx,ax
	pop	edi esi
	ret
    adapt_path:
	mov	esi,edx
	mov	edi,[buffer_address]
      copy_path:
	lodsb
	cmp	al,'/'
	jne	path_char_ok
	mov	al,'\'
      path_char_ok:
	stosb
	or	al,al
	jnz	copy_path
	ret
create:
	push	esi edi
	call	adapt_path
	mov	ax,716Ch
	mov	bx,100001b
	mov	dx,10010b
	xor	cx,cx
	xor	si,si
	xor	di,di
	call	dos_int_with_buffer
	jnc	create_done
	cmp	ax,7100h
	je	old_create
	stc
	jmp	create_done
      old_create:
	mov	ah,3Ch
	xor	cx,cx
	xor	dx,dx
	call	dos_int_with_buffer
      create_done:
	mov	bx,ax
	pop	edi esi
	ret
write:
	push	edx esi edi ebp
	mov	ebp,ecx
	mov	esi,edx
      .loop:
	mov	ecx,1000h
	sub	ebp,1000h
	jnc	.write
	add	ebp,1000h
	mov	ecx,ebp
	xor	ebp,ebp
      .write:
	push	ecx
	mov	edi,[buffer_address]
	shr	ecx,2
	rep	movsd
	mov	ecx,[esp]
	and	ecx,11b
	rep	movsb
	pop	ecx
	mov	ah,40h
	xor	dx,dx
	call	dos_int_with_buffer
	or	ebp,ebp
	jnz	.loop
	pop	ebp edi esi edx
	ret
read:
	push	edx esi edi ebp
	mov	ebp,ecx
	mov	edi,edx
      .loop:
	mov	ecx,1000h
	sub	ebp,1000h
	jnc	.read
	add	ebp,1000h
	mov	ecx,ebp
	xor	ebp,ebp
      .read:
	push	ecx
	mov	ah,3Fh
	xor	dx,dx
	call	dos_int_with_buffer
	cmp	ax,cx
	jne	.eof
	mov	esi,[buffer_address]
	mov	ecx,[esp]
	shr	ecx,2
	rep	movsd
	pop	ecx
	and	ecx,11b
	rep	movsb
	or	ebp,ebp
	jnz	.loop
      .exit:
	pop	ebp edi esi edx
	ret
      .eof:
	pop	ecx
	stc
	jmp	.exit
close:
	mov	ah,3Eh
	int	21h
	ret
lseek:
	mov	ah,42h
	mov	ecx,edx
	shr	ecx,16
	int	21h
	pushf
	shl	edx,16
	popf
	mov	dx,ax
	mov	eax,edx
	ret

display_string:
	lods	byte [esi]
	or	al,al
	jz	string_end
	mov	dl,al
	mov	ah,2
	int	21h
	jmp	display_string
      string_end:
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
	mov	ah,2
	int	21h
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
	cmp	[displayed_count],1
	jb	line_break_ok
	je	make_line_break
	mov	ax,word [last_displayed]
	cmp	ax,0A0Dh
	je	line_break_ok
	cmp	ax,0D0Ah
	je	line_break_ok
      make_line_break:
	mov	ah,2
	mov	dl,0Dh
	int	21h
	mov	dl,0Ah
	int	21h
      line_break_ok:
	ret
display_block:
	add	[displayed_count],ecx
	cmp	ecx,1
	ja	take_last_two_characters
	jb	display_character
	mov	al,[last_displayed+1]
	mov	ah,[esi]
	mov	word [last_displayed],ax
	jmp	display_character
      take_last_two_characters:
	mov	ax,[esi+ecx-2]
	mov	word [last_displayed],ax
      display_character:
	lods	byte [esi]
	mov	dl,al
	mov	ah,2
	int	21h
	loopd	display_character
	ret

fatal_error:
	mov	esi,error_prefix
	call	display_string
	pop	esi
	call	display_string
	mov	esi,error_suffix
	call	display_string
	mov	al,0FFh
	jmp	exit_program
assembler_error:
	call	display_user_messages
	mov	ebx,[current_line]
	test	ebx,ebx
	jz	display_error_message
	pushd	0
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
	mov	ah,2
	int	21h
	pop	esi
	cmp	ebx,esi
	je	line_number_ok
	mov	dl,20h
	mov	ah,2
	int	21h
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
	mov	ah,2
	int	21h
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
	mov	[counter],eax
	xor	al,al
	call	lseek
	mov	esi,[additional_memory]
      read_line_data:
	mov	ecx,100h
	cmp	ecx,[counter]
	jbe	bytes_count_ok
	mov	ecx,[counter]
      bytes_count_ok:
	sub	[counter],ecx
	lea	eax,[esi+ecx]
	cmp	eax,[additional_memory_end]
	ja	out_of_memory
	push	ecx
	mov	edx,esi
	call	read
	pop	ecx
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
	cmp	[counter],0
	ja	read_line_data
      display_line_data:
	call	close
	mov	ecx,esi
	mov	esi,[additional_memory]
	sub	ecx,esi
	call	display_block
      line_data_displayed:
	mov	esi,cr_lf
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
	mov	esi,cr_lf
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
	mov	ah,2Ah
	int	21h
	push	dx cx
	movzx	ecx,cx
	mov	eax,ecx
	sub	eax,1970
	mov	ebx,365
	mul	ebx
	mov	ebp,eax
	mov	eax,ecx
	sub	eax,1969
	shr	eax,2
	add	ebp,eax
	mov	eax,ecx
	sub	eax,1901
	mov	ebx,100
	div	ebx
	sub	ebp,eax
	mov	eax,ecx
	xor	edx,edx
	sub	eax,1601
	mov	ebx,400
	div	ebx
	add	ebp,eax
	movzx	ecx,byte [esp+3]
	mov	eax,ecx
	dec	eax
	mov	ebx,30
	mul	ebx
	add	ebp,eax
	cmp	ecx,8
	jbe	months_correction
	mov	eax,ecx
	sub	eax,7
	shr	eax,1
	add	ebp,eax
	mov	ecx,8
      months_correction:
	mov	eax,ecx
	shr	eax,1
	add	ebp,eax
	cmp	ecx,2
	pop	cx
	jbe	day_correction_ok
	sub	ebp,2
	test	ecx,11b
	jnz	day_correction_ok
	xor	edx,edx
	mov	eax,ecx
	mov	ebx,100
	div	ebx
	or	edx,edx
	jnz	day_correction
	mov	eax,ecx
	mov	ebx,400
	div	ebx
	or	edx,edx
	jnz	day_correction_ok
      day_correction:
	inc	ebp
      day_correction_ok:
	pop	dx
	movzx	eax,dl
	dec	eax
	add	eax,ebp
	mov	ebx,24
	mul	ebx
	push	eax
	mov	ah,2Ch
	int	21h
	pop	eax
	push	dx
	movzx	ebx,ch
	add	eax,ebx
	mov	ebx,60
	mul	ebx
	movzx	ebx,cl
	add	eax,ebx
	mov	ebx,60
	mul	ebx
	pop	bx
	movzx	ebx,bh
	add	eax,ebx
	adc	edx,0
	ret
