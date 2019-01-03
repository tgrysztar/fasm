
; flat assembler core
; Copyright (c) 1999-2019, Tomasz Grysztar.
; All rights reserved.

convert_expression:
	push	ebp
	call	get_fp_value
	jnc	fp_expression
	mov	[current_offset],esp
      expression_loop:
	push	edi
	mov	edi,single_operand_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	expression_element
	cmp	al,82h
	je	expression_loop
	push	eax
	jmp	expression_loop
      expression_element:
	mov	al,[esi]
	cmp	al,1Ah
	je	expression_number
	cmp	al,22h
	je	expression_number
	cmp	al,'('
	je	expression_number
	mov	al,'!'
	stos	byte [edi]
	jmp	expression_operator
      expression_number:
	call	convert_number
      expression_operator:
	push	edi
	mov	edi,operators
	call	get_operator
	pop	edi
	or	al,al
	jz	expression_end
      operators_loop:
	cmp	esp,[current_offset]
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
	cmp	esp,[current_offset]
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
	lea	eax,[edi+20h]
	mov	edx,[memory_end]
	cmp	[source_start],0
	je	check_memory_for_number
	mov	edx,[labels_list]
      check_memory_for_number:
	cmp	eax,edx
	jae	out_of_memory
	mov	eax,esp
	sub	eax,[stack_limit]
	cmp	eax,100h
	jb	stack_overflow
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
	inc	esi
	push	[current_offset]
	call	convert_expression
	pop	[current_offset]
	lods	byte [esi]
	cmp	al,')'
	je	subexpression_closed
	dec	esi
	mov	al,'!'
	stosb
      subexpression_closed:
	ret
      symbol_value:
	cmp	[source_start],0
	je	preprocessor_value
	push	edi esi
	lods	word [esi]
	cmp	al,1Ah
	jne	no_address_register
	movzx	ecx,ah
	call	get_symbol
	jc	no_address_register
	cmp	al,10h
	jne	no_address_register
	mov	al,ah
	shr	ah,4
	cmp	ah,4
	je	register_value
	and	ah,not 1
	cmp	ah,8
	je	register_value
	cmp	ah,0Ch
	jae	register_value
	cmp	ah,6
	je	register_value
	cmp	al,23h
	je	register_value
	cmp	al,25h
	je	register_value
	cmp	al,26h
	je	register_value
	cmp	al,27h
	je	register_value
      no_address_register:
	pop	esi
	mov	edi,directive_operators
	call	get_operator
	pop	edi
	or	al,al
	jnz	broken_value
	lods	byte [esi]
	cmp	al,1Ah
	jne	invalid_value
	lods	byte [esi]
	movzx	ecx,al
	call	get_label_id
      store_label_value:
	mov	byte [edi-1],11h
	stos	dword [edi]
	ret
      broken_value:
	mov	eax,0Fh
	jmp	store_label_value
      register_value:
	pop	edx edi
	mov	byte [edi-1],10h
	stos	byte [edi]
	ret
      preprocessor_value:
	dec	edi
	cmp	[hash_tree],0
	je	invalid_value
	lods	byte [esi]
	cmp	al,1Ah
	jne	invalid_value
	lods	byte [esi]
	mov	cl,al
	mov	ch,10b
	call	get_preprocessor_symbol
	jc	invalid_value
	push	esi
	mov	esi,[edx+8]
	push	[current_offset]
	call	convert_expression
	pop	[current_offset]
	pop	esi
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
	cmp	al,'q'
	je	get_oct_number
	cmp	al,'H'
	je	get_hex_number
	cmp	al,'B'
	je	get_bin_number
	cmp	al,'D'
	je	get_dec_number
	cmp	al,'O'
	je	get_oct_number
	cmp	al,'Q'
	je	get_oct_number
	inc	esi
      get_dec_number:
	mov	ebx,esi
	mov	esi,[number_start]
      get_dec_digit:
	cmp	esi,ebx
	ja	number_ok
	cmp	byte [esi],27h
	je	next_dec_digit
	cmp	byte [esi],'_'
	je	next_dec_digit
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
	jc	bad_number
	cmp	al,9
	ja	bad_number
	add	[edi],eax
	adc	dword [edi+4],0
	jc	dec_out_of_range
      next_dec_digit:
	inc	esi
	jmp	get_dec_digit
      dec_out_of_range:
	cmp	esi,ebx
	ja	dec_out_of_range_finished
	lods	byte [esi]
	cmp	al,27h
	je	bad_number
	cmp	al,'_'
	je	bad_number
	sub	al,30h
	jc	bad_number
	cmp	al,9
	ja	bad_number
	jmp	dec_out_of_range
      dec_out_of_range_finished:
	or	ebp,-1
	jmp	number_ok
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
	cmp	al,27h
	je	bin_digit_skip
	cmp	al,'_'
	je	bin_digit_skip
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
      bin_digit_skip:
	dec	esi
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
	cmp	al,27h
	je	hex_digit_skip
	cmp	al,'_'
	je	hex_digit_skip
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
      hex_digit_skip:
	dec	esi
	jmp	get_hex_digit
      get_oct_number:
	xor	bl,bl
      get_oct_digit:
	cmp	esi,[number_start]
	jb	number_ok
	movzx	eax,byte [esi]
	cmp	al,27h
	je	oct_digit_skip
	cmp	al,'_'
	je	oct_digit_skip
	sub	al,30h
	cmp	al,7
	ja	bad_number
      oct_digit_ok:
	xor	edx,edx
	mov	cl,bl
	dec	esi
	cmp	bl,63
	ja	oct_out_of_range
	jne	oct_range_ok
	cmp	al,1
	ja	oct_out_of_range
      oct_range_ok:
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
      oct_digit_skip:
	dec	esi
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
      fp_last_character:
	cmp	cl,1
	jne	not_fp_value
	or	ah,4
	jmp	fp_character_ok
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
	dec	ecx
	jnz	fp_after_dot
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

preevaluate_logical_expression:
	xor	al,al
  preevaluate_embedded_logical_expression:
	mov	[logical_value_wrapping],al
	push	edi
	call	preevaluate_logical_value
      preevaluation_loop:
	cmp	al,0FFh
	je	invalid_logical_expression
	mov	dl,[esi]
	inc	esi
	cmp	dl,'|'
	je	preevaluate_or
	cmp	dl,'&'
	je	preevaluate_and
	cmp	dl,92h
	je	preevaluation_done
	or	dl,dl
	jnz	invalid_logical_expression
      preevaluation_done:
	pop	edx
	dec	esi
	ret
      preevaluate_or:
	cmp	al,'1'
	je	quick_true
	cmp	al,'0'
	je	leave_only_following
	push	edi
	mov	al,dl
	stos	byte [edi]
	call	preevaluate_logical_value
	pop	ebx
	cmp	al,'0'
	je	leave_only_preceding
	cmp	al,'1'
	jne	preevaluation_loop
	stos	byte [edi]
	xor	al,al
	jmp	preevaluation_loop
      preevaluate_and:
	cmp	al,'0'
	je	quick_false
	cmp	al,'1'
	je	leave_only_following
	push	edi
	mov	al,dl
	stos	byte [edi]
	call	preevaluate_logical_value
	pop	ebx
	cmp	al,'1'
	je	leave_only_preceding
	cmp	al,'0'
	jne	preevaluation_loop
	stos	byte [edi]
	xor	al,al
	jmp	preevaluation_loop
      leave_only_following:
	mov	edi,[esp]
	call	preevaluate_logical_value
	jmp	preevaluation_loop
      leave_only_preceding:
	mov	edi,ebx
	xor	al,al
	jmp	preevaluation_loop
      quick_true:
	call	skip_logical_value
	jc	invalid_logical_expression
	mov	edi,[esp]
	mov	al,'1'
	jmp	preevaluation_loop
      quick_false:
	call	skip_logical_value
	jc	invalid_logical_expression
	mov	edi,[esp]
	mov	al,'0'
	jmp	preevaluation_loop
      invalid_logical_expression:
	pop	edi
	mov	esi,edi
	mov	al,0FFh
	stos	byte [edi]
	ret
  skip_logical_value:
	cmp	byte [esi],'~'
	jne	negation_skipped
	inc	esi
	jmp	skip_logical_value
      negation_skipped:
	mov	al,[esi]
	cmp	al,91h
	jne	skip_simple_logical_value
	inc	esi
	xchg	al,[logical_value_wrapping]
	push	eax
      skip_logical_expression:
	call	skip_logical_value
	lods	byte [esi]
	or	al,al
	jz	wrongly_structured_logical_expression
	cmp	al,0Fh
	je	wrongly_structured_logical_expression
	cmp	al,'|'
	je	skip_logical_expression
	cmp	al,'&'
	je	skip_logical_expression
	cmp	al,92h
	jne	wrongly_structured_logical_expression
	pop	eax
	mov	[logical_value_wrapping],al
      logical_value_skipped:
	clc
	ret
      wrongly_structured_logical_expression:
	pop	eax
	stc
	ret
      skip_simple_logical_value:
	mov	[logical_value_parentheses],0
      find_simple_logical_value_end:
	mov	al,[esi]
	or	al,al
	jz	logical_value_skipped
	cmp	al,0Fh
	je	logical_value_skipped
	cmp	al,'|'
	je	logical_value_skipped
	cmp	al,'&'
	je	logical_value_skipped
	cmp	al,91h
	je	skip_logical_value_internal_parenthesis
	cmp	al,92h
	jne	skip_logical_value_symbol
	sub	[logical_value_parentheses],1
	jnc	skip_logical_value_symbol
	cmp	[logical_value_wrapping],91h
	jne	skip_logical_value_symbol
	jmp	logical_value_skipped
      skip_logical_value_internal_parenthesis:
	inc	[logical_value_parentheses]
      skip_logical_value_symbol:
	call	skip_symbol
	jmp	find_simple_logical_value_end
  preevaluate_logical_value:
	mov	ebp,edi
      preevaluate_negation:
	cmp	byte [esi],'~'
	jne	preevaluate_negation_ok
	movs	byte [edi],[esi]
	jmp	preevaluate_negation
      preevaluate_negation_ok:
	mov	ebx,esi
	cmp	byte [esi],91h
	jne	preevaluate_simple_logical_value
	lods	byte [esi]
	stos	byte [edi]
	push	ebp
	mov	dl,[logical_value_wrapping]
	push	edx
	call	preevaluate_embedded_logical_expression
	pop	edx
	mov	[logical_value_wrapping],dl
	pop	ebp
	cmp	al,0FFh
	je	invalid_logical_value
	cmp	byte [esi],92h
	jne	invalid_logical_value
	or	al,al
	jnz	preevaluated_expression_value
	movs	byte [edi],[esi]
	ret
      preevaluated_expression_value:
	inc	esi
	lea	edx,[edi-1]
	sub	edx,ebp
	test	edx,1
	jz	expression_negation_ok
	xor	al,1
      expression_negation_ok:
	mov	edi,ebp
	ret
      invalid_logical_value:
	mov	edi,ebp
	mov	al,0FFh
	ret
      preevaluate_simple_logical_value:
	xor	edx,edx
	mov	[logical_value_parentheses],edx
      find_logical_value_boundaries:
	mov	al,[esi]
	or	al,al
	jz	logical_value_boundaries_found
	cmp	al,91h
	je	logical_value_internal_parentheses
	cmp	al,92h
	je	logical_value_boundaries_parenthesis_close
	cmp	al,'|'
	je	logical_value_boundaries_found
	cmp	al,'&'
	je	logical_value_boundaries_found
	or	edx,edx
	jnz	next_symbol_in_logical_value
	cmp	al,0F0h
	je	preevaluable_logical_operator
	cmp	al,0F7h
	je	preevaluable_logical_operator
	cmp	al,0F6h
	jne	next_symbol_in_logical_value
      preevaluable_logical_operator:
	mov	edx,esi
      next_symbol_in_logical_value:
	call	skip_symbol
	jmp	find_logical_value_boundaries
      logical_value_internal_parentheses:
	inc	[logical_value_parentheses]
	jmp	next_symbol_in_logical_value
      logical_value_boundaries_parenthesis_close:
	sub	[logical_value_parentheses],1
	jnc	next_symbol_in_logical_value
	cmp	[logical_value_wrapping],91h
	jne	next_symbol_in_logical_value
      logical_value_boundaries_found:
	or	edx,edx
	jz	non_preevaluable_logical_value
	mov	al,[edx]
	cmp	al,0F0h
	je	compare_symbols
	cmp	al,0F7h
	je	compare_symbol_types
	cmp	al,0F6h
	je	scan_symbols_list
      non_preevaluable_logical_value:
	mov	ecx,esi
	mov	esi,ebx
	sub	ecx,esi
	jz	invalid_logical_value
	cmp	esi,edi
	je	leave_logical_value_intact
	rep	movs byte [edi],[esi]
	xor	al,al
	ret
      leave_logical_value_intact:
	add	edi,ecx
	add	esi,ecx
	xor	al,al
	ret
      compare_symbols:
	lea	ecx,[esi-1]
	sub	ecx,edx
	mov	eax,edx
	sub	eax,ebx
	cmp	ecx,eax
	jne	preevaluated_false
	push	esi edi
	mov	esi,ebx
	lea	edi,[edx+1]
	repe	cmps byte [esi],[edi]
	pop	edi esi
	je	preevaluated_true
      preevaluated_false:
	mov	eax,edi
	sub	eax,ebp
	test	eax,1
	jnz	store_true
      store_false:
	mov	edi,ebp
	mov	al,'0'
	ret
      preevaluated_true:
	mov	eax,edi
	sub	eax,ebp
	test	eax,1
	jnz	store_false
      store_true:
	mov	edi,ebp
	mov	al,'1'
	ret
      compare_symbol_types:
	push	esi
	lea	esi,[edx+1]
      type_comparison:
	cmp	esi,[esp]
	je	types_compared
	mov	al,[esi]
	cmp	al,[ebx]
	jne	different_type
	cmp	al,'('
	jne	equal_type
	mov	al,[esi+1]
	mov	ah,[ebx+1]
	cmp	al,ah
	je	equal_type
	or	al,al
	jz	different_type
	or	ah,ah
	jz	different_type
	cmp	al,'.'
	je	different_type
	cmp	ah,'.'
	je	different_type
      equal_type:
	call	skip_symbol
	xchg	esi,ebx
	call	skip_symbol
	xchg	esi,ebx
	jmp	type_comparison
      types_compared:
	pop	esi
	cmp	byte [ebx],0F7h
	jne	preevaluated_false
	jmp	preevaluated_true
      different_type:
	pop	esi
	jmp	preevaluated_false
      scan_symbols_list:
	push	edi esi
	lea	esi,[edx+1]
	sub	edx,ebx
	lods	byte [esi]
	cmp	al,'<'
	jne	invalid_symbols_list
      get_next_from_list:
	mov	edi,esi
      get_from_list:
	cmp	byte [esi],','
	je	compare_in_list
	cmp	byte [esi],'>'
	je	compare_in_list
	cmp	esi,[esp]
	jae	invalid_symbols_list
	call	skip_symbol
	jmp	get_from_list
      compare_in_list:
	mov	ecx,esi
	sub	ecx,edi
	cmp	ecx,edx
	jne	not_equal_length_in_list
	mov	esi,ebx
	repe	cmps byte [esi],[edi]
	mov	esi,edi
	jne	not_equal_in_list
      skip_rest_of_list:
	cmp	byte [esi],'>'
	je	check_list_end
	cmp	esi,[esp]
	jae	invalid_symbols_list
	call	skip_symbol
	jmp	skip_rest_of_list
      check_list_end:
	inc	esi
	cmp	esi,[esp]
	jne	invalid_symbols_list
	pop	esi edi
	jmp	preevaluated_true
      not_equal_in_list:
	add	esi,ecx
      not_equal_length_in_list:
	lods	byte [esi]
	cmp	al,','
	je	get_next_from_list
	cmp	esi,[esp]
	jne	invalid_symbols_list
	pop	esi edi
	jmp	preevaluated_false
      invalid_symbols_list:
	pop	esi edi
	jmp	invalid_logical_value
