
; flat assembler  version 1.09
; Copyright (c) 1999-2000, Tomasz Grysztar
; All rights reserved.

convert_expression:
	push	ebp
	mov	ebp,esp
	push	edi
	mov	edi,operators
	call	get_operator
	pop	edi
	or	al,al
	jz	expression_loop
	push	ebp
	cmp	al,80h
	je	init_positive
	cmp	al,81h
	je	init_negative
	jmp	invalid_expression
      init_positive:
	xor	al,al
	jmp	expression_number
      init_negative:
	mov	al,0D1h
	jmp	expression_number
      expression_loop:
	push	ebp edi
	mov	edi,single_operand_operators
	call	get_operator
	pop	edi
      expression_number:
	push	ax
	cmp	byte [esi],0
	je	invalid_expression
	call	convert_number
	pop	ax
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
	pop	bx
	mov	byte [edi],bl
	inc	edi
	jmp	operators_loop
      push_operator:
	push	ax
	jmp	expression_loop
      expression_end:
	cmp	esp,ebp
	je	expression_converted
	pop	ax
	stos	byte [edi]
	jmp	expression_end
      expression_converted:
	pop	ebp
	ret

convert_number:
	cmp	byte [esi],'('
	je	expression_value
	inc	edi
	push	edi
	mov	ebp,edi
	call	get_number
	pop	edi
	jc	symbol_value
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
	add	edi,4
	ret
      word_number:
	mov	byte [edi-1],2
	add	edi,2
	ret
      expression_value:
	inc	esi
	call	convert_expression
	lods	byte [esi]
	cmp	al,')'
	jne	invalid_value
	ret
      symbol_value:
	lods	byte [esi]
	cmp	al,1Ah
	jne	invalid_value
	lods	byte [esi]
	movzx	ecx,al
	push	ecx esi edi
	mov	edi,index_registers
	call	get_symbol
	jnc	register_value
	mov	edi,symbols
	call	get_symbol
	jnc	invalid_value
	pop	edi esi ecx
	call	get_label_id
	mov	byte [edi-1],11h
	stos	dword [edi]
	ret
      register_value:
	pop	edi
	add	esp,8
	mov	byte [edi-1],10h
	mov	al,ah
	stos	byte [edi]
	ret

get_number:
	cmp	byte [esi],27h
	je	get_text_number
	cmp	byte [esi],1Ah
	jne	no_number
	inc	esi
	lods	byte [esi]
	movzx	ecx,al
	mov	[number_start],esi
	mov	al,[esi]
	sub	al,30h
	jb	invalid_number
	cmp	al,9
	ja	invalid_number
	mov	eax,esi
	add	esi,ecx
	push	esi
	sub	esi,2
	mov	dword [ds:ebp],0
	mov	dword [ds:ebp+4],0
	cmp	byte [esi+1],'h'
	je	get_hex_number
	cmp	byte [esi+1],'o'
	je	get_oct_number
	cmp	byte [esi+1],'b'
	je	get_bin_number
	cmp	byte [esi+1],'d'
	je	get_dec_number
	inc	esi
	cmp	word [eax],'0x'
	je	get_hex_number
	cmp	byte [eax],'0'
	je	get_oct_number
      get_dec_number:
	xor	edx,edx
	mov	ebx,1
      get_dec_digit:
	cmp	esi,[number_start]
	jb	number_ok
	movzx	eax,byte [esi]
	sub	al,30h
	jc	bad_number
	cmp	al,9
	ja	bad_number
	mov	ecx,eax
	jecxz	next_dec_digit
      convert_dec_digit:
	add	dword [ds:ebp],ebx
	adc	dword [ds:ebp+4],edx
	loop	convert_dec_digit
      next_dec_digit:
	dec	esi
	mov	ecx,edx
	mov	eax,10
	mul	ebx
	mov	ebx,eax
	imul	ecx,10
	jo	bad_number
	add	edx,ecx
	jnc	get_dec_digit
      bad_number:
	pop	eax
      invalid_number:
	mov	esi,[number_start]
	sub	esi,2
      no_number:
	stc
	ret
      get_bin_number:
	xor	bl,bl
      get_bin_digit:
	cmp	esi,[number_start]
	jb	number_ok
	movzx	eax,byte [esi]
	cmp	bl,64
	je	bad_number
	sub	al,30h
	jc	bad_number
	cmp	al,1
	ja	bad_number
	xor	edx,edx
	mov	cl,bl
	dec	esi
	inc	bl
	cmp	cl,32
	jae	bin_digit_high
	shl	eax,cl
	or	dword [ds:ebp],eax
	jmp	get_bin_digit
      bin_digit_high:
	sub	cl,32
	shl	eax,cl
	or	dword [ds:ebp+4],eax
	jmp	get_bin_digit
      get_hex_number:
	xor	bl,bl
      get_hex_digit:
	cmp	esi,[number_start]
	jb	number_ok
	movzx	eax,byte [esi]
	cmp	al,'x'
	je	hex_number_ok
	cmp	bl,64
	je	bad_number
	sub	al,30h
	jc	bad_number
	cmp	al,9
	jbe	hex_digit_ok
	sub	al,7
	jc	bad_number
	cmp	al,15
	jbe	hex_digit_ok
	sub	al,20h
	jc	bad_number
	cmp	al,15
	ja	bad_number
      hex_digit_ok:
	xor	edx,edx
	mov	cl,bl
	dec	esi
	add	bl,4
	cmp	cl,32
	jae	hex_digit_high
	shl	eax,cl
	or	dword [ds:ebp],eax
	jmp	get_hex_digit
      hex_digit_high:
	sub	cl,32
	shl	eax,cl
	or	dword [ds:ebp+4],eax
	jmp	get_hex_digit
      get_oct_number:
	xor	bl,bl
      get_oct_digit:
	cmp	esi,[number_start]
	jb	number_ok
	movzx	eax,byte [esi]
	cmp	bl,64
	jae	bad_number
	sub	al,30h
	jc	bad_number
	cmp	al,7
	ja	bad_number
      oct_digit_ok:
	xor	edx,edx
	mov	cl,bl
	dec	esi
	add	bl,3
	cmp	cl,32
	jae	oct_digit_high
	shl	eax,cl
	or	dword [ds:ebp],eax
	jmp	get_oct_digit
      oct_digit_high:
	sub	cl,32
	shl	eax,cl
	or	dword [ds:ebp+4],eax
	jmp	get_oct_digit
      get_text_number:
	inc	esi
	xor	bl,bl
	mov	dword [ds:ebp],0
	mov	dword [ds:ebp+4],0
      get_text_character:
	lods	byte [esi]
	cmp	al,1Ah
	je	number_done
	cmp	bl,64
	je	value_out_of_range
	or	al,al
	jz	missing_end_quote
	movzx	eax,al
	xor	edx,edx
	mov	cl,bl
	add	bl,8
	cmp	cl,32
	jae	text_character_high
	shl	eax,cl
	or	dword [ds:ebp],eax
	jmp	get_text_character
      text_character_high:
	sub	cl,32
	shl	eax,cl
	or	dword [ds:ebp+4],eax
	jmp	get_text_character
      hex_number_ok:
	dec	esi
	cmp	esi,[number_start]
	jne	bad_number
      number_ok:
	pop	esi
      number_done:
	clc
	ret

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
	xor	ah,ah
      check_fp_value:
	lods	byte [esi]
	cmp	al,'.'
	je	fp_character_dot
	cmp	al,'E'
	je	fp_character_exp
	cmp	al,'0'
	jb	not_fp_value
	cmp	al,'9'
	ja	not_fp_value
	jmp	fp_character_ok
      fp_character_dot:
	or	ah,ah
	jnz	not_fp_value
	or	ah,1
	jmp	fp_character_ok
      fp_character_exp:
	cmp	ah,1
	ja	not_fp_value
	mov	ah,2
	cmp	ecx,1
	jne	fp_character_ok
	cmp	byte [esi],'+'
	je	fp_exp_sign
	cmp	byte [esi],'-'
	jne	fp_character_ok
      fp_exp_sign:
	inc	esi
	cmp	byte [esi],1Ah
	jne	not_fp_value
	inc	esi
	lods	byte [esi]
	movzx	ecx,al
	inc	ecx
      fp_character_ok:
	loop	check_fp_value
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
      fp_exponent_sign:
	not	ebp
	add	esi,2
	lods	byte [esi]
	movzx	ecx,al
      get_exponent:
	lods	byte [esi]
	sub	al,30h
	imul	edx,10
	cmp	edx,8000h
	jae	value_out_of_range
	movzx	eax,al
	add	edx,eax
	loop	get_exponent
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
      not_fp_value:
	pop	esi edi
	stc
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
	shl	eax,1
	mov	[edi+12],eax
	dec	dword [edi+8]
	shl	edx,1
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
	lods	byte [esi]
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
	cmp	al,10h
	je	get_register
	cmp	al,11h
	je	get_label
	cmp	al,')'
	je	expression_calculated
	cmp	al,']'
	je	expression_calculated
	sub	edi,20h
	mov	ebx,edi
	sub	ebx,20h
	mov	dx,[ebx+24]
	or	dx,[edi+24]
	cmp	al,0E0h
	je	calculate_rva
	cmp	al,0D0h
	je	calculate_not
	cmp	al,0D1h
	je	calculate_neg
	cmp	al,80h
	je	calculate_add
	cmp	al,81h
	je	calculate_sub
	mov	ah,[ebx+26]
	or	ah,[edi+26]
	jnz	invalid_use_of_symbol
	cmp	al,90h
	je	calculate_mul
	cmp	al,91h
	je	calculate_div
	or	dx,dx
	jnz	invalid_value
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
	jmp	invalid_value
      expression_calculated:
	sub	edi,20h
	ret
      get_byte_number:
	mov	word [edi+24],0
	mov	byte [edi+26],0
	xor	eax,eax
	lods	byte [esi]
	stos	dword [edi]
	xor	al,al
	stos	dword [edi]
	add	edi,18h
	jmp	calculate_expression
      get_word_number:
	mov	word [edi+24],0
	mov	byte [edi+26],0
	xor	eax,eax
	lods	word [esi]
	stos	dword [edi]
	xor	ax,ax
	stos	dword [edi]
	add	edi,18h
	jmp	calculate_expression
      get_dword_number:
	mov	word [edi+24],0
	mov	byte [edi+26],0
	movs	dword [edi],[esi]
	xor	eax,eax
	stos	dword [edi]
	add	edi,18h
	jmp	calculate_expression
      get_qword_number:
	mov	word [edi+24],0
	mov	byte [edi+26],0
	movs	dword [edi],[esi]
	movs	dword [edi],[esi]
	add	edi,18h
	jmp	calculate_expression
      get_register:
	mov	byte [edi+25],0
	mov	byte [edi+26],0
	lods	byte [esi]
	mov	[edi+24],al
	xor	eax,eax
	stos	dword [edi]
	stos	dword [edi]
	mov	al,1
	stos	dword [edi]
	xor	al,al
	stos	dword [edi]
	add	edi,10h
	jmp	calculate_expression
      get_label:
	mov	word [edi+24],0
	mov	byte [edi+26],0
	lods	dword [esi]
	or	eax,eax
	jz	current_offset_label
	cmp	eax,1
	je	counter_label
	mov	ebx,eax
	cmp	byte [ebx+8],0
	je	label_undefined
	cmp	byte [ebx+13],0
	je	label_defined
	mov	al,[current_pass]
	cmp	al,[ebx+9]
	jne	label_undefined
      label_defined:
	mov	al,[ebx+11]
	cmp	[next_pass_needed],0
	je	label_type_ok
	cmp	[current_pass],0
	jne	label_type_ok
	xor	al,al
      label_type_ok:
	mov	[edi+26],al
	mov	eax,[ebx]
	stos	dword [edi]
	mov	eax,[ebx+4]
	stos	dword [edi]
	add	edi,18h
	mov	al,[ebx+10]
	or	al,al
	jz	calculate_expression
	cmp	[forced_size],2
	je	calculate_expression
	cmp	[forced_size],1
	jne	check_size
	cmp	[operand_size],0
	jne	calculate_expression
	mov	[operand_size],al
	jmp	calculate_expression
      check_size:
	xchg	[operand_size],al
	or	al,al
	jz	calculate_expression
	cmp	al,[operand_size]
	jne	operand_sizes_do_not_match
	jmp	calculate_expression
      current_offset_label:
	cmp	[reloc_labels],0
	je	get_current_offset
	mov	byte [edi+26],2
      get_current_offset:
	mov	eax,[current_offset]
	sub	eax,[org_start]
	stos	dword [edi]
	xor	eax,eax
	stos	dword [edi]
	add	edi,18h
	jmp	calculate_expression
      counter_label:
	mov	eax,[counter]
	stos	dword [edi]
	xor	eax,eax
	stos	dword [edi]
	add	edi,18h
	jmp	calculate_expression
      label_undefined:
	cmp	[current_pass],0
	jne	invalid_value
	mov	[next_pass_needed],1
	mov	byte [edi+26],0
	xor	eax,eax
	stos	dword [edi]
	stos	dword [edi]
	add	edi,18h
	jmp	calculate_expression
      calculate_add:
	cmp	[next_pass_needed],0
	jne	add_values
	cmp	byte [edi+26],0
	je	add_values
	cmp	byte [ebx+26],0
	je	add_values
	jmp	invalid_use_of_symbol
      add_values:
	mov	al,[edi+26]
	or	[ebx+26],al
	mov	eax,[edi]
	add	[ebx],eax
	mov	eax,[edi+4]
	adc	[ebx+4],eax
	or	dx,dx
	jz	calculate_expression
	push	esi
	mov	esi,ebx
	lea	ebx,[edi+8]
	mov	cl,[edi+24]
	call	add_register
	lea	ebx,[edi+16]
	mov	cl,[edi+25]
	call	add_register
	pop	esi
	jmp	calculate_expression
      add_register:
	or	cl,cl
	jz	add_register_done
      add_register_start:
	cmp	[esi+24],cl
	jne	add_in_second_slot
	mov	eax,[ebx]
	add	[esi+8],eax
	mov	eax,[ebx+4]
	adc	[esi+8+4],eax
	ret
      add_in_second_slot:
	cmp	[esi+25],cl
	jne	create_in_first_slot
	mov	eax,[ebx]
	add	[esi+16],eax
	mov	eax,[ebx+4]
	adc	[esi+16+4],eax
	ret
      create_in_first_slot:
	cmp	byte [esi+24],0
	jne	create_in_second_slot
	mov	[esi+24],cl
	mov	eax,[ebx]
	mov	[esi+8],eax
	mov	eax,[ebx+4]
	mov	[esi+8+4],eax
	ret
      create_in_second_slot:
	cmp	byte [esi+25],0
	jne	invalid_value
	mov	[esi+25],cl
	mov	eax,[ebx]
	mov	[esi+16],eax
	mov	eax,[ebx+4]
	mov	[esi+16+4],eax
      add_register_done:
	ret
      calculate_sub:
	xor	ah,ah
	cmp	[next_pass_needed],0
	jne	sub_values
	mov	ah,[ebx+26]
	mov	al,[edi+26]
	or	al,al
	jz	sub_values
	cmp	al,ah
	jne	invalid_use_of_symbol
	xor	ah,ah
      sub_values:
	mov	byte [ebx+26],ah
	mov	eax,[edi]
	sub	[ebx],eax
	mov	eax,[edi+4]
	sbb	[ebx+4],eax
	or	dx,dx
	jz	calculate_expression
	push	esi
	mov	esi,ebx
	lea	ebx,[edi+8]
	mov	cl,[edi+24]
	call	sub_register
	lea	ebx,[edi+16]
	mov	cl,[edi+25]
	call	sub_register
	pop	esi
	jmp	calculate_expression
      sub_register:
	or	cl,cl
	jz	add_register_done
	not	dword [ebx]
	not	dword [ebx+4]
	add	dword [ebx],1
	adc	dword [ebx+4],0
	jmp	add_register_start
      calculate_mul:
	or	dx,dx
	jz	mul_start
	cmp	word [ebx+24],0
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
	mov	eax,[ebx+8+4]
	xchg	eax,[edi+8+4]
	mov	[ebx+8+4],eax
	mov	eax,[ebx+16]
	xchg	eax,[edi+16]
	mov	[ebx+16],eax
	mov	eax,[ebx+16+4]
	xchg	eax,[edi+16+4]
	mov	[ebx+16+4],eax
	mov	ax,[ebx+24]
	xchg	ax,[edi+24]
	mov	[ebx+24],ax
      mul_start:
	push	esi dx
	mov	esi,ebx
	call	mul_64
	jc	value_out_of_range
	pop	dx
	or	dx,dx
	jz	mul_calculated
	cmp	word [edi+24],0
	jne	invalid_value
	add	esi,8
	cmp	byte [esi+16],0
	je	mul_first_register_ok
	call	mul_64
	jc	value_out_of_range
      mul_first_register_ok:
	add	esi,8
	cmp	byte [esi+9],0
	je	mul_calculated
	call	mul_64
	jc	value_out_of_range
      mul_calculated:
	pop	esi
	jmp	calculate_expression
      calculate_div:
	push	esi dx
	mov	esi,ebx
	call	div_64
	jc	value_out_of_range
	pop	dx
	or	dx,dx
	jz	div_calculated
	add	esi,8
	cmp	byte [esi+16],0
	je	div_first_register_ok
	call	div_64
	or	eax,edx
	jnz	invalid_value
      div_first_register_ok:
	add	esi,8
	cmp	byte [esi+9],0
	je	div_calculated
	call	div_64
	or	eax,edx
	jnz	invalid_value
      div_calculated:
	pop	esi
	jmp	calculate_expression
      calculate_mod:
	push	esi
	mov	esi,ebx
	call	div_64
	jc	value_out_of_range
	mov	[esi],eax
	mov	[esi+4],edx
	pop	esi
	jmp	calculate_expression
      calculate_and:
	mov	eax,[edi]
	and	[ebx],eax
	mov	eax,[edi+4]
	and	[ebx+4],eax
	jmp	calculate_expression
      calculate_or:
	mov	eax,[edi]
	or	[ebx],eax
	mov	eax,[edi+4]
	or	[ebx+4],eax
	jmp	calculate_expression
      calculate_xor:
	cmp	[value_size],1
	je	xor_byte
	cmp	[value_size],2
	je	xor_word
	cmp	[value_size],4
	je	xor_dword
	cmp	[value_size],6
	je	xor_pword
      xor_qword:
	mov	eax,[edi]
	xor	[ebx],eax
	mov	eax,[edi+4]
	xor	[ebx+4],eax
	jmp	calculate_expression
      xor_byte:
	cmp	dword [edi+4],0
	jne	xor_qword
	cmp	word [edi+2],0
	jne	xor_qword
	cmp	byte [edi+1],0
	jne	xor_qword
	mov	al,[edi]
	xor	[ebx],al
	jmp	calculate_expression
      xor_word:
	cmp	dword [edi+4],0
	jne	xor_qword
	cmp	word [edi+2],0
	jne	xor_qword
	mov	ax,[edi]
	xor	[ebx],ax
	jmp	calculate_expression
      xor_dword:
	cmp	dword [edi+4],0
	jne	xor_qword
	mov	eax,[edi]
	xor	[ebx],eax
	jmp	calculate_expression
      xor_pword:
	cmp	word [edi+6],0
	jne	xor_qword
	mov	eax,[edi]
	xor	[ebx],eax
	mov	ax,[edi+4]
	xor	[ebx+4],ax
	jmp	calculate_expression
      calculate_shl:
	mov	eax,dword [edi+4]
	test	eax,1 shl 31
	jnz	shl_negative
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
	jmp	calculate_expression
      shl_high:
	sub	cl,32
	mov	eax,[ebx]
	shl	eax,cl
	mov	[ebx+4],eax
	mov	dword [ebx],0
	jmp	calculate_expression
      shl_negative:
	not	dword [edi]
	not	dword [edi+4]
	add	dword [edi],1
	adc	dword [edi+4],0
      calculate_shr:
	mov	eax,dword [edi+4]
	test	eax,1 shl 31
	jnz	shr_negative
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
	jmp	calculate_expression
      shr_high:
	sub	cl,32
	mov	eax,[ebx+4]
	shr	eax,cl
	mov	[ebx],eax
	mov	dword [ebx+4],0
	jmp	calculate_expression
      shr_negative:
	not	dword [edi]
	not	dword [edi+4]
	add	dword [edi],1
	adc	dword [edi+4],0
	jmp	calculate_shl
      zero_value:
	mov	dword [ebx],0
	mov	dword [ebx+4],0
	jmp	calculate_expression
      calculate_not:
	cmp	word [edi+24],0
	jne	invalid_value
	cmp	byte [edi+26],0
	jne	invalid_use_of_symbol
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
	add	edi,20h
	jmp	calculate_expression
      not_byte:
	cmp	dword [edi+4],0
	jne	not_qword
	cmp	word [edi+2],0
	jne	not_qword
	cmp	byte [edi+1],0
	jne	not_qword
	not	byte [edi]
	add	edi,20h
	jmp	calculate_expression
      not_word:
	cmp	dword [edi+4],0
	jne	not_qword
	cmp	word [edi+2],0
	jne	not_qword
	not	word [edi]
	add	edi,20h
	jmp	calculate_expression
      not_dword:
	cmp	dword [edi+4],0
	jne	not_qword
	not	dword [edi]
	add	edi,20h
	jmp	calculate_expression
      not_pword:
	cmp	word [edi+6],0
	jne	not_qword
	not	dword [edi]
	not	word [edi+4]
	add	edi,20h
	jmp	calculate_expression
      calculate_neg:
	cmp	word [edi+24],0
	jne	invalid_value
	cmp	byte [edi+26],0
	jne	invalid_use_of_symbol
	mov	eax,[edi]
	mov	edx,[edi+4]
	mov	dword [edi],0
	mov	dword [edi+4],0
	sub	[edi],eax
	sbb	[edi+4],edx
	add	edi,20h
	jmp	calculate_expression
      calculate_rva:
	cmp	word [edi+24],0
	jne	invalid_value
	mov	al,[edi+26]
	cmp	al,2
	je	rva_ok
	or	al,al
	jnz	invalid_use_of_symbol
	cmp	[next_pass_needed],0
	je	invalid_use_of_symbol
      rva_ok:
	mov	byte [edi+26],0
	mov	eax,[image_base]
	sub	[edi],eax
	sbb	dword [edi+4],0
	add	edi,20h
	jmp	calculate_expression
      mul_64:
	xor	bl,bl
	test	dword [esi+4],1 shl 31
	jz	mul_first_sign_ok
	not	dword [esi]
	not	dword [esi+4]
	add	dword [esi],1
	adc	dword [esi+4],0
	not	bl
      mul_first_sign_ok:
	test	dword [edi+4],1 shl 31
	jz	mul_second_sign_ok
	not	dword [edi]
	not	dword [edi+4]
	add	dword [edi],1
	adc	dword [edi+4],0
	not	bl
      mul_second_sign_ok:
	cmp	dword [esi+4],0
	jz	mul_numbers
	cmp	dword [edi+4],0
	jnz	mul_error
      mul_numbers:
	mov	eax,[esi+4]
	mul	dword [edi]
	or	edx,edx
	jnz	mul_error
	mov	ecx,eax
	mov	eax,[esi]
	mul	dword [edi+4]
	or	edx,edx
	jnz	mul_error
	add	ecx,eax
	jc	mul_error
	mov	eax,[esi]
	mul	dword [edi]
	add	edx,ecx
	jc	mul_error
	mov	[esi],eax
	mov	[esi+4],edx
	or	bl,bl
	jz	mul_ok
	not	dword [esi]
	not	dword [esi+4]
	add	dword [esi],1
	adc	dword [esi+4],0
      mul_ok:
	clc
	ret
      mul_error:
	stc
	ret
      div_64:
	xor	bl,bl
	cmp	dword [edi],0
	jne	divider_ok
	cmp	dword [edi+4],0
	jne	divider_ok
	cmp	[next_pass_needed],0
	je	value_out_of_range
	jmp	div_done
      divider_ok:
	test	dword [esi+4],1 shl 31
	jz	div_first_sign_ok
	not	dword [esi]
	not	dword [esi+4]
	add	dword [esi],1
	adc	dword [esi+4],0
	not	bl
      div_first_sign_ok:
	test	dword [edi+4],1 shl 31
	jz	div_second_sign_ok
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
	jb	div_high_change
	cmp	ebx,eax
	jae	div_high_done
      div_high_change:
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
      div_done:
	or	bl,bl
	jz	div_ok
	not	dword [esi]
	not	dword [esi+4]
	add	dword [esi],1
	adc	dword [esi+4],0
      div_ok:
	ret
      convert_fp:
	mov	word [edi+24],0
	mov	byte [edi+26],0
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
	test	eax,1 shl 23
	jz	fp_dword_ok
	and	eax,1 shl 23 - 1
	inc	bx
	shr	eax,1
      fp_dword_ok:
	add	bx,7Fh
	cmp	bx,100h
	jae	value_out_of_range
	shl	ebx,23
	or	eax,ebx
	mov	bl,[esi+11]
	shl	ebx,31
	or	eax,ebx
      fp_dword_store:
	mov	[edi],eax
	xor	eax,eax
	mov	[edi+4],eax
	add	esi,12
	ret
      convert_fp_qword:
	xor	eax,eax
	xor	edx,edx
	cmp	word [esi+8],8000h
	je	fp_qword_store
	mov	bx,[esi+8]
	mov	eax,[esi]
	mov	edx,[esi+4]
	shl	eax,1
	rcl	edx,1
	mov	ecx,edx
	shr	edx,12
	shrd	eax,ecx,12
	jnc	fp_qword_ok
	add	eax,1
	adc	edx,0
	test	edx,1 shl 20
	jz	fp_qword_ok
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
	mov	bl,[esi+11]
	shl	ebx,31
	or	edx,ebx
      fp_qword_store:
	mov	[edi],eax
	mov	[edi+4],edx
	add	esi,12
	ret

get_byte_value:
	mov	[value_size],1
	mov	[forced_size],2
	mov	[current_offset],edi
	call	calculate_expression
	cmp	byte [edi+26],0
	jne	invalid_use_of_symbol
	mov	eax,[edi]
	cmp	dword [edi+4],0
	je	byte_positive
	cmp	dword [edi+4],-1
	jne	range_exceeded
	cmp	eax,-80h
	jb	range_exceeded
	ret
      byte_positive:
	cmp	eax,100h
	jae	range_exceeded
      return_value:
	ret
      range_exceeded:
	cmp	[error_line],0
	jne	return_value
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	eax,[home_line]
	mov	[error_home_line],eax
	mov	[error],value_out_of_range
	ret
get_word_value:
	mov	[value_size],2
	mov	[forced_size],2
	mov	[current_offset],edi
	call	calculate_expression
	mov	al,[edi+26]
	cmp	al,2
	je	invalid_use_of_symbol
	mov	[value_type],al
      check_word_value:
	mov	eax,[edi]
	cmp	dword [edi+4],0
	je	word_positive
	cmp	dword [edi+4],-1
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
	mov	[forced_size],2
	mov	[current_offset],edi
	call	calculate_expression
	mov	al,[edi+26]
	mov	[value_type],al
      check_dword_value:
	mov	eax,[edi]
	cmp	dword [edi+4],0
	je	dword_positive
	cmp	dword [edi+4],-1
	jne	range_exceeded
	test	eax,1 shl 31
	jz	range_exceeded
      dword_positive:
	ret
get_pword_value:
	mov	[value_size],6
	mov	[forced_size],2
	mov	[current_offset],edi
	call	calculate_expression
	mov	al,[edi+26]
	mov	[value_type],al
	mov	eax,[edi]
	mov	edx,[edi+4]
	cmp	edx,10000h
	jge	range_exceeded
	cmp	edx,-8000h
	jl	range_exceeded
	ret
get_qword_value:
	mov	[value_size],8
	mov	[forced_size],2
	mov	[current_offset],edi
	call	calculate_expression
	mov	al,[edi+26]
	mov	[value_type],al
	mov	eax,[edi]
	mov	edx,[edi+4]
	ret
get_value:
	mov	[operand_size],0
	mov	[forced_size],0
	call	get_operand_size
	lods	byte [esi]
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

get_address:
	mov	[segment_register],0
	mov	[address_size],0
	mov	[value_size],4
	push	address_ok
	mov	al,[esi]
	and	al,11110000b
	cmp	al,60h
	jne	get_size_prefix
	lods	byte [esi]
	sub	al,60h
	mov	[segment_register],al
	mov	al,[esi]
	and	al,11110000b
      get_size_prefix:
	cmp	al,70h
	jne	calculate_address
	lods	byte [esi]
	sub	al,70h
	cmp	al,4
	ja	invalid_address_size
	mov	[address_size],al
	mov	[value_size],al
      calculate_address:
	mov	[current_offset],edi
	call	calculate_expression
	mov	al,[edi+26]
	cmp	al,1
	je	invalid_use_of_symbol
	or	al,al
	jz	address_symbol_ok
	mov	al,84h
	xchg	[address_size],al
	or	al,al
	jz	address_symbol_ok
	cmp	al,4
	jne	address_sizes_do_not_agree
      address_symbol_ok:
	xor	bx,bx
	xor	cl,cl
	mov	ch,[address_size]
	cmp	word [edi+24],0
	je	check_dword_value
	mov	al,[edi+24]
	lea	edx,[edi+8]
	call	get_address_register
	mov	al,[edi+25]
	lea	edx,[edi+16]
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
	cmp	al,2
	je	address_16bit
	cmp	al,4
	jne	invalid_address
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
      address_16bit:
	or	cl,cl
	jz	check_word_value
	cmp	cl,1
	je	check_word_value
	jmp	invalid_address
      get_address_register:
	or	al,al
	jz	address_register_ok
	cmp	dword [edx+4],0
	jne	invalid_address
	cmp	dword [edx],1
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
	mov	cl,[edx]
	cmp	dword [edx],255
	ja	invalid_address
	jmp	address_register_ok
      address_ok:
	mov	edx,eax
	ret

calculate_logical_expression:
	call	get_logical_value
      logical_loop:
	push	ax
	lods	byte [esi]
	cmp	al,'|'
	je	logical_or
	cmp	al,'&'
	je	logical_and
	dec	esi
	pop	ax
	ret
      logical_or:
	call	get_logical_value
	pop	bx
	or	al,bl
	jmp	logical_loop
      logical_and:
	call	get_logical_value
	pop	bx
	and	al,bl
	jmp	logical_loop

get_logical_value:
	xor	al,al
	cmp	byte [esi],'~'
	jne	negation_ok
	inc	esi
	mov	al,1
      negation_ok:
	push	ax
	cmp	byte [esi],'{'
	je	logical_expression
	push	esi
	cmp	byte [esi],11h
	jne	check_for_values
	add	esi,2
      check_for_values:
	cmp	byte [esi],'('
	jne	find_equ_symbol
	call	skip_symbol
	lods	byte [esi]
	cmp	al,'='
	je	compare_values
	cmp	al,'>'
	je	compare_values
	cmp	al,'<'
	je	compare_values
	cmp	al,'ò'
	je	compare_values
	cmp	al,'ó'
	je	compare_values
	cmp	al,'ö'
	je	compare_values
	dec	esi
      find_equ_symbol:
	cmp	byte [esi],17h
	je	compare_symbols
	cmp	word [esi],13h+1 shl 8
	je	scan_symbols_list
	call	check_character
	jc	invalid_expression
	cmp	al,','
	je	invalid_expression
	call	skip_symbol
	jmp	find_equ_symbol
      compare_symbols:
	add	esi,2
	pop	ebx
	mov	edx,esi
	push	edi
	mov	edi,ebx
	mov	ecx,esi
	sub	ecx,2
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
      scan_symbols_list:
	xor	bp,bp
	add	esi,2
	lods	byte [esi]
	cmp	al,'<'
	jne	invalid_expression
	pop	ebx
	mov	ecx,esi
	sub	ecx,3
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
	call	check_character
	jc	invalid_expression
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
	call	check_character
	jc	invalid_expression
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
	jz	.stop
	cmp	al,80h
	je	.stop
	cmp	al,81h
	je	.stop
	cmp	al,'}'
	je	.stop
	cmp	al,'|'
	je	.stop
	cmp	al,'&'
	je	.stop
	clc
	ret
      .stop:
	stc
	ret
      compare_values:
	pop	esi
	call	get_value
	mov	bl,[value_type]
	push	eax edx bx
	lods	byte [esi]
	mov	[compare_type],al
	call	get_value
	pop	bx
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
	cmp	[compare_type],'ò'
	je	check_not_less
	cmp	[compare_type],'ó'
	je	check_not_greater
	cmp	[compare_type],'ö'
	je	check_not_equal
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
      return_false:
	xor	al,al
	jmp	logical_value_ok
      return_true:
	mov	al,1
	jmp	logical_value_ok
      logical_expression:
	inc	esi
	call	calculate_logical_expression
	push	ax
	lods	byte [esi]
	cmp	al,'}'
	jne	invalid_expression
	pop	ax
      logical_value_ok:
	pop	bx
	xor	al,bl
	ret

number_start dd ?
fp_value rd 8
fp_sign db ?
fp_format db ?
expression_stack dd ?
value_size db ?
forced_size db ?
value_type db ?
address_size db ?
current_offset dd ?
compare_type db ?
