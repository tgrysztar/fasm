
; flat assembler  version 1.10
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
	mov	al,D1h
	jmp	expression_number
      expression_loop:
	push	ebp edi
	mov	edi,single_operand_operators
	call	get_operator
	pop	edi
      expression_number:
	push	ax
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
	and	bl,F0h
	mov	bh,byte [esp]
	and	bh,F0h
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
	add	sp,8
	mov	byte [edi-1],10h
	mov	al,ah
	stos	byte [edi]
	ret

get_number:
	cmp	byte [esi],27h
	je	get_text_number
	cmp	byte [esi],1Ah
	jne	invalid_number
	inc	esi
	lods	byte [esi]
	movzx	ecx,al
	mov	[number_start],esi
	mov	eax,esi
	add	esi,ecx
	push	esi
	sub	esi,2
	mov	dword [ds:ebp],0
	mov	dword [ds:ebp+4],0
	cmp	byte [esi+1],'h'
	je	get_hex_number
	cmp	byte [esi+1],'b'
	je	get_bin_number
	cmp	byte [esi+1],'d'
	je	get_dec_number
	inc	esi
	cmp	word [eax],'0x'
	je	get_hex_number
      get_dec_number:
	xor	edx,edx
	mov	ebx,1
      get_dec_digit:
	cmp	esi,[number_start]
	jb	number_ok
	movzx	eax,byte [esi]
	sub	al,30h
	jc	no_number
	cmp	al,9
	ja	no_number
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
	jo	no_number
	add	edx,ecx
	jnc	get_dec_digit
      no_number:
	pop	eax
      invalid_number:
	mov	esi,[number_start]
	sub	esi,2
	stc
	ret
      get_bin_number:
	xor	bl,bl
      get_bin_digit:
	cmp	esi,[number_start]
	jb	number_ok
	movzx	eax,byte [esi]
	cmp	bl,64
	je	no_number
	sub	al,30h
	jc	no_number
	cmp	al,1
	ja	no_number
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
	je	no_number
	sub	al,30h
	jc	no_number
	cmp	al,9
	jbe	hex_digit_ok
	sub	al,7
	jc	no_number
	cmp	al,15
	ja	no_number
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
	jne	no_number
      number_ok:
	pop	esi
      number_done:
	clc
	ret

calculate_expression:
	lods	byte [esi]
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
	cmp	al,D0h
	je	calculate_not
	cmp	al,D1h
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
	cmp	al,A0h
	je	calculate_mod
	cmp	al,B0h
	je	calculate_and
	cmp	al,B1h
	je	calculate_or
	cmp	al,B2h
	je	calculate_xor
	cmp	al,C0h
	je	calculate_shl
	cmp	al,C1h
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
	cmp	[output_format],3
	jne	get_current_offset
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
	call	check_character
	jc	invalid_expression
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
expression_stack dd ?
value_size db ?
forced_size db ?
value_type db ?
address_size db ?
current_offset dd ?
compare_type db ?
