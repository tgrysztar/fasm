
get_byte_value:
	mov	[current_offset],edi
	mov	[value_size],1
	mov	[expression_start],esi
	call	get_expression
	jc	invalid_value
	cmp	byte [edi+26],0
	jne	invalid_use_of_symbol
	cmp	word [edi+24],0
	jne	invalid_value
	mov	eax,[edi]
	cmp	dword [edi+4],0
	je	byte_positive
	cmp	dword [edi+4],-1
	jne	value_out_of_range
	cmp	eax,-80h
	jb	value_out_of_range
	mov	edi,[current_offset]
	ret
      byte_positive:
	cmp	eax,100h
	jae	value_out_of_range
	mov	edi,[current_offset]
	ret
get_word_value:
	mov	[current_offset],edi
	mov	[value_size],2
	mov	[expression_start],esi
	call	get_expression
	jc	invalid_value
	cmp	byte [edi+26],0
	jne	invalid_use_of_symbol
	cmp	word [edi+24],0
	jne	invalid_value
      check_word_value:
	mov	eax,[edi]
	cmp	dword [edi+4],0
	je	word_positive
	cmp	dword [edi+4],-1
	jne	value_out_of_range
	cmp	eax,-8000h
	jb	value_out_of_range
	mov	edi,[current_offset]
	ret
      word_positive:
	cmp	eax,10000h
	jae	value_out_of_range
	mov	edi,[current_offset]
	clc
	ret
get_dword_value:
	mov	[current_offset],edi
	mov	[value_size],4
	mov	[expression_start],esi
	call	get_expression
	jc	invalid_value
	cmp	word [edi+24],0
	jne	invalid_value
	mov	ch,byte [edi+26]
	mov	[value_type],ch
      check_dword_value:
	mov	eax,[edi]
	cmp	dword [edi+4],0
	je	dword_positive
	cmp	dword [edi+4],-1
	jne	value_out_of_range
	test	eax,1 shl 31
	jz	value_out_of_range
      dword_positive:
	mov	edi,[current_offset]
	clc
	ret
get_pword_value:
	mov	[current_offset],edi
	mov	[value_size],6
	mov	[expression_start],esi
	call	get_expression
	jc	invalid_value
	cmp	byte [edi+26],0
	jne	invalid_use_of_symbol
	cmp	word [edi+24],0
	jne	invalid_value
	mov	eax,[edi]
	mov	edx,[edi+4]
	cmp	edx,10000h
	jge	value_out_of_range
	cmp	edx,-8000h
	jl	value_out_of_range
	mov	edi,[current_offset]
	ret
get_qword_value:
	mov	[current_offset],edi
	mov	[value_size],8
	mov	[expression_start],esi
	call	get_expression
	jc	invalid_value
	cmp	byte [edi+26],0
	jne	invalid_use_of_symbol
	cmp	word [edi+24],0
	jne	invalid_value
	mov	eax,[edi]
	mov	edx,[edi+4]
	mov	edi,[current_offset]
	ret
get_memory_address:
	mov	[expression_start],esi
	mov	[current_offset],edi
	mov	edi,size_operators
	call	get_operator
	mov	[operand_size],al
      find_memory_address_start:
	lodsb
	cmp	al,20h
	je	find_memory_address_start
	cmp	al,'['
	jne	bad_expression
	mov	[segment_register],0
	mov	edi,segment_registers
	push	esi
	call	get_register
	pop	edx
	jc	get_address_expression
	mov	[segment_register],al
	lodsb
	cmp	al,':'
	je	get_address_expression
	mov	esi,edx
      get_address_expression:
	mov	edi,[current_offset]
	mov	[value_size],4
	call	get_expression
	jc	invalid_address
	lodsb
	cmp	al,']'
	jne	invalid_address
	xor	bx,bx
	xor	cx,cx
	mov	ch,byte [edi+26]
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
	jne	bad_expression
      check_address_registers:
	or	al,ah
	cmp	al,2
	je	address_16bit
	cmp	al,4
	jne	bad_expression
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
	jmp	bad_expression
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
	jmp	bad_expression
      get_address_register:
	or	al,al
	jz	address_register_ok
	cmp	dword [edx+4],0
	jne	address_register_error
	cmp	dword [edx],1
	jne	scaled_register
	or	bh,bh
	jnz	scaled_register
	mov	bh,al
      address_register_ok:
	clc
	ret
      scaled_register:
	or	bl,bl
	jnz	address_register_error
	mov	bl,al
	mov	cl,[edx]
	cmp	dword [edx],255
	ja	address_register_error
	jmp	address_register_ok
      address_register_error:
	add	esp,4
	jmp	bad_expression
  get_expression:
	lodsb
	cmp	al,20h
	je	get_expression
	dec	esi
	xor	eax,eax
	stosb
	times 8 stosd
	lodsb
	cmp	al,'-'
	je	value_negative
	cmp	al,'+'
	je	value_positive
	dec	esi
	mov	al,10h
	stosb
	jmp	value_start
      value_positive:
	mov	al,10h
	stosb
	mov	byte [edi+26],0
	jmp	get_value
      value_negative:
	mov	al,11h
	stosb
	mov	byte [edi+26],0
	jmp	get_value
      value_start:
	lodsb
	cmp	al,20h
	je	value_start
	dec	esi
	mov	byte [edi+26],0
	mov	ebp,edi
	mov	edi,single_operand_operators
	call	get_operator
	mov	edi,ebp
	mov	word [edi+24],0
	or	al,al
	jnz	value_loop
      get_value:
	cmp	byte [esi],'('
	jne	value_register
	inc	esi
	push	edi
	call	get_expression
	mov	ebx,esi
	mov	esi,edi
	pop	edi
	mov	ebp,edi
	mov	ecx,8
	rep	movsd
	mov	esi,ebx
	lodsb
	cmp	al,')'
	jne	bad_expression
	jmp	value_operator
      value_register:
	mov	word [edi+24],0
	mov	ebp,edi
	mov	edi,index_registers
	call	get_register
	jc	value_number
	mov	edi,ebp
	mov	[edi+24],al
	xor	eax,eax
	mov	[edi],eax
	mov	[edi+4],eax
	mov	dword [edi+8],1
	mov	[edi+8+4],eax
	mov	[edi+25],al
	jmp	value_operator
      value_number:
	call	get_number
	jnc	value_operator
	mov	ecx,eax
	sub	ecx,esi
	push	ebp
	call	get_symbol
	pop	ebp
	cmp	cl,1
	jb	invalid_number
	ja	store_symbol_value
	mov	[next_pass_needed],1
      store_symbol_value:
	mov	dword [ds:ebp],eax
	mov	dword [ds:ebp+4],edx
	mov	byte [ds:ebp+26],ch
      value_operator:
	mov	edi,operators
	call	get_operator
	mov	edi,ebp
      value_loop:
	mov	bl,[edi-1]
	and	bl,0F0h
	mov	ah,al
	and	ah,0F0h
	cmp	bl,ah
	jbe	value_next
	mov	ebp,edi
      find_calculations_start:
	sub	edi,32+1
	mov	bh,[edi-1]
	and	bh,0F0h
	cmp	bh,bl
	je	find_calculations_start
	push	eax esi
	mov	esi,edi
      calculate:
	add	edi,32+1
	cmp	edi,[symbols_list]
	jae	out_of_memory
	mov	al,[edi-1]
	cmp	al,10h
	je	calculate_add
	cmp	al,11h
	je	calculate_sub
	cmp	al,20h
	je	calculate_mul
	cmp	al,21h
	je	calculate_div
	mov	dx,[esi+24]
	or	dx,[edi+24]
	jnz	calculate_error
	cmp	al,30h
	je	calculate_mod
	cmp	al,40h
	je	calculate_and
	cmp	al,41h
	je	calculate_or
	cmp	al,42h
	je	calculate_xor
	cmp	al,50h
	je	calculate_not
	cmp	al,60h
	je	calculate_shl
	cmp	al,61h
	je	calculate_shr
	jmp	calculate_done
      calculate_add:
	mov	al,[esi+26]
	and	al,[edi+26]
	jnz	invalid_use_of_symbol
	mov	al,[edi+26]
	or	[esi+26],al
	mov	eax,[edi]
	add	[esi],eax
	mov	eax,[edi+4]
	adc	[esi+4],eax
	lea	ebx,[edi+8]
	mov	cl,[edi+24]
	call	add_register
	lea	ebx,[edi+16]
	mov	cl,[edi+25]
	call	add_register
	jmp	calculate_done
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
	jne	add_register_error
	mov	[esi+25],cl
	mov	eax,[ebx]
	mov	[esi+16],eax
	mov	eax,[ebx+4]
	mov	[esi+16+4],eax
      add_register_done:
	ret
      add_register_error:
	add	esp,4
	jmp	calculate_error
      calculate_sub:
	mov	al,[esi+26]
	or	al,[edi+26]
	jz	sub_allowed
	cmp	al,1
	jne	invalid_use_of_symbol
	mov	al,[esi+26]
	and	al,[esi+26]
	cmp	al,1
	jne	invalid_use_of_symbol
	xor	al,al
	mov	[esi+26],al
	mov	[edi+26],al
      sub_allowed:
	mov	eax,[edi]
	sub	[esi],eax
	mov	eax,[edi+4]
	sbb	[esi+4],eax
	lea	ebx,[edi+8]
	mov	cl,[edi+24]
	call	sub_register
	lea	ebx,[edi+16]
	mov	cl,[edi+25]
	call	sub_register
	jmp	calculate_done
      sub_register:
	or	cl,cl
	jz	add_register_done
	not	dword [ebx]
	not	dword [ebx+4]
	add	dword [ebx],1
	adc	dword [ebx+4],0
	jmp	add_register_start
      calculate_mul:
	mov	al,[esi+26]
	or	al,[edi+26]
	jnz	invalid_use_of_symbol
	cmp	word [esi+24],0
	jne	mul_start
	mov	eax,[esi]
	xchg	eax,[edi]
	mov	[esi],eax
	mov	eax,[esi+4]
	xchg	eax,[edi+4]
	mov	[esi+4],eax
	mov	eax,[esi+8]
	xchg	eax,[edi+8]
	mov	[esi+8],eax
	mov	eax,[esi+8+4]
	xchg	eax,[edi+8+4]
	mov	[esi+8+4],eax
	mov	eax,[esi+16]
	xchg	eax,[edi+16]
	mov	[esi+16],eax
	mov	eax,[esi+16+4]
	xchg	eax,[edi+16+4]
	mov	[esi+16+4],eax
	mov	ax,[esi+24]
	xchg	ax,[edi+24]
	mov	[esi+24],ax
      mul_start:
	call	mul_64
	jc	calculate_error
	cmp	word [edi+24],0
	jne	calculate_error
	add	esi,8
	cmp	byte [esi+16],0
	je	mul_first_register_ok
	call	mul_64
	jc	calculate_error
      mul_first_register_ok:
	add	esi,8
	cmp	byte [esi+9],0
	je	mul_second_register_ok
	call	mul_64
	jc	calculate_error
      mul_second_register_ok:
	sub	esi,16
	jmp	calculate_done
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
      calculate_div:
	mov	al,[esi+26]
	or	al,[edi+26]
	jnz	invalid_use_of_symbol
	call	div_64
	cmp	word [edi+24],0
	jne	calculate_error
	add	esi,8
	cmp	byte [esi+16],0
	je	div_first_register_ok
	call	div_64
	or	eax,edx
	jnz	calculate_error
      div_first_register_ok:
	add	esi,8
	cmp	byte [esi+9],0
	je	div_second_register_ok
	call	div_64
	or	eax,edx
	jnz	calculate_error
      div_second_register_ok:
	sub	esi,16
	jmp	calculate_done
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
      calculate_mod:
	mov	al,[esi+26]
	or	al,[edi+26]
	jnz	invalid_use_of_symbol
	call	div_64
	mov	[esi],eax
	mov	[esi+4],edx
	jmp	calculate_done
      calculate_and:
	mov	al,[esi+26]
	or	al,[edi+26]
	jnz	invalid_use_of_symbol
	mov	eax,[edi]
	and	[esi],eax
	mov	eax,[edi+4]
	and	[esi+4],eax
	jmp	calculate_done
      calculate_or:
	mov	al,[esi+26]
	or	al,[edi+26]
	jnz	invalid_use_of_symbol
	mov	eax,[edi]
	or	[esi],eax
	mov	eax,[edi+4]
	or	[esi+4],eax
	jmp	calculate_done
      calculate_xor:
	mov	al,[esi+26]
	or	al,[edi+26]
	jnz	invalid_use_of_symbol
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
	xor	[esi],eax
	mov	eax,[edi+4]
	xor	[esi+4],eax
	jmp	calculate_done
      xor_byte:
	cmp	dword [edi+4],0
	jne	xor_qword
	cmp	word [edi+2],0
	jne	xor_qword
	cmp	byte [edi+1],0
	jne	xor_qword
	mov	al,[edi]
	xor	[esi],al
	jmp	calculate_done
      xor_word:
	cmp	dword [edi+4],0
	jne	xor_qword
	cmp	word [edi+2],0
	jne	xor_qword
	mov	ax,[edi]
	xor	[esi],ax
	jmp	calculate_done
      xor_dword:
	cmp	dword [edi+4],0
	jne	xor_qword
	mov	eax,[edi]
	xor	[esi],eax
	jmp	calculate_done
      xor_pword:
	cmp	word [edi+6],0
	jne	xor_qword
	mov	eax,[edi]
	xor	[esi],eax
	mov	ax,[edi+4]
	xor	[esi+4],ax
	jmp	calculate_done
      calculate_not:
	cmp	byte [esi+26],0
	jnz	invalid_use_of_symbol
	mov	dword [esi],0
	mov	dword [esi+4],0
	cmp	[value_size],1
	je	not_byte
	cmp	[value_size],2
	je	not_word
	cmp	[value_size],4
	je	not_dword
	cmp	[value_size],6
	je	not_pword
      not_qword:
	mov	eax,[edi]
	not	eax
	mov	[esi],eax
	mov	eax,[edi+4]
	not	eax
	mov	[esi+4],eax
	jmp	calculate_done
      not_byte:
	cmp	dword [edi+4],0
	jne	not_qword
	cmp	word [edi+2],0
	jne	not_qword
	cmp	byte [edi+1],0
	jne	not_qword
	mov	al,[edi]
	not	al
	mov	[esi],al
	jmp	calculate_done
      not_word:
	cmp	dword [edi+4],0
	jne	not_qword
	cmp	word [edi+2],0
	jne	not_qword
	mov	ax,[edi]
	not	ax
	mov	[esi],ax
	jmp	calculate_done
      not_dword:
	cmp	dword [edi+4],0
	jne	not_qword
	mov	eax,[edi]
	not	eax
	mov	[esi],eax
	jmp	calculate_done
      not_pword:
	cmp	word [edi+6],0
	jne	not_qword
	mov	eax,[edi]
	not	eax
	mov	[esi],eax
	mov	ax,[edi+4]
	not	ax
	mov	[esi+4],ax
	jmp	calculate_done
      calculate_shl:
	mov	al,[esi+26]
	or	al,[edi+26]
	jnz	invalid_use_of_symbol
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
	mov	edx,[esi+4]
	mov	eax,[esi]
	shld	edx,eax,cl
	shl	eax,cl
	mov	[esi],eax
	mov	[esi+4],edx
	jmp	calculate_done
      shl_high:
	sub	cl,32
	mov	eax,[esi]
	shl	eax,cl
	mov	[esi+4],eax
	mov	dword [esi],0
	jmp	calculate_done
      shl_negative:
	not	dword [edi]
	not	dword [edi+4]
	add	dword [edi],1
	adc	dword [edi+4],0
      calculate_shr:
	mov	al,[esi+26]
	or	al,[edi+26]
	jnz	invalid_use_of_symbol
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
	mov	edx,[esi+4]
	mov	eax,[esi]
	shrd	eax,edx,cl
	shr	edx,cl
	mov	[esi],eax
	mov	[esi+4],edx
	jmp	calculate_done
      shr_high:
	sub	cl,32
	mov	eax,[esi+4]
	shr	eax,cl
	mov	[esi],eax
	mov	dword [esi+4],0
	jmp	calculate_done
      shr_negative:
	not	dword [edi]
	not	dword [edi+4]
	add	dword [edi],1
	adc	dword [edi+4],0
	jmp	calculate_shl
      zero_value:
	mov	dword [esi],0
	mov	dword [esi+4],0
	jmp	calculate_done
      calculate_error:
	add	esp,8
	jmp	bad_expression
      calculate_done:
	cmp	byte [esi+24],0
	je	first_register_ok
	mov	eax,[esi+8]
	or	eax,[esi+8+4]
	jnz	first_register_ok
	mov	byte [esi+24],0
      first_register_ok:
	cmp	byte [esi+25],0
	je	second_register_ok
	mov	eax,[esi+16]
	or	eax,[esi+16+4]
	jnz	second_register_ok
	mov	byte [esi+25],0
      second_register_ok:
	cmp	edi,ebp
	jne	calculate
	mov	edi,esi
	pop	esi eax
	jmp	value_loop
      value_next:
	or	al,al
	jz	value_end
	add	edi,32
	stosb
	jmp	value_start
      value_end:
	clc
	ret
      bad_expression:
	mov	esi,[expression_start]
	mov	edi,[current_offset]
	stc
	ret
get_number:
	mov	[number_start],esi
	cmp	byte [esi],27h
	je	get_text_number
      find_number_end:
	lodsb
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	jne	find_number_end
      number_end:
	dec	esi
	cmp	esi,[number_start]
	je	no_number
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
      get_dec_number:
	xor	edx,edx
	mov	ebx,1
      get_dec_digit:
	movzx	eax,byte [esi]
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	je	number_ok
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
	mov	esi,[number_start]
	pop	eax
      invalid_number:
	stc
	ret
      get_bin_number:
	xor	bl,bl
      get_bin_digit:
	movzx	eax,byte [esi]
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	je	number_ok
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
	movzx	eax,byte [esi]
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	je	number_ok
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
	lodsb
	cmp	al,27h
	jne	text_character_ok
	lodsb
	cmp	al,27h
	je	text_character_ok
	dec	esi
	clc
	ret
      text_character_ok:
	cmp	bl,64
	je	no_number
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
      number_ok:
	pop	esi
	clc
	ret

expression_start dd 0
number_start dd 0
value_size db 0
value dd 0
value_type db 0
import_value dd 0
