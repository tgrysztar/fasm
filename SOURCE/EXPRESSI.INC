
; flat assembler core
; Copyright (c) 1999-2005, Tomasz Grysztar.
; All rights reserved.

convert_expression:
	push	ebp
	call	get_fp_value
	jnc	fp_expression
	mov	ebp,esp
      expression_loop:
	push	ebp edi
	mov	edi,single_operand_operators
	call	get_operator
	pop	edi
	push	eax
	mov	al,[esi]
	cmp	al,1Ah
	je	expression_number
	cmp	al,22h
	je	expression_number
	cmp	al,'('
	je	expression_number
	mov	al,11h
	stos	byte [edi]
	or	eax,-1
	stos	dword [edi]
	jmp	expression_number_ok
      expression_number:
	call	convert_number
      expression_number_ok:
	pop	eax
	or	al,al
	jz	expression_operator
	stos	byte [edi]
      expression_operator:
	push	edi
	mov	edi,operators
	call	get_operator
	pop	edi ebp
	or	al,al
	jz	expression_end
      operators_loop:
	cmp	esp,ebp
	je	push_operator
	mov	bl,al
	and	bl,0F0h
	mov	bh,byte [esp]
	and	bh,0F0h
	cmp	bl,bh
	ja	push_operator
	pop	ebx
	mov	byte [edi],bl
	inc	edi
	jmp	operators_loop
      push_operator:
	push	eax
	jmp	expression_loop
      expression_end:
	cmp	esp,ebp
	je	expression_converted
	pop	eax
	stos	byte [edi]
	jmp	expression_end
      expression_converted:
	pop	ebp
	ret
      fp_expression:
	mov	al,'.'
	stos	byte [edi]
	mov	eax,[fp_value]
	stos	dword [edi]
	mov	eax,[fp_value+4]
	stos	dword [edi]
	mov	eax,[fp_value+8]
	stos	dword [edi]
	pop	ebp
	ret

convert_number:
	cmp	byte [esi],'('
	je	expression_value
	inc	edi
	call	get_number
	jc	symbol_value
	or	ebp,ebp
	jz	valid_number
	mov	byte [edi-1],0Fh
	ret
      valid_number:
	cmp	dword [edi+4],0
	jne	qword_number
	cmp	word [edi+2],0
	jne	dword_number
	cmp	byte [edi+1],0
	jne	word_number
      byte_number:
	mov	byte [edi-1],1
	inc	edi
	ret
      qword_number:
	mov	byte [edi-1],8
	add	edi,8
	ret
      dword_number:
	mov	byte [edi-1],4
	scas	dword [edi]
	ret
      word_number:
	mov	byte [edi-1],2
	scas	word [edi]
	ret
      expression_value:
	mov	eax,esp
	sub	eax,100h
	jc	stack_overflow
	cmp	eax,[stack_limit]
	jb	stack_overflow
	inc	esi
	call	convert_expression
	lods	byte [esi]
	cmp	al,')'
	jne	invalid_expression
	ret
      symbol_value:
	lods	byte [esi]
	cmp	al,1Ah
	jne	invalid_value
	lods	byte [esi]
	movzx	ecx,al
	push	ecx esi edi
	mov	edi,address_registers
	call	get_symbol
	jnc	register_value
	mov	edi,directive_operators
	call	get_symbol
	pop	edi esi ecx
	jnc	broken_value
	call	get_label_id
      store_label_value:
	mov	byte [edi-1],11h
	stos	dword [edi]
	ret
      broken_value:
	sub	esi,2
	or	eax,-1
	jmp	store_label_value
      register_value:
	pop	edi
	add	esp,8
	mov	byte [edi-1],10h
	mov	al,ah
	stos	byte [edi]
	ret

get_number:
	xor	ebp,ebp
	lods	byte [esi]
	cmp	al,22h
	je	get_text_number
	cmp	al,1Ah
	jne	not_number
	lods	byte [esi]
	movzx	ecx,al
	mov	[number_start],esi
	mov	al,[esi]
	cmp	al,'$'
	je	number_begin
	sub	al,30h
	cmp	al,9
	ja	invalid_number
      number_begin:
	mov	ebx,esi
	add	esi,ecx
	push	esi
	dec	esi
	mov	dword [edi],0
	mov	dword [edi+4],0
	cmp	byte [ebx],'$'
	je	pascal_hex_number
	cmp	word [ebx],'0x'
	je	get_hex_number
	mov	al,[esi]
	dec	esi
	cmp	al,'h'
	je	get_hex_number
	cmp	al,'b'
	je	get_bin_number
	cmp	al,'d'
	je	get_dec_number
	cmp	al,'o'
	je	get_oct_number
	cmp	al,'H'
	je	get_hex_number
	cmp	al,'B'
	je	get_bin_number
	cmp	al,'D'
	je	get_dec_number
	cmp	al,'O'
	je	get_oct_number
	inc	esi
      get_dec_number:
	mov	ebx,esi
	mov	esi,[number_start]
      get_dec_digit:
	cmp	esi,ebx
	ja	number_ok
	xor	edx,edx
	mov	eax,[edi]
	shld	edx,eax,2
	shl	eax,2
	add	eax,[edi]
	adc	edx,0
	add	eax,eax
	adc	edx,edx
	mov	[edi],eax
	mov	eax,[edi+4]
	add	eax,eax
	jc	dec_out_of_range
	add	eax,eax
	jc	dec_out_of_range
	add	eax,[edi+4]
	jc	dec_out_of_range
	add	eax,eax
	jc	dec_out_of_range
	add	eax,edx
	jc	dec_out_of_range
	mov	[edi+4],eax
	movzx	eax,byte [esi]
	sub	al,30h
	cmp	al,9
	ja	bad_number
	add	[edi],eax
	adc	dword [edi+4],0
	jc	dec_out_of_range
	inc	esi
	jmp	get_dec_digit
      dec_out_of_range:
	or	ebp,-1
	inc	esi
	jmp	get_dec_digit
      bad_number:
	pop	eax
      invalid_number:
	mov	esi,[number_start]
	dec	esi
      not_number:
	dec	esi
	stc
	ret
      get_bin_number:
	xor	bl,bl
      get_bin_digit:
	cmp	esi,[number_start]
	jb	number_ok
	movzx	eax,byte [esi]
	sub	al,30h
	cmp	al,1
	ja	bad_number
	xor	edx,edx
	mov	cl,bl
	dec	esi
	cmp	bl,64
	je	bin_out_of_range
	inc	bl
	cmp	cl,32
	jae	bin_digit_high
	shl	eax,cl
	or	dword [edi],eax
	jmp	get_bin_digit
      bin_digit_high:
	sub	cl,32
	shl	eax,cl
	or	dword [edi+4],eax
	jmp	get_bin_digit
      bin_out_of_range:
	or	al,al
	jz	get_bin_digit
	or	ebp,-1
	jmp	get_bin_digit
      pascal_hex_number:
	cmp	cl,1
	je	bad_number
      get_hex_number:
	xor	bl,bl
      get_hex_digit:
	cmp	esi,[number_start]
	jb	number_ok
	movzx	eax,byte [esi]
	cmp	al,'x'
	je	hex_number_ok
	cmp	al,'$'
	je	pascal_hex_ok
	sub	al,30h
	cmp	al,9
	jbe	hex_digit_ok
	sub	al,7
	cmp	al,15
	jbe	hex_letter_digit_ok
	sub	al,20h
	cmp	al,15
	ja	bad_number
      hex_letter_digit_ok:
	cmp	al,10
	jb	bad_number
      hex_digit_ok:
	xor	edx,edx
	mov	cl,bl
	dec	esi
	cmp	bl,64
	je	hex_out_of_range
	add	bl,4
	cmp	cl,32
	jae	hex_digit_high
	shl	eax,cl
	or	dword [edi],eax
	jmp	get_hex_digit
      hex_digit_high:
	sub	cl,32
	shl	eax,cl
	or	dword [edi+4],eax
	jmp	get_hex_digit
      hex_out_of_range:
	or	al,al
	jz	get_hex_digit
	or	ebp,-1
	jmp	get_hex_digit
      get_oct_number:
	xor	bl,bl
      get_oct_digit:
	cmp	esi,[number_start]
	jb	number_ok
	movzx	eax,byte [esi]
	sub	al,30h
	cmp	al,7
	ja	bad_number
      oct_digit_ok:
	xor	edx,edx
	mov	cl,bl
	dec	esi
	cmp	bl,64
	jae	oct_out_of_range
	add	bl,3
	cmp	cl,30
	je	oct_digit_wrap
	ja	oct_digit_high
	shl	eax,cl
	or	dword [edi],eax
	jmp	get_oct_digit
      oct_digit_wrap:
	shl	eax,cl
	adc	dword [edi+4],0
	or	dword [edi],eax
	jmp	get_oct_digit
      oct_digit_high:
	sub	cl,32
	shl	eax,cl
	or	dword [edi+4],eax
	jmp	get_oct_digit
      oct_out_of_range:
	or	al,al
	jz	get_oct_digit
	or	ebp,-1
	jmp	get_oct_digit
      hex_number_ok:
	dec	esi
      pascal_hex_ok:
	cmp	esi,[number_start]
	jne	bad_number
      number_ok:
	pop	esi
      number_done:
	clc
	ret
      get_text_number:
	lods	dword [esi]
	mov	edx,eax
	xor	bl,bl
	mov	dword [edi],0
	mov	dword [edi+4],0
      get_text_character:
	sub	edx,1
	jc	number_done
	movzx	eax,byte [esi]
	inc	esi
	mov	cl,bl
	cmp	bl,64
	je	text_out_of_range
	add	bl,8
	cmp	cl,32
	jae	text_character_high
	shl	eax,cl
	or	dword [edi],eax
	jmp	get_text_character
      text_character_high:
	sub	cl,32
	shl	eax,cl
	or	dword [edi+4],eax
	jmp	get_text_character
      text_out_of_range:
	or	ebp,-1
	jmp	get_text_character

get_fp_value:
	push	edi esi
	lods	byte [esi]
	cmp	al,1Ah
	je	fp_value_start
	cmp	al,'-'
	je	fp_sign_ok
	cmp	al,'+'
	jne	not_fp_value
      fp_sign_ok:
	lods	byte [esi]
	cmp	al,1Ah
	jne	not_fp_value
      fp_value_start:
	lods	byte [esi]
	movzx	ecx,al
	cmp	cl,1
	jbe	not_fp_value
	lea	edx,[esi+1]
	xor	ah,ah
      check_fp_value:
	lods	byte [esi]
	cmp	al,'.'
	je	fp_character_dot
	cmp	al,'E'
	je	fp_character_exp
	cmp	al,'e'
	je	fp_character_exp
	cmp	al,'F'
	je	fp_last_character
	cmp	al,'f'
	je	fp_last_character
      digit_expected:
	cmp	al,'0'
	jb	not_fp_value
	cmp	al,'9'
	ja	not_fp_value
	jmp	fp_character_ok
      fp_character_dot:
	cmp	esi,edx
	je	not_fp_value
	or	ah,ah
	jnz	not_fp_value
	or	ah,1
	lods	byte [esi]
	loop	digit_expected
      not_fp_value:
	pop	esi edi
	stc
	ret
      fp_character_exp:
	cmp	esi,edx
	je	not_fp_value
	cmp	ah,1
	ja	not_fp_value
	or	ah,2
	cmp	ecx,1
	jne	fp_character_ok
	cmp	byte [esi],'+'
	je	fp_exp_sign
	cmp	byte [esi],'-'
	jne	fp_character_ok
      fp_last_character:
	cmp	cl,1
	jne	not_fp_value
	or	ah,4
	jmp	fp_character_ok
      fp_exp_sign:
	inc	esi
	cmp	byte [esi],1Ah
	jne	not_fp_value
	inc	esi
	lods	byte [esi]
	movzx	ecx,al
	inc	ecx
      fp_character_ok:
	dec	ecx
	jnz	check_fp_value
	or	ah,ah
	jz	not_fp_value
	pop	esi
	lods	byte [esi]
	mov	[fp_sign],0
	cmp	al,1Ah
	je	fp_get
	inc	esi
	cmp	al,'+'
	je	fp_get
	mov	[fp_sign],1
      fp_get:
	lods	byte [esi]
	movzx	ecx,al
	xor	edx,edx
	mov	edi,fp_value
	mov	[edi],edx
	mov	[edi+4],edx
	mov	[edi+12],edx
	call	fp_optimize
	mov	[fp_format],0
	mov	al,[esi]
      fp_before_dot:
	lods	byte [esi]
	cmp	al,'.'
	je	fp_dot
	cmp	al,'E'
	je	fp_exponent
	cmp	al,'e'
	je	fp_exponent
	cmp	al,'F'
	je	fp_done
	cmp	al,'f'
	je	fp_done
	sub	al,30h
	mov	edi,fp_value+16
	xor	edx,edx
	mov	dword [edi+12],edx
	mov	dword [edi],edx
	mov	dword [edi+4],edx
	mov	[edi+7],al
	mov	dl,7
	mov	dword [edi+8],edx
	call	fp_optimize
	mov	edi,fp_value
	push	ecx
	mov	ecx,10
	call	fp_mul
	pop	ecx
	mov	ebx,fp_value+16
	call	fp_add
	loop	fp_before_dot
      fp_dot:
	mov	edi,fp_value+16
	xor	edx,edx
	mov	[edi],edx
	mov	[edi+4],edx
	mov	byte [edi+7],80h
	mov	[edi+8],edx
	mov	dword [edi+12],edx
	dec	ecx
	jz	fp_done
      fp_after_dot:
	lods	byte [esi]
	cmp	al,'E'
	je	fp_exponent
	cmp	al,'e'
	je	fp_exponent
	cmp	al,'F'
	je	fp_done
	cmp	al,'f'
	je	fp_done
	inc	[fp_format]
	cmp	[fp_format],80h
	jne	fp_counter_ok
	mov	[fp_format],7Fh
      fp_counter_ok:
	dec	esi
	mov	edi,fp_value+16
	push	ecx
	mov	ecx,10
	call	fp_div
	push	dword [edi]
	push	dword [edi+4]
	push	dword [edi+8]
	push	dword [edi+12]
	lods	byte [esi]
	sub	al,30h
	movzx	ecx,al
	call	fp_mul
	mov	ebx,edi
	mov	edi,fp_value
	call	fp_add
	mov	edi,fp_value+16
	pop	dword [edi+12]
	pop	dword [edi+8]
	pop	dword [edi+4]
	pop	dword [edi]
	pop	ecx
	loop	fp_after_dot
	jmp	fp_done
      fp_exponent:
	or	[fp_format],80h
	xor	edx,edx
	xor	ebp,ebp
	dec	ecx
	jnz	get_exponent
	cmp	byte [esi],'+'
	je	fp_exponent_sign
	cmp	byte [esi],'-'
	jne	fp_done
	not	ebp
      fp_exponent_sign:
	add	esi,2
	lods	byte [esi]
	movzx	ecx,al
      get_exponent:
	movzx	eax,byte [esi]
	inc	esi
	sub	al,30h
	cmp	al,10
	jae	exponent_ok
	imul	edx,10
	cmp	edx,8000h
	jae	value_out_of_range
	add	edx,eax
	loop	get_exponent
      exponent_ok:
	mov	edi,fp_value
	or	edx,edx
	jz	fp_done
	mov	ecx,edx
	or	ebp,ebp
	jnz	fp_negative_power
      fp_power:
	push	ecx
	mov	ecx,10
	call	fp_mul
	pop	ecx
	loop	fp_power
	jmp	fp_done
      fp_negative_power:
	push	ecx
	mov	ecx,10
	call	fp_div
	pop	ecx
	loop	fp_negative_power
      fp_done:
	mov	edi,fp_value
	mov	al,[fp_format]
	mov	[edi+10],al
	mov	al,[fp_sign]
	mov	[edi+11],al
	test	byte [edi+15],80h
	jz	fp_ok
	add	dword [edi],1
	adc	dword [edi+4],0
	jnc	fp_ok
	mov	eax,[edi+4]
	shrd	[edi],eax,1
	shr	eax,1
	or	eax,80000000h
	mov	[edi+4],eax
	inc	word [edi+8]
      fp_ok:
	pop	edi
	clc
	ret
      fp_mul:
	or	ecx,ecx
	jz	fp_zero
	mov	eax,[edi+12]
	mul	ecx
	mov	[edi+12],eax
	mov	ebx,edx
	mov	eax,[edi]
	mul	ecx
	add	eax,ebx
	adc	edx,0
	mov	[edi],eax
	mov	ebx,edx
	mov	eax,[edi+4]
	mul	ecx
	add	eax,ebx
	adc	edx,0
	mov	[edi+4],eax
      .loop:
	or	edx,edx
	jz	.done
	mov	eax,[edi]
	shrd	[edi+12],eax,1
	mov	eax,[edi+4]
	shrd	[edi],eax,1
	shrd	eax,edx,1
	mov	[edi+4],eax
	shr	edx,1
	inc	dword [edi+8]
	cmp	dword [edi+8],8000h
	jge	value_out_of_range
	jmp	.loop
      .done:
	ret
      fp_div:
	mov	eax,[edi+4]
	xor	edx,edx
	div	ecx
	mov	[edi+4],eax
	mov	eax,[edi]
	div	ecx
	mov	[edi],eax
	mov	eax,[edi+12]
	div	ecx
	mov	[edi+12],eax
	mov	ebx,eax
	or	ebx,[edi]
	or	ebx,[edi+4]
	jz	fp_zero
      .loop:
	test	byte [edi+7],80h
	jnz	.exp_ok
	mov	eax,[edi]
	shld	[edi+4],eax,1
	mov	eax,[edi+12]
	shld	[edi],eax,1
	add	eax,eax
	mov	[edi+12],eax
	dec	dword [edi+8]
	add	edx,edx
	jmp	.loop
      .exp_ok:
	mov	eax,edx
	xor	edx,edx
	div	ecx
	add	[edi+12],eax
	adc	dword [edi],0
	adc	dword [edi+4],0
	jnc	.done
	mov	eax,[edi+4]
	mov	ebx,[edi]
	shrd	[edi],eax,1
	shrd	[edi+12],ebx,1
	shr	eax,1
	or	eax,80000000h
	mov	[edi+4],eax
	inc	dword [edi+8]
      .done:
	ret
      fp_add:
	cmp	dword [ebx+8],8000h
	je	.done
	cmp	dword [edi+8],8000h
	je	.copy
	mov	eax,[ebx+8]
	cmp	eax,[edi+8]
	jge	.exp_ok
	mov	eax,[edi+8]
      .exp_ok:
	call	.change_exp
	xchg	ebx,edi
	call	.change_exp
	xchg	ebx,edi
	mov	edx,[ebx+12]
	mov	eax,[ebx]
	mov	ebx,[ebx+4]
	add	[edi+12],edx
	adc	[edi],eax
	adc	[edi+4],ebx
	jnc	.done
	mov	eax,[edi]
	shrd	[edi+12],eax,1
	mov	eax,[edi+4]
	shrd	[edi],eax,1
	shr	eax,1
	or	eax,80000000h
	mov	[edi+4],eax
	inc	dword [edi+8]
      .done:
	ret
      .copy:
	mov	eax,[ebx]
	mov	[edi],eax
	mov	eax,[ebx+4]
	mov	[edi+4],eax
	mov	eax,[ebx+8]
	mov	[edi+8],eax
	mov	eax,[ebx+12]
	mov	[edi+12],eax
	ret
      .change_exp:
	push	ecx
	mov	ecx,eax
	sub	ecx,[ebx+8]
	mov	edx,[ebx+4]
	jecxz	.exp_done
      .exp_loop:
	mov	ebp,[ebx]
	shrd	[ebx+12],ebp,1
	shrd	[ebx],edx,1
	shr	edx,1
	inc	dword [ebx+8]
	loop	.exp_loop
      .exp_done:
	mov	[ebx+4],edx
	pop	ecx
	ret
      fp_optimize:
	mov	eax,[edi]
	mov	ebp,[edi+4]
	or	ebp,[edi]
	or	ebp,[edi+12]
	jz	fp_zero
      .loop:
	test	byte [edi+7],80h
	jnz	.done
	shld	[edi+4],eax,1
	mov	ebp,[edi+12]
	shld	eax,ebp,1
	mov	[edi],eax
	shl	dword [edi+12],1
	dec	dword [edi+8]
	jmp	.loop
      .done:
	ret
      fp_zero:
	mov	dword [edi+8],8000h
	ret

calculate_expression:
	mov	[current_offset],edi
	mov	[value_undefined],0
      calculation_loop:
	lods	byte [esi]
	or	al,al
	jz	get_string_value
	cmp	al,'.'
	je	convert_fp
	cmp	al,1
	je	get_byte_number
	cmp	al,2
	je	get_word_number
	cmp	al,4
	je	get_dword_number
	cmp	al,8
	je	get_qword_number
	cmp	al,0Fh
	je	value_out_of_range
	cmp	al,10h
	je	get_register
	cmp	al,11h
	je	get_label
	cmp	al,')'
	je	expression_calculated
	cmp	al,']'
	je	expression_calculated
	sub	edi,14h
	mov	ebx,edi
	sub	ebx,14h
	cmp	al,0E0h
	je	calculate_rva
	cmp	al,0D0h
	je	calculate_not
	cmp	al,0D1h
	je	calculate_neg
	mov	dx,[ebx+8]
	or	dx,[edi+8]
	cmp	al,80h
	je	calculate_add
	cmp	al,81h
	je	calculate_sub
	mov	ah,[ebx+12]
	or	ah,[edi+12]
	jz	absolute_values_calculation
	cmp	[error_line],0
	jne	absolute_values_calculation
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_use_of_symbol
      absolute_values_calculation:
	cmp	al,90h
	je	calculate_mul
	cmp	al,91h
	je	calculate_div
	or	dx,dx
	jnz	invalid_expression
	cmp	al,0A0h
	je	calculate_mod
	cmp	al,0B0h
	je	calculate_and
	cmp	al,0B1h
	je	calculate_or
	cmp	al,0B2h
	je	calculate_xor
	cmp	al,0C0h
	je	calculate_shl
	cmp	al,0C1h
	je	calculate_shr
	jmp	invalid_expression
      expression_calculated:
	sub	edi,14h
	cmp	[value_undefined],0
	je	expression_value_ok
	xor	eax,eax
	mov	[edi],eax
	mov	[edi+4],eax
      expression_value_ok:
	ret
      get_byte_number:
	mov	word [edi+8],0
	mov	byte [edi+12],0
	xor	eax,eax
	lods	byte [esi]
	stos	dword [edi]
	xor	al,al
	stos	dword [edi]
	add	edi,0Ch
	jmp	calculation_loop
      get_word_number:
	mov	word [edi+8],0
	mov	byte [edi+12],0
	xor	eax,eax
	lods	word [esi]
	stos	dword [edi]
	xor	ax,ax
	stos	dword [edi]
	add	edi,0Ch
	jmp	calculation_loop
      get_dword_number:
	mov	word [edi+8],0
	mov	byte [edi+12],0
	movs	dword [edi],[esi]
	xor	eax,eax
	stos	dword [edi]
	add	edi,0Ch
	jmp	calculation_loop
      get_qword_number:
	mov	word [edi+8],0
	mov	byte [edi+12],0
	movs	dword [edi],[esi]
	movs	dword [edi],[esi]
	add	edi,0Ch
	jmp	calculation_loop
      get_register:
	mov	byte [edi+9],0
	mov	byte [edi+12],0
	lods	byte [esi]
	mov	[edi+8],al
	mov	byte [edi+10],1
	xor	eax,eax
	stos	dword [edi]
	stos	dword [edi]
	add	edi,0Ch
	jmp	calculation_loop
      get_label:
	xor	eax,eax
	mov	[edi+8],eax
	mov	[edi+12],al
	mov	[edi+20],eax
	lods	dword [esi]
	cmp	eax,0Fh
	jb	predefined_label
	je	reserved_word_used_as_symbol
	cmp	eax,-1
	je	invalid_expression
	mov	ebx,eax
	mov	ax,[current_pass]
	mov	[ebx+18],ax
	or	byte [ebx+8],8
	test	byte [ebx+8],1
	jz	label_undefined
	cmp	ax,[ebx+16]
	je	label_defined
	test	byte [ebx+8],4
	jnz	label_undefined
	test	byte [ebx+9],1
	jz	label_defined
	mov	eax,[ebx]
	sub	eax,dword [adjustment]
	stos	dword [edi]
	mov	eax,[ebx+4]
	sbb	eax,dword [adjustment+4]
	stos	dword [edi]
	mov	eax,dword [adjustment]
	or	eax,dword [adjustment+4]
	jz	got_label
	or	[next_pass_needed],-1
	jmp	got_label
      label_defined:
	mov	eax,[ebx]
	stos	dword [edi]
	mov	eax,[ebx+4]
	stos	dword [edi]
      got_label:
	mov	al,[ebx+11]
	mov	[edi-8+12],al
	mov	eax,[ebx+12]
	mov	[edi-8+8],eax
	mov	eax,[ebx+20]
	mov	[edi-8+16],eax
	add	edi,0Ch
	mov	al,[ebx+10]
	or	al,al
	jz	calculation_loop
	cmp	[size_override],-1
	je	calculation_loop
	cmp	[size_override],0
	je	check_size
	cmp	[operand_size],0
	jne	calculation_loop
	mov	[operand_size],al
	jmp	calculation_loop
      check_size:
	xchg	[operand_size],al
	or	al,al
	jz	calculation_loop
	cmp	al,[operand_size]
	jne	operand_sizes_do_not_match
	jmp	calculation_loop
      current_offset_label:
	mov	al,[labels_type]
	mov	[edi+12],al
	mov	eax,[org_symbol]
	mov	[edi+16],eax
	mov	eax,[current_offset]
	xor	edx,edx
	sub	eax,dword [org_origin]
	sbb	edx,dword [org_origin+4]
	stos	dword [edi]
	mov	eax,edx
	stos	dword [edi]
	mov	eax,[org_registers]
	stos	dword [edi]
	add	edi,8
	jmp	calculation_loop
      org_origin_label:
	mov	al,[labels_type]
	mov	[edi+12],al
	mov	eax,[org_symbol]
	mov	[edi+16],eax
	mov	eax,[org_start]
	xor	edx,edx
	sub	eax,dword [org_origin]
	sbb	edx,dword [org_origin+4]
	stos	dword [edi]
	mov	eax,edx
	stos	dword [edi]
	mov	eax,[org_registers]
	stos	dword [edi]
	add	edi,8
	jmp	calculation_loop
      counter_label:
	mov	eax,[counter]
      make_dword_label_value:
	stos	dword [edi]
	xor	eax,eax
	stos	dword [edi]
	add	edi,0Ch
	jmp	calculation_loop
      timestamp_label:
	call	make_timestamp
	jmp	make_dword_label_value
      predefined_label:
	or	eax,eax
	jz	current_offset_label
	cmp	eax,1
	je	counter_label
	cmp	eax,2
	je	timestamp_label
	cmp	eax,3
	je	org_origin_label
      label_undefined:
	cmp	[current_pass],1
	jbe	force_next_pass
	cmp	[error_line],0
	jne	undefined_value
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],undefined_symbol
	jmp	undefined_value
      force_next_pass:
	or	[next_pass_needed],-1
      undefined_value:
	mov	byte [edi+12],0
	or	[value_undefined],-1
	xor	eax,eax
	stos	dword [edi]
	stos	dword [edi]
	add	edi,0Ch
	jmp	calculation_loop
      calculate_add:
	mov	ecx,[ebx+16]
	cmp	byte [edi+12],0
	je	add_values
	mov	ecx,[edi+16]
	cmp	byte [ebx+12],0
	je	add_values
	cmp	[error_line],0
	jne	add_values
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_use_of_symbol
      add_values:
	mov	al,[edi+12]
	or	[ebx+12],al
	mov	[ebx+16],ecx
	mov	eax,[edi]
	add	[ebx],eax
	mov	eax,[edi+4]
	adc	[ebx+4],eax
	or	dx,dx
	jz	calculation_loop
	push	esi
	mov	esi,ebx
	lea	ebx,[edi+10]
	mov	cl,[edi+8]
	call	add_register
	lea	ebx,[edi+11]
	mov	cl,[edi+9]
	call	add_register
	pop	esi
	jmp	calculation_loop
      add_register:
	or	cl,cl
	jz	add_register_done
      add_register_start:
	cmp	[esi+8],cl
	jne	add_in_second_slot
	mov	al,[ebx]
	add	[esi+10],al
	jnz	add_register_done
	mov	byte [esi+8],0
	ret
      add_in_second_slot:
	cmp	[esi+9],cl
	jne	create_in_first_slot
	mov	al,[ebx]
	add	[esi+11],al
	jnz	add_register_done
	mov	byte [esi+9],0
	ret
      create_in_first_slot:
	cmp	byte [esi+8],0
	jne	create_in_second_slot
	mov	[esi+8],cl
	mov	al,[ebx]
	mov	[esi+10],al
	ret
      create_in_second_slot:
	cmp	byte [esi+9],0
	jne	invalid_expression
	mov	[esi+9],cl
	mov	al,[ebx]
	mov	[esi+11],al
      add_register_done:
	ret
      calculate_sub:
	xor	ah,ah
	mov	ah,[ebx+12]
	mov	al,[edi+12]
	or	al,al
	jz	sub_values
	cmp	al,ah
	jne	invalid_sub
	xor	ah,ah
	mov	ecx,[edi+16]
	cmp	ecx,[ebx+16]
	je	sub_values
	cmp	ecx,[org_symbol]
	jne	invalid_sub
	test	byte [ebx+12],1
	jnz	invalid_sub
	mov	ah,3
      sub_values:
	mov	[ebx+12],ah
	mov	eax,[edi]
	sub	[ebx],eax
	mov	eax,[edi+4]
	sbb	[ebx+4],eax
	or	dx,dx
	jz	calculation_loop
	push	esi
	mov	esi,ebx
	lea	ebx,[edi+10]
	mov	cl,[edi+8]
	call	sub_register
	lea	ebx,[edi+11]
	mov	cl,[edi+9]
	call	sub_register
	pop	esi
	jmp	calculation_loop
      invalid_sub:
	cmp	[error_line],0
	jne	sub_values
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_use_of_symbol
	jmp	sub_values
      sub_register:
	or	cl,cl
	jz	add_register_done
	neg	byte [ebx]
	jmp	add_register_start
      calculate_mul:
	or	dx,dx
	jz	mul_start
	cmp	word [ebx+8],0
	jne	mul_start
	mov	eax,[ebx]
	xchg	eax,[edi]
	mov	[ebx],eax
	mov	eax,[ebx+4]
	xchg	eax,[edi+4]
	mov	[ebx+4],eax
	mov	eax,[ebx+8]
	xchg	eax,[edi+8]
	mov	[ebx+8],eax
	mov	eax,[ebx+12]
	xchg	eax,[edi+12]
	mov	[ebx+12],eax
      mul_start:
	push	esi edx
	mov	esi,ebx
	xor	bl,bl
	bt	dword [esi+4],31
	jnc	mul_first_sign_ok
	not	dword [esi]
	not	dword [esi+4]
	add	dword [esi],1
	adc	dword [esi+4],0
	not	bl
      mul_first_sign_ok:
	bt	dword [edi+4],31
	jnc	mul_second_sign_ok
	not	dword [edi]
	not	dword [edi+4]
	add	dword [edi],1
	adc	dword [edi+4],0
	not	bl
      mul_second_sign_ok:
	cmp	dword [esi+4],0
	jz	mul_numbers
	cmp	dword [edi+4],0
	jnz	value_out_of_range
      mul_numbers:
	mov	eax,[esi+4]
	mul	dword [edi]
	or	edx,edx
	jnz	value_out_of_range
	mov	ecx,eax
	mov	eax,[esi]
	mul	dword [edi+4]
	or	edx,edx
	jnz	value_out_of_range
	add	ecx,eax
	jc	value_out_of_range
	mov	eax,[esi]
	mul	dword [edi]
	add	edx,ecx
	jc	value_out_of_range
	test	edx,1 shl 31
	jnz	value_out_of_range
	mov	[esi],eax
	mov	[esi+4],edx
	or	bl,bl
	jz	mul_ok
	not	dword [esi]
	not	dword [esi+4]
	add	dword [esi],1
	adc	dword [esi+4],0
      mul_ok:
	pop	edx
	or	dx,dx
	jz	mul_calculated
	cmp	word [edi+8],0
	jne	invalid_value
	cmp	byte [esi+8],0
	je	mul_first_register_ok
	mov	al,[edi]
	cbw
	cwde
	cdq
	cmp	edx,[edi+4]
	jne	value_out_of_range
	cmp	eax,[edi]
	jne	value_out_of_range
	imul	byte [esi+10]
	mov	dl,ah
	cbw
	cmp	ah,dl
	jne	value_out_of_range
	mov	[esi+10],al
      mul_first_register_ok:
	cmp	byte [esi+9],0
	je	mul_calculated
	mov	al,[edi]
	cbw
	cwde
	cdq
	cmp	edx,[edi+4]
	jne	value_out_of_range
	cmp	eax,[edi]
	jne	value_out_of_range
	imul	byte [esi+11]
	mov	dl,ah
	cbw
	cmp	ah,dl
	jne	value_out_of_range
	mov	[esi+11],al
      mul_calculated:
	pop	esi
	jmp	calculation_loop
      calculate_div:
	push	esi edx
	mov	esi,ebx
	call	div_64
	pop	edx
	or	dx,dx
	jz	div_calculated
	cmp	byte [esi+8],0
	je	div_first_register_ok
	mov	al,[edi]
	cbw
	cwde
	cdq
	cmp	edx,[edi+4]
	jne	value_out_of_range
	cmp	eax,[edi]
	jne	value_out_of_range
	or	al,al
	jz	value_out_of_range
	mov	al,[esi+10]
	cbw
	idiv	byte [edi]
	mov	[esi+10],al
      div_first_register_ok:
	cmp	byte [esi+9],0
	je	div_calculated
	mov	al,[edi]
	cbw
	cwde
	cdq
	cmp	edx,[edi+4]
	jne	value_out_of_range
	cmp	eax,[edi]
	jne	value_out_of_range
	or	al,al
	jz	value_out_of_range
	mov	al,[esi+11]
	cbw
	idiv	byte [edi]
	mov	[esi+11],al
      div_calculated:
	pop	esi
	jmp	calculation_loop
      calculate_mod:
	push	esi
	mov	esi,ebx
	call	div_64
	mov	[esi],eax
	mov	[esi+4],edx
	pop	esi
	jmp	calculation_loop
      calculate_and:
	mov	eax,[edi]
	and	[ebx],eax
	mov	eax,[edi+4]
	and	[ebx+4],eax
	jmp	calculation_loop
      calculate_or:
	mov	eax,[edi]
	or	[ebx],eax
	mov	eax,[edi+4]
	or	[ebx+4],eax
	jmp	calculation_loop
      calculate_xor:
	mov	eax,[edi]
	xor	[ebx],eax
	mov	eax,[edi+4]
	xor	[ebx+4],eax
	jmp	calculation_loop
      shr_negative:
	not	dword [edi]
	not	dword [edi+4]
	add	dword [edi],1
	adc	dword [edi+4],0
      calculate_shl:
	mov	eax,dword [edi+4]
	bt	eax,31
	jc	shl_negative
	or	eax,eax
	jnz	zero_value
	mov	ecx,[edi]
	cmp	ecx,64
	jae	zero_value
	cmp	ecx,32
	jae	shl_high
	mov	edx,[ebx+4]
	mov	eax,[ebx]
	shld	edx,eax,cl
	shl	eax,cl
	mov	[ebx],eax
	mov	[ebx+4],edx
	jmp	calculation_loop
      shl_high:
	sub	cl,32
	mov	eax,[ebx]
	shl	eax,cl
	mov	[ebx+4],eax
	mov	dword [ebx],0
	jmp	calculation_loop
      shl_negative:
	not	dword [edi]
	not	dword [edi+4]
	add	dword [edi],1
	adc	dword [edi+4],0
      calculate_shr:
	mov	eax,dword [edi+4]
	bt	eax,31
	jc	shr_negative
	or	eax,eax
	jnz	zero_value
	mov	ecx,[edi]
	cmp	ecx,64
	jae	zero_value
	cmp	ecx,32
	jae	shr_high
	mov	edx,[ebx+4]
	mov	eax,[ebx]
	shrd	eax,edx,cl
	shr	edx,cl
	mov	[ebx],eax
	mov	[ebx+4],edx
	jmp	calculation_loop
      shr_high:
	sub	cl,32
	mov	eax,[ebx+4]
	shr	eax,cl
	mov	[ebx],eax
	mov	dword [ebx+4],0
	jmp	calculation_loop
      zero_value:
	mov	dword [ebx],0
	mov	dword [ebx+4],0
	jmp	calculation_loop
      calculate_not:
	cmp	word [edi+8],0
	jne	invalid_expression
	cmp	byte [edi+12],0
	je	not_ok
	cmp	[error_line],0
	jne	not_ok
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_use_of_symbol
      not_ok:
	cmp	[value_size],1
	je	not_byte
	cmp	[value_size],2
	je	not_word
	cmp	[value_size],4
	je	not_dword
	cmp	[value_size],6
	je	not_pword
      not_qword:
	not	dword [edi]
	not	dword [edi+4]
	add	edi,14h
	jmp	calculation_loop
      not_byte:
	cmp	dword [edi+4],0
	jne	not_qword
	cmp	word [edi+2],0
	jne	not_qword
	cmp	byte [edi+1],0
	jne	not_qword
	not	byte [edi]
	add	edi,14h
	jmp	calculation_loop
      not_word:
	cmp	dword [edi+4],0
	jne	not_qword
	cmp	word [edi+2],0
	jne	not_qword
	not	word [edi]
	add	edi,14h
	jmp	calculation_loop
      not_dword:
	cmp	dword [edi+4],0
	jne	not_qword
	not	dword [edi]
	add	edi,14h
	jmp	calculation_loop
      not_pword:
	cmp	word [edi+6],0
	jne	not_qword
	not	dword [edi]
	not	word [edi+4]
	add	edi,14h
	jmp	calculation_loop
      calculate_neg:
	cmp	word [edi+8],0
	jne	invalid_expression
	cmp	byte [edi+12],0
	je	neg_ok
	cmp	[error_line],0
	jne	neg_ok
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_use_of_symbol
      neg_ok:
	mov	eax,[edi]
	mov	edx,[edi+4]
	mov	dword [edi],0
	mov	dword [edi+4],0
	sub	[edi],eax
	sbb	[edi+4],edx
	add	edi,14h
	jmp	calculation_loop
      calculate_rva:
	cmp	[output_format],3
	jne	invalid_expression
	cmp	word [edi+8],0
	jne	invalid_expression
	test	[format_flags],8
	jnz	pe64_rva
	cmp	byte [edi+12],2
	je	rva_ok
	cmp	[error_line],0
	jne	rva_ok
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_use_of_symbol
      rva_ok:
	mov	byte [edi+12],0
	mov	eax,[code_start]
	mov	eax,[eax+34h]
	cdq
	sub	[edi],eax
	sbb	[edi+4],edx
	add	edi,14h
	jmp	calculation_loop
      pe64_rva:
	cmp	byte [edi+12],4
	je	pe64_rva_ok
	cmp	[error_line],0
	jne	pe64_rva_ok
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_use_of_symbol
      pe64_rva_ok:
	mov	byte [edi+12],0
	mov	eax,[code_start]
	mov	edx,[eax+34h]
	mov	eax,[eax+30h]
	sub	[edi],eax
	sbb	[edi+4],edx
	add	edi,14h
	jmp	calculation_loop
      div_64:
	xor	ebx,ebx
	cmp	dword [edi],0
	jne	divider_ok
	cmp	dword [edi+4],0
	jne	divider_ok
	cmp	[next_pass_needed],0
	je	value_out_of_range
	jmp	div_done
      divider_ok:
	bt	dword [esi+4],31
	jnc	div_first_sign_ok
	not	dword [esi]
	not	dword [esi+4]
	add	dword [esi],1
	adc	dword [esi+4],0
	not	bx
      div_first_sign_ok:
	bt	dword [edi+4],31
	jnc	div_second_sign_ok
	not	dword [edi]
	not	dword [edi+4]
	add	dword [edi],1
	adc	dword [edi+4],0
	not	bl
      div_second_sign_ok:
	cmp	dword [edi+4],0
	jne	div_high
	mov	ecx,[edi]
	mov	eax,[esi+4]
	xor	edx,edx
	div	ecx
	mov	[esi+4],eax
	mov	eax,[esi]
	div	ecx
	mov	[esi],eax
	mov	eax,edx
	xor	edx,edx
	jmp	div_done
      div_high:
	mov	eax,[esi+4]
	xor	edx,edx
	div	dword [edi+4]
	mov	ebx,[esi]
	mov	[esi],eax
	mov	dword [esi+4],0
	mov	ecx,edx
	mul	dword [edi]
      div_high_loop:
	cmp	ecx,edx
	ja	div_high_done
	jb	div_high_large_correction
	cmp	ebx,eax
	jae	div_high_done
      div_high_correction:
	dec	dword [esi]
	sub	eax,[edi]
	sbb	edx,[edi+4]
	jnc	div_high_loop
      div_high_done:
	sub	ebx,eax
	sbb	ecx,edx
	mov	edx,ecx
	mov	eax,ebx
	ret
      div_high_large_correction:
	push	eax edx
	mov	eax,edx
	sub	eax,ecx
	xor	edx,edx
	div	dword [edi+4]
	shr	eax,1
	jz	div_high_small_correction
	sub	[esi],eax
	push	eax
	mul	dword [edi+4]
	sub	dword [esp+4],eax
	pop	eax
	mul	dword [edi]
	sub	dword [esp+4],eax
	sbb	dword [esp],edx
	pop	edx eax
	jmp	div_high_loop
      div_high_small_correction:
	pop	edx eax
	jmp	div_high_correction
      div_done:
	or	bh,bh
	jz	remainder_ok
	not	eax
	not	edx
	add	eax,1
	adc	edx,0
      remainder_ok:
	or	bl,bl
	jz	div_ok
	not	dword [esi]
	not	dword [esi+4]
	add	dword [esi],1
	adc	dword [esi+4],0
      div_ok:
	ret
      convert_fp:
	mov	word [edi+8],0
	mov	byte [edi+12],0
	mov	al,[value_size]
	cmp	al,4
	je	convert_fp_dword
	cmp	al,8
	je	convert_fp_qword
	jmp	invalid_value
      convert_fp_dword:
	xor	eax,eax
	cmp	word [esi+8],8000h
	je	fp_dword_store
	mov	bx,[esi+8]
	mov	eax,[esi+4]
	shl	eax,1
	shr	eax,9
	jnc	fp_dword_ok
	inc	eax
	bt	eax,23
	jnc	fp_dword_ok
	and	eax,1 shl 23 - 1
	inc	bx
	shr	eax,1
      fp_dword_ok:
	add	bx,7Fh
	cmp	bx,100h
	jae	value_out_of_range
	shl	ebx,23
	or	eax,ebx
      fp_dword_store:
	mov	bl,[esi+11]
	shl	ebx,31
	or	eax,ebx
	mov	[edi],eax
	xor	eax,eax
	mov	[edi+4],eax
	add	esi,13
	ret
      convert_fp_qword:
	xor	eax,eax
	xor	edx,edx
	cmp	word [esi+8],8000h
	je	fp_qword_store
	mov	bx,[esi+8]
	mov	eax,[esi]
	mov	edx,[esi+4]
	add	eax,eax
	adc	edx,edx
	mov	ecx,edx
	shr	edx,12
	shrd	eax,ecx,12
	jnc	fp_qword_ok
	add	eax,1
	adc	edx,0
	bt	edx,20
	jnc	fp_qword_ok
	and	edx,1 shl 20 - 1
	inc	bx
	shr	edx,1
	rcr	eax,1
      fp_qword_ok:
	add	bx,3FFh
	cmp	bx,800h
	jae	value_out_of_range
	shl	ebx,20
	or	edx,ebx
      fp_qword_store:
	mov	bl,[esi+11]
	shl	ebx,31
	or	edx,ebx
	mov	[edi],eax
	mov	[edi+4],edx
	add	esi,13
	ret
      get_string_value:
	lods	dword [esi]
	mov	ecx,eax
	cmp	ecx,8
	ja	value_out_of_range
	mov	edx,edi
	xor	eax,eax
	stos	dword [edi]
	stos	dword [edi]
	mov	edi,edx
	rep	movs byte [edi],[esi]
	mov	edi,edx
	inc	esi
	mov	word [edi+8],0
	mov	byte [edi+12],0
	ret

get_byte_value:
	mov	[value_size],1
	mov	[size_override],-1
	call	calculate_expression
	mov	eax,[edi+16]
	mov	[symbol_identifier],eax
	mov	[value_type],0
	cmp	word [edi+8],0
	jne	invalid_value
	cmp	byte [edi+12],0
	je	check_byte_value
	cmp	[error_line],0
	jne	check_byte_value
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_use_of_symbol
      check_byte_value:
	mov	eax,[edi]
	mov	edx,[edi+4]
	or	edx,edx
	jz	byte_positive
	cmp	edx,-1
	jne	range_exceeded
	cmp	eax,-80h
	jb	range_exceeded
	ret
      byte_positive:
	cmp	eax,100h
	jae	range_exceeded
      return_byte_value:
	ret
      range_exceeded:
	xor	eax,eax
	xor	edx,edx
	cmp	[error_line],0
	jne	return_byte_value
	mov	ecx,[current_line]
	mov	[error_line],ecx
	mov	[error],value_out_of_range
	ret
get_word_value:
	mov	[value_size],2
	mov	[size_override],-1
	call	calculate_expression
	mov	eax,[edi+16]
	mov	[symbol_identifier],eax
	cmp	word [edi+8],0
	jne	invalid_value
	mov	al,[edi+12]
	mov	[value_type],al
	cmp	al,2
	jb	check_word_value
	cmp	[error_line],0
	jne	check_word_value
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_use_of_symbol
      check_word_value:
	mov	eax,[edi]
	mov	edx,[edi+4]
	or	edx,edx
	jz	word_positive
	cmp	edx,-1
	jne	range_exceeded
	cmp	eax,-8000h
	jb	range_exceeded
	ret
      word_positive:
	cmp	eax,10000h
	jae	range_exceeded
	ret
get_dword_value:
	mov	[value_size],4
	mov	[size_override],-1
	call	calculate_expression
	mov	eax,[edi+16]
	mov	[symbol_identifier],eax
	cmp	word [edi+8],0
	jne	invalid_value
	mov	al,[edi+12]
	mov	[value_type],al
	cmp	al,4
	jb	check_dword_value
	cmp	[error_line],0
	jne	check_dword_value
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_use_of_symbol
      check_dword_value:
	mov	eax,[edi]
	mov	edx,[edi+4]
	or	edx,edx
	jz	dword_positive
	cmp	edx,-1
	jne	range_exceeded
	bt	eax,31
	jnc	range_exceeded
      dword_positive:
	ret
get_pword_value:
	mov	[value_size],6
	mov	[size_override],-1
	call	calculate_expression
	mov	eax,[edi+16]
	mov	[symbol_identifier],eax
	cmp	word [edi+8],0
	jne	invalid_value
	mov	al,[edi+12]
	mov	[value_type],al
	cmp	al,4
	jb	check_pword_value
	cmp	[error_line],0
	jne	check_pword_value
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_use_of_symbol
      check_pword_value:
	mov	eax,[edi]
	mov	edx,[edi+4]
	cmp	edx,10000h
	jge	range_exceeded
	cmp	edx,-8000h
	jl	range_exceeded
	ret
get_qword_value:
	mov	[value_size],8
	mov	[size_override],-1
	call	calculate_expression
	mov	eax,[edi+16]
	mov	[symbol_identifier],eax
	cmp	word [edi+8],0
	jne	invalid_value
	mov	al,[edi+12]
	mov	[value_type],al
      check_qword_value:
	mov	eax,[edi]
	mov	edx,[edi+4]
	ret
get_value:
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	jne	invalid_value
	mov	al,[operand_size]
	cmp	al,1
	je	value_byte
	cmp	al,2
	je	value_word
	cmp	al,4
	je	value_dword
	cmp	al,6
	je	value_pword
	cmp	al,8
	je	value_qword
	or	al,al
	jnz	invalid_value
      value_qword:
	call	get_qword_value
	ret
      value_pword:
	call	get_pword_value
	movzx	edx,dx
	ret
      value_dword:
	call	get_dword_value
	xor	edx,edx
	ret
      value_word:
	call	get_word_value
	xor	edx,edx
	movzx	eax,ax
	ret
      value_byte:
	call	get_byte_value
	xor	edx,edx
	movzx	eax,al
	ret
get_address_value:
	mov	[address_size],0
	mov	[value_size],8
      calculate_address:
	cmp	byte [esi],'.'
	je	invalid_address
	call	calculate_expression
	mov	eax,[edi+16]
	mov	[address_symbol],eax
	mov	al,[edi+12]
	test	al,1
	jnz	invalid_use_of_symbol
	mov	[value_type],al
	or	al,al
	jz	address_symbol_ok
	shl	al,5
	mov	ah,[address_size]
	or	[address_size],al
	shr	al,4
	or	ah,ah
	jz	address_symbol_ok
	cmp	al,ah
	jne	address_sizes_do_not_agree
      address_symbol_ok:
	xor	bx,bx
	xor	cl,cl
	mov	ch,[address_size]
	cmp	word [edi+8],0
	je	check_immediate_address
	mov	al,[edi+8]
	mov	dl,[edi+10]
	call	get_address_register
	mov	al,[edi+9]
	mov	dl,[edi+11]
	call	get_address_register
	mov	ax,bx
	shr	ah,4
	shr	al,4
	or	bh,bh
	jz	check_address_registers
	or	bl,bl
	jz	check_address_registers
	cmp	al,ah
	jne	invalid_address
      check_address_registers:
	or	al,ah
	cmp	al,4
	je	sib_allowed
	cmp	al,8
	je	sib_allowed
	or	cl,cl
	jz	check_word_value
	cmp	cl,1
	je	check_word_value
	jmp	invalid_address
      get_address_register:
	or	al,al
	jz	address_register_ok
	cmp	dl,1
	jne	scaled_register
	or	bh,bh
	jnz	scaled_register
	mov	bh,al
      address_register_ok:
	ret
      scaled_register:
	or	bl,bl
	jnz	invalid_address
	mov	bl,al
	mov	cl,dl
	jmp	address_register_ok
      sib_allowed:
	or	bh,bh
	jnz	check_index_scale
	cmp	cl,2
	je	special_index_scale
	cmp	cl,3
	je	special_index_scale
	cmp	cl,5
	je	special_index_scale
	cmp	cl,9
	je	special_index_scale
      check_index_scale:
	or	cl,cl
	jz	address_registers_ok
	cmp	cl,1
	je	address_registers_ok
	cmp	cl,2
	je	address_registers_ok
	cmp	cl,4
	je	address_registers_ok
	cmp	cl,8
	je	address_registers_ok
	jmp	invalid_address
      special_index_scale:
	mov	bh,bl
	dec	cl
      address_registers_ok:
	jmp	check_dword_value
      check_immediate_address:
	cmp	[code_type],64
	jne	check_dword_value
	jmp	check_qword_value
get_relative_offset:
	mov	[value_size],4
	mov	[size_override],-1
	call	calculate_expression
      calculate_relative_offset:
	push	esi
	add	edi,14h
	mov	esi,[display_buffer]
	sub	esi,7
	lea	eax,[esi-14h]
	cmp	eax,edi
	jb	out_of_memory
	mov	byte [esi],11h
	xor	eax,eax
	mov	dword [esi+1],eax
	mov	word [esi+5],')' shl 8 + 81h
	call	calculation_loop
	pop	esi
	cmp	word [edi+8],0
	jne	invalid_value
	mov	al,[edi+12]
	mov	[value_type],al
	mov	eax,[edi+16]
	mov	[symbol_identifier],eax
	mov	eax,[edi]
	mov	edx,[edi+4]
	or	edx,edx
	jz	offset_positive
	cmp	edx,-1
	jne	range_exceeded
	bt	eax,31
	jnc	range_exceeded
	ret
      offset_positive:
	bt	eax,31
	jc	range_exceeded
	ret

calculate_logical_expression:
	call	get_logical_value
      logical_loop:
	cmp	byte [esi],'|'
	je	logical_or
	cmp	byte [esi],'&'
	je	logical_and
	ret
      logical_or:
	inc	esi
	push	eax
	call	get_logical_value
	pop	ebx
	or	al,bl
	jmp	logical_loop
      logical_and:
	inc	esi
	push	eax
	call	get_logical_value
	pop	ebx
	and	al,bl
	jmp	logical_loop

get_logical_value:
	xor	al,al
      check_for_negation:
	cmp	byte [esi],'~'
	jne	negation_ok
	inc	esi
	xor	al,-1
	jmp	check_for_negation
      negation_ok:
	push	eax
	mov	al,[esi]
	cmp	al,'{'
	je	logical_expression
	cmp	al,88h
	je	check_for_defined
	cmp	al,89h
	je	check_for_used
	push	esi
	cmp	al,11h
	jne	check_for_values
	add	esi,2
      check_for_values:
	xor	bl,bl
	cmp	byte [esi],'('
	jne	find_eq_symbol
	call	skip_symbol
	lods	byte [esi]
	cmp	al,'='
	je	compare_values
	cmp	al,'>'
	je	compare_values
	cmp	al,'<'
	je	compare_values
	cmp	al,0F1h
	je	compare_values
	cmp	al,0F2h
	je	compare_values
	cmp	al,0F3h
	je	compare_values
	dec	esi
      find_eq_symbol:
	cmp	byte [esi],0F0h
	je	compare_symbols
	cmp	byte [esi],0F7h
	je	compare_symbol_types
	cmp	byte [esi],0F6h
	je	scan_symbols_list
	call	check_character
	jc	logical_number
	cmp	al,','
	jne	next_eq_symbol
	mov	bl,1
      next_eq_symbol:
	call	skip_symbol
	jmp	find_eq_symbol
      compare_symbols:
	inc	esi
	pop	ebx
	mov	edx,esi
	push	edi
	mov	edi,ebx
	mov	ecx,esi
	dec	ecx
	sub	ecx,edi
	repe	cmps byte [esi],[edi]
	pop	edi
	je	symbols_equal
	mov	esi,edx
      symbols_different:
	call	check_character
	jc	return_false
	call	skip_symbol
	jmp	symbols_different
      symbols_equal:
	call	check_character
	jc	return_true
	jmp	symbols_different
      compare_symbol_types:
	inc	esi
	pop	ebx
      type_comparision:
	call	check_character
	jc	types_compared
	mov	al,[esi]
	cmp	al,[ebx]
	jne	symbols_different
	cmp	al,'('
	jne	equal_type
	mov	al,[esi+1]
	mov	ah,[ebx+1]
	cmp	al,ah
	je	equal_type
	or	al,al
	jz	symbols_different
	or	ah,ah
	jz	symbols_different
	cmp	al,'.'
	je	symbols_different
	cmp	ah,'.'
	je	symbols_different
      equal_type:
	call	skip_symbol
	xchg	esi,ebx
	call	skip_symbol
	xchg	esi,ebx
	jmp	type_comparision
      types_compared:
	cmp	byte [ebx],0F7h
	jne	return_false
	jmp	return_true
      scan_symbols_list:
	or	bl,bl
	jnz	invalid_expression
	xor	bp,bp
	inc	esi
	lods	byte [esi]
	cmp	al,'<'
	jne	invalid_expression
	pop	ebx
	mov	ecx,esi
	sub	ecx,2
	sub	ecx,ebx
      compare_in_list:
	mov	edx,esi
	push	ecx edi
	mov	edi,ebx
	repe	cmps byte [esi],[edi]
	pop	edi ecx
	jne	not_equal_in_list
	cmp	byte [esi],','
	je	skip_rest_of_list
	cmp	byte [esi],'>'
	jne	not_equal_in_list
      skip_rest_of_list:
	mov	al,[esi]
	or	al,al
	jz	invalid_expression
	cmp	al,0Fh
	je	invalid_expression
	cmp	al,'>'
	je	list_return_true
	call	skip_symbol
	jmp	skip_rest_of_list
      list_return_true:
	inc	esi
	jmp	return_true
      not_equal_in_list:
	mov	esi,edx
      skip_list_item:
	mov	al,[esi]
	or	al,al
	jz	invalid_expression
	cmp	al,0Fh
	je	invalid_expression
	cmp	al,'<'
	je	invalid_expression
	cmp	al,'>'
	je	list_return_false
	cmp	al,','
	je	next_in_list
	call	skip_symbol
	jmp	skip_list_item
      next_in_list:
	inc	esi
	jmp	compare_in_list
      list_return_false:
	inc	esi
	jmp	return_false
      check_character:
	mov	al,[esi]
	or	al,al
	jz	stop
	cmp	al,0Fh
	je	stop
	cmp	al,'}'
	je	stop
	cmp	al,'|'
	je	stop
	cmp	al,'&'
	je	stop
	clc
	ret
      stop:
	stc
	ret
      compare_values:
	pop	esi
	call	get_value
	mov	bl,[value_type]
	push	eax edx ebx
	lods	byte [esi]
	mov	[compare_type],al
	call	get_value
	pop	ebx
	cmp	[next_pass_needed],0
	jne	values_ok
	cmp	bl,[value_type]
	jne	invalid_use_of_symbol
      values_ok:
	pop	ecx ebx
	cmp	[compare_type],'='
	je	check_equal
	cmp	[compare_type],'>'
	je	check_greater
	cmp	[compare_type],'<'
	je	check_less
	cmp	[compare_type],0F1h
	je	check_not_equal
	cmp	[compare_type],0F2h
	je	check_not_less
	cmp	[compare_type],0F3h
	je	check_not_greater
	jmp	invalid_expression
      check_equal:
	cmp	eax,ebx
	jne	return_false
	cmp	edx,ecx
	jne	return_false
	jmp	return_true
      check_greater:
	cmp	edx,ecx
	jl	return_true
	jg	return_false
	cmp	eax,ebx
	jb	return_true
	jae	return_false
      check_less:
	cmp	edx,ecx
	jl	return_false
	jg	return_true
	cmp	eax,ebx
	jbe	return_false
	ja	return_true
      check_not_less:
	cmp	edx,ecx
	jl	return_true
	jg	return_false
	cmp	eax,ebx
	jbe	return_true
	ja	return_false
      check_not_greater:
	cmp	edx,ecx
	jl	return_false
	jg	return_true
	cmp	eax,ebx
	jb	return_false
	jae	return_true
      check_not_equal:
	cmp	eax,ebx
	jne	return_true
	cmp	edx,ecx
	jne	return_true
	jmp	return_false
      logical_number:
	pop	esi
	call	get_value
	cmp	[value_type],0
	jne	invalid_expression
	or	eax,edx
	jnz	return_true
	jmp	return_false
      check_for_defined:
	or	bl,-1
	lods	word [esi]
	cmp	ah,'('
	jne	invalid_expression
      check_expression:
	lods	byte [esi]
	or	al,al
	jz	defined_string
	cmp	al,'.'
	je	defined_fp_value
	cmp	al,')'
	je	expression_checked
	cmp	al,0Fh
	je	check_expression
	cmp	al,10h
	je	defined_register
	cmp	al,11h
	je	check_if_symbol_defined
	cmp	al,80h
	jae	check_expression
	movzx	eax,al
	add	esi,eax
	jmp	check_expression
      defined_register:
	inc	esi
	jmp	check_expression
      defined_fp_value:
	add	esi,12
	jmp	expression_checked
      defined_string:
	lods	dword [esi]
	add	esi,eax
	inc	esi
	jmp	expression_checked
      check_if_symbol_defined:
	lods	dword [esi]
	cmp	eax,-1
	je	invalid_expression
	cmp	eax,0Fh
	jb	check_expression
	je	reserved_word_used_as_symbol
	test	byte [eax+8],4
	jnz	no_prediction
	test	byte [eax+8],1
	jz	symbol_predicted_undefined
	mov	cx,[current_pass]
	sub	cx,[eax+16]
	jz	check_expression
	cmp	cx,1
	ja	symbol_predicted_undefined
	or	byte [eax+8],40h+80h
	jmp	check_expression
      no_prediction:
	test	byte [eax+8],1
	jz	symbol_undefined
	mov	cx,[current_pass]
	sub	cx,[eax+16]
	jz	check_expression
	jmp	symbol_undefined
      symbol_predicted_undefined:
	or	byte [eax+8],40h
	and	byte [eax+8],not 80h
      symbol_undefined:
	xor	bl,bl
	jmp	check_expression
      expression_checked:
	mov	al,bl
	jmp	logical_value_ok
      check_for_used:
	lods	word [esi]
	cmp	ah,2
	jne	invalid_expression
	lods	dword [esi]
	cmp	eax,0Fh
	jb	invalid_use_of_symbol
	je	reserved_word_used_as_symbol
	inc	esi
	test	byte [eax+8],8
	jz	not_used
	mov	cx,[current_pass]
	sub	cx,[eax+18]
	jz	return_true
	cmp	cx,1
	ja	not_used
	or	byte [eax+8],10h+20h
	jmp	return_true
      not_used:
	or	byte [eax+8],10h
	and	byte [eax+8],not 20h
      return_false:
	xor	al,al
	jmp	logical_value_ok
      return_true:
	or	al,-1
	jmp	logical_value_ok
      logical_expression:
	inc	esi
	call	calculate_logical_expression
	push	eax
	lods	byte [esi]
	cmp	al,'}'
	jne	invalid_expression
	pop	eax
      logical_value_ok:
	pop	ebx
	xor	al,bl
	ret
