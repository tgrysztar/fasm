
; flat assembler core
; Copyright (c) 1999-2019, Tomasz Grysztar.
; All rights reserved.

calculate_expression:
	mov	[current_offset],edi
	mov	[value_undefined],0
	cmp	byte [esi],0
	je	get_string_value
	cmp	byte [esi],'.'
	je	convert_fp
      calculation_loop:
	mov	eax,[tagged_blocks]
	sub	eax,0Ch
	cmp	eax,edi
	jbe	out_of_memory
	lods	byte [esi]
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
	cmp	al,'!'
	je	invalid_expression
	sub	edi,14h
	mov	ebx,edi
	sub	ebx,14h
	cmp	al,0F0h
	je	calculate_rva
	cmp	al,0F1h
	je	calculate_plt
	cmp	al,0D0h
	je	calculate_not
	cmp	al,0E0h
	je	calculate_bsf
	cmp	al,0E1h
	je	calculate_bsr
	cmp	al,083h
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
	call	recoverable_misuse
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
	mov	[edi+12],eax
      expression_value_ok:
	ret
      get_byte_number:
	xor	eax,eax
	lods	byte [esi]
	stos	dword [edi]
	xor	al,al
	stos	dword [edi]
      got_number:
	and	word [edi-8+8],0
	and	word [edi-8+12],0
	and	dword [edi-8+16],0
	add	edi,0Ch
	jmp	calculation_loop
      get_word_number:
	xor	eax,eax
	lods	word [esi]
	stos	dword [edi]
	xor	ax,ax
	stos	dword [edi]
	jmp	got_number
      get_dword_number:
	movs	dword [edi],[esi]
	xor	eax,eax
	stos	dword [edi]
	jmp	got_number
      get_qword_number:
	movs	dword [edi],[esi]
	movs	dword [edi],[esi]
	jmp	got_number
      get_register:
	mov	byte [edi+9],0
	and	word [edi+12],0
	lods	byte [esi]
	mov	[edi+8],al
	mov	byte [edi+10],1
	xor	eax,eax
	mov	[edi+16],eax
	stos	dword [edi]
	stos	dword [edi]
	add	edi,0Ch
	jmp	calculation_loop
      get_label:
	xor	eax,eax
	mov	[edi+8],eax
	mov	[edi+12],eax
	mov	[edi+20],eax
	lods	dword [esi]
	cmp	eax,0Fh
	jb	predefined_label
	je	reserved_word_used_as_symbol
	mov	ebx,eax
	mov	ax,[current_pass]
	mov	[ebx+18],ax
	mov	cl,[ebx+9]
	shr	cl,1
	and	cl,1
	neg	cl
	or	byte [ebx+8],8
	test	byte [ebx+8],1
	jz	label_undefined
	cmp	ax,[ebx+16]
	je	unadjusted_label
	test	byte [ebx+8],4
	jnz	label_out_of_scope
	test	byte [ebx+9],1
	jz	unadjusted_label
	mov	eax,[ebx]
	sub	eax,dword [adjustment]
	stos	dword [edi]
	mov	eax,[ebx+4]
	sbb	eax,dword [adjustment+4]
	stos	dword [edi]
	sbb	cl,[adjustment_sign]
	mov	[edi-8+13],cl
	mov	eax,dword [adjustment]
	or	al,[adjustment_sign]
	or	eax,dword [adjustment+4]
	jz	got_label
	or	[next_pass_needed],-1
	jmp	got_label
      unadjusted_label:
	mov	eax,[ebx]
	stos	dword [edi]
	mov	eax,[ebx+4]
	stos	dword [edi]
	mov	[edi-8+13],cl
      got_label:
	test	byte [ebx+9],4
	jnz	invalid_use_of_symbol
	call	store_label_reference
	mov	al,[ebx+11]
	mov	[edi-8+12],al
	mov	eax,[ebx+12]
	mov	[edi-8+8],eax
	cmp	al,ah
	jne	labeled_registers_ok
	shr	eax,16
	add	al,ah
	jo	labeled_registers_ok
	xor	ah,ah
	mov	[edi-8+10],ax
	mov	[edi-8+9],ah
      labeled_registers_ok:
	mov	eax,[ebx+20]
	mov	[edi-8+16],eax
	add	edi,0Ch
	mov	al,[ebx+10]
	or	al,al
	jz	calculation_loop
	test	[operand_flags],1
	jnz	calculation_loop
      check_size:
	xchg	[operand_size],al
	or	al,al
	jz	calculation_loop
	cmp	al,[operand_size]
	jne	operand_sizes_do_not_match
	jmp	calculation_loop
      current_offset_label:
	mov	eax,[current_offset]
      make_current_offset_label:
	xor	edx,edx
	xor	ch,ch
	mov	ebp,[addressing_space]
	sub	eax,[ds:ebp]
	sbb	edx,[ds:ebp+4]
	sbb	ch,[ds:ebp+8]
	jp	current_offset_label_ok
	call	recoverable_overflow
      current_offset_label_ok:
	stos	dword [edi]
	mov	eax,edx
	stos	dword [edi]
	mov	eax,[ds:ebp+10h]
	stos	dword [edi]
	mov	cl,[ds:ebp+9]
	mov	[edi-12+12],cx
	mov	eax,[ds:ebp+14h]
	mov	[edi-12+16],eax
	add	edi,8
	jmp	calculation_loop
      org_origin_label:
	mov	eax,[addressing_space]
	mov	eax,[eax+18h]
	jmp	make_current_offset_label
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
      make_qword_label_value:
	stos	dword [edi]
	mov	eax,edx
	stos	dword [edi]
	add	edi,0Ch
	jmp	calculation_loop
      predefined_label:
	or	eax,eax
	jz	current_offset_label
	cmp	eax,1
	je	counter_label
	cmp	eax,2
	je	timestamp_label
	cmp	eax,3
	je	org_origin_label
	mov	edx,invalid_value
	jmp	error_undefined
      label_out_of_scope:
	mov	edx,symbol_out_of_scope
	jmp	error_undefined
      label_undefined:
	mov	edx,undefined_symbol
      error_undefined:
	cmp	[current_pass],1
	ja	undefined_value
      force_next_pass:
	or	[next_pass_needed],-1
      undefined_value:
	or	[value_undefined],-1
	and	word [edi+12],0
	xor	eax,eax
	stos	dword [edi]
	stos	dword [edi]
	add	edi,0Ch
	cmp	[error_line],0
	jne	calculation_loop
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],edx
	mov	[error_info],ebx
	jmp	calculation_loop
      calculate_add:
	xor	ah,ah
	mov	ah,[ebx+12]
	mov	al,[edi+12]
	or	al,al
	jz	add_values
	or	ah,ah
	jz	add_relocatable
	add	ah,al
	jnz	invalid_add
	mov	ecx,[edi+16]
	cmp	ecx,[ebx+16]
	je	add_values
      invalid_add:
	call	recoverable_misuse
	jmp	add_values
      add_relocatable:
	mov	ah,al
	mov	ecx,[edi+16]
	mov	[ebx+16],ecx
      add_values:
	mov	[ebx+12],ah
	mov	eax,[edi]
	add	[ebx],eax
	mov	eax,[edi+4]
	adc	[ebx+4],eax
	mov	al,[edi+13]
	adc	[ebx+13],al
	jp	add_sign_ok
	call	recoverable_overflow
      add_sign_ok:
	or	dx,dx
	jz	calculation_loop
	push	esi
	mov	esi,ebx
	mov	cl,[edi+10]
	mov	al,[edi+8]
	call	add_register
	mov	cl,[edi+11]
	mov	al,[edi+9]
	call	add_register
	pop	esi
	jmp	calculation_loop
      add_register:
	or	al,al
	jz	add_register_done
      add_register_start:
	cmp	[esi+8],al
	jne	add_in_second_slot
	add	[esi+10],cl
	jo	value_out_of_range
	jnz	add_register_done
	mov	byte [esi+8],0
	ret
      add_in_second_slot:
	cmp	[esi+9],al
	jne	create_in_first_slot
	add	[esi+11],cl
	jo	value_out_of_range
	jnz	add_register_done
	mov	byte [esi+9],0
	ret
      create_in_first_slot:
	cmp	byte [esi+8],0
	jne	create_in_second_slot
	mov	[esi+8],al
	mov	[esi+10],cl
	ret
      create_in_second_slot:
	cmp	byte [esi+9],0
	jne	invalid_expression
	mov	[esi+9],al
	mov	[esi+11],cl
      add_register_done:
	ret
      out_of_range:
	jmp	calculation_loop
      calculate_sub:
	xor	ah,ah
	mov	ah,[ebx+12]
	mov	al,[edi+12]
	or	al,al
	jz	sub_values
	or	ah,ah
	jz	negate_relocatable
	cmp	al,ah
	jne	invalid_sub
	xor	ah,ah
	mov	ecx,[edi+16]
	cmp	ecx,[ebx+16]
	je	sub_values
      invalid_sub:
	call	recoverable_misuse
	jmp	sub_values
      negate_relocatable:
	neg	al
	mov	ah,al
	mov	ecx,[edi+16]
	mov	[ebx+16],ecx
      sub_values:
	mov	[ebx+12],ah
	mov	eax,[edi]
	sub	[ebx],eax
	mov	eax,[edi+4]
	sbb	[ebx+4],eax
	mov	al,[edi+13]
	sbb	[ebx+13],al
	jp	sub_sign_ok
	cmp	[error_line],0
	jne	sub_sign_ok
	call	recoverable_overflow
      sub_sign_ok:
	or	dx,dx
	jz	calculation_loop
	push	esi
	mov	esi,ebx
	mov	cl,[edi+10]
	mov	al,[edi+8]
	call	sub_register
	mov	cl,[edi+11]
	mov	al,[edi+9]
	call	sub_register
	pop	esi
	jmp	calculation_loop
      sub_register:
	or	al,al
	jz	add_register_done
	neg	cl
	jo	value_out_of_range
	jmp	add_register_start
      calculate_mul:
	or	dx,dx
	jz	mul_start
	cmp	word [ebx+8],0
	jne	mul_start
	xor	ecx,ecx
      swap_values:
	mov	eax,[ebx+ecx]
	xchg	eax,[edi+ecx]
	mov	[ebx+ecx],eax
	add	ecx,4
	cmp	ecx,16
	jb	swap_values
      mul_start:
	push	esi edx
	mov	esi,ebx
	xor	bl,bl
	cmp	byte [esi+13],0
	je	mul_first_sign_ok
	xor	bl,-1
	mov	eax,[esi]
	mov	edx,[esi+4]
	not	eax
	not	edx
	add	eax,1
	adc	edx,0
	mov	[esi],eax
	mov	[esi+4],edx
	or	eax,edx
	jz	mul_overflow
      mul_first_sign_ok:
	cmp	byte [edi+13],0
	je	mul_second_sign_ok
	xor	bl,-1
	cmp	byte [esi+8],0
	je	mul_first_register_sign_ok
	neg	byte [esi+10]
	jo	invalid_expression
      mul_first_register_sign_ok:
	cmp	byte [esi+9],0
	je	mul_second_register_sign_ok
	neg	byte [esi+11]
	jo	invalid_expression
      mul_second_register_sign_ok:
	mov	eax,[edi]
	mov	edx,[edi+4]
	not	eax
	not	edx
	add	eax,1
	adc	edx,0
	mov	[edi],eax
	mov	[edi+4],edx
	or	eax,edx
	jz	mul_overflow
      mul_second_sign_ok:
	cmp	dword [esi+4],0
	jz	mul_numbers
	cmp	dword [edi+4],0
	jz	mul_numbers
	jnz	mul_overflow
      mul_numbers:
	mov	eax,[esi+4]
	mul	dword [edi]
	or	edx,edx
	jnz	mul_overflow
	mov	ecx,eax
	mov	eax,[esi]
	mul	dword [edi+4]
	or	edx,edx
	jnz	mul_overflow
	add	ecx,eax
	jc	mul_overflow
	mov	eax,[esi]
	mul	dword [edi]
	add	edx,ecx
	jc	mul_overflow
	mov	[esi],eax
	mov	[esi+4],edx
	or	bl,bl
	jz	mul_ok
	not	eax
	not	edx
	add	eax,1
	adc	edx,0
	mov	[esi],eax
	mov	[esi+4],edx
	or	eax,edx
	jnz	mul_ok
	not	bl
      mul_ok:
	mov	[esi+13],bl
	pop	edx
	or	dx,dx
	jz	mul_calculated
	cmp	word [edi+8],0
	jne	invalid_value
	cmp	byte [esi+8],0
	je	mul_first_register_ok
	call	get_byte_scale
	imul	byte [esi+10]
	mov	dl,ah
	cbw
	cmp	ah,dl
	jne	value_out_of_range
	mov	[esi+10],al
	or	al,al
	jnz	mul_first_register_ok
	mov	[esi+8],al
      mul_first_register_ok:
	cmp	byte [esi+9],0
	je	mul_calculated
	call	get_byte_scale
	imul	byte [esi+11]
	mov	dl,ah
	cbw
	cmp	ah,dl
	jne	value_out_of_range
	mov	[esi+11],al
	or	al,al
	jnz	mul_calculated
	mov	[esi+9],al
      mul_calculated:
	pop	esi
	jmp	calculation_loop
      mul_overflow:
	pop	edx esi
	call	recoverable_overflow
	jmp	calculation_loop
      get_byte_scale:
	mov	al,[edi]
	cbw
	cwde
	cdq
	cmp	edx,[edi+4]
	jne	value_out_of_range
	cmp	eax,[edi]
	jne	value_out_of_range
	ret
      calculate_div:
	push	esi edx
	mov	esi,ebx
	call	div_64
	pop	edx
	or	dx,dx
	jz	div_calculated
	cmp	byte [esi+8],0
	je	div_first_register_ok
	call	get_byte_scale
	or	al,al
	jz	value_out_of_range
	mov	al,[esi+10]
	cbw
	idiv	byte [edi]
	or	ah,ah
	jnz	invalid_use_of_symbol
	mov	[esi+10],al
      div_first_register_ok:
	cmp	byte [esi+9],0
	je	div_calculated
	call	get_byte_scale
	or	al,al
	jz	value_out_of_range
	mov	al,[esi+11]
	cbw
	idiv	byte [edi]
	or	ah,ah
	jnz	invalid_use_of_symbol
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
	mov	[esi+13],bh
	pop	esi
	jmp	calculation_loop
      calculate_and:
	mov	eax,[edi]
	mov	edx,[edi+4]
	mov	cl,[edi+13]
	and	[ebx],eax
	and	[ebx+4],edx
	and	[ebx+13],cl
	jmp	calculation_loop
      calculate_or:
	mov	eax,[edi]
	mov	edx,[edi+4]
	mov	cl,[edi+13]
	or	[ebx],eax
	or	[ebx+4],edx
	or	[ebx+13],cl
	jmp	calculation_loop
      calculate_xor:
	mov	eax,[edi]
	mov	edx,[edi+4]
	mov	cl,[edi+13]
	xor	[ebx],eax
	xor	[ebx+4],edx
	xor	[ebx+13],cl
	jmp	calculation_loop
      shr_negative:
	mov	byte [edi+13],0
	not	dword [edi]
	not	dword [edi+4]
	add	dword [edi],1
	adc	dword [edi+4],0
	jc	shl_over
      calculate_shl:
	cmp	byte [edi+13],0
	jne	shl_negative
	mov	edx,[ebx+4]
	mov	eax,[ebx]
	cmp	dword [edi+4],0
	jne	shl_over
	movsx	ecx,byte [ebx+13]
	xchg	ecx,[edi]
	cmp	ecx,64
	je	shl_max
	ja	shl_over
	cmp	ecx,32
	jae	shl_high
	shld	[edi],edx,cl
	shld	edx,eax,cl
	shl	eax,cl
	mov	[ebx],eax
	mov	[ebx+4],edx
	jmp	shl_done
      shl_over:
	cmp	byte [ebx+13],0
	jne	shl_overflow
      shl_max:
	movsx	ecx,byte [ebx+13]
	cmp	eax,ecx
	jne	shl_overflow
	cmp	edx,ecx
	jne	shl_overflow
	xor	eax,eax
	mov	[ebx],eax
	mov	[ebx+4],eax
	jmp	calculation_loop
      shl_high:
	sub	cl,32
	shld	[edi],edx,cl
	shld	edx,eax,cl
	shl	eax,cl
	mov	[ebx+4],eax
	and	dword [ebx],0
	cmp	edx,[edi]
	jne	shl_overflow
      shl_done:
	movsx	eax,byte [ebx+13]
	cmp	eax,[edi]
	je	calculation_loop
      shl_overflow:
	call	recoverable_overflow
	jmp	calculation_loop
      shl_negative:
	mov	byte [edi+13],0
	not	dword [edi]
	not	dword [edi+4]
	add	dword [edi],1
	adc	dword [edi+4],0
	jnc	calculate_shr
	dec	dword [edi+4]
      calculate_shr:
	cmp	byte [edi+13],0
	jne	shr_negative
	mov	edx,[ebx+4]
	mov	eax,[ebx]
	cmp	dword [edi+4],0
	jne	shr_over
	mov	ecx,[edi]
	cmp	ecx,64
	jae	shr_over
	push	esi
	movsx	esi,byte [ebx+13]
	cmp	ecx,32
	jae	shr_high
	shrd	eax,edx,cl
	shrd	edx,esi,cl
	mov	[ebx],eax
	mov	[ebx+4],edx
	pop	esi
	jmp	calculation_loop
      shr_high:
	sub	cl,32
	shrd	edx,esi,cl
	mov	[ebx],edx
	mov	[ebx+4],esi
	pop	esi
	jmp	calculation_loop
      shr_over:
	movsx	eax,byte [ebx+13]
	mov	dword [ebx],eax
	mov	dword [ebx+4],eax
	jmp	calculation_loop
      calculate_not:
	cmp	word [edi+8],0
	jne	invalid_expression
	cmp	byte [edi+12],0
	je	not_ok
	call	recoverable_misuse
      not_ok:
	not	dword [edi]
	not	dword [edi+4]
	not	byte [edi+13]
	add	edi,14h
	jmp	calculation_loop
      calculate_bsf:
	cmp	word [edi+8],0
	jne	invalid_expression
	cmp	byte [edi+12],0
	je	bsf_ok
	call	recoverable_misuse
      bsf_ok:
	xor	ecx,ecx
	bsf	eax,[edi]
	jnz	finish_bs
	mov	ecx,32
	bsf	eax,[edi+4]
	jnz	finish_bs
	cmp	byte [edi+13],0
	jne	finish_bs
      bs_overflow:
	call	recoverable_overflow
	add	edi,14h
	jmp	calculation_loop
      calculate_bsr:
	cmp	word [edi+8],0
	jne	invalid_expression
	cmp	byte [edi+12],0
	je	bsr_ok
	call	recoverable_misuse
      bsr_ok:
	cmp	byte [edi+13],0
	jne	bs_overflow
	mov	ecx,32
	bsr	eax,[edi+4]
	jnz	finish_bs
	xor	ecx,ecx
	bsr	eax,[edi]
	jz	bs_overflow
      finish_bs:
	add	eax,ecx
	xor	edx,edx
	mov	[edi],eax
	mov	[edi+4],edx
	mov	[edi+13],dl
	add	edi,14h
	jmp	calculation_loop
      calculate_neg:
	cmp	byte [edi+8],0
	je	neg_first_register_ok
	neg	byte [edi+10]
	jo	invalid_expression
      neg_first_register_ok:
	cmp	byte [edi+9],0
	je	neg_second_register_ok
	neg	byte [edi+11]
	jo	invalid_expression
      neg_second_register_ok:
	neg	byte [edi+12]
	xor	eax,eax
	xor	edx,edx
	xor	cl,cl
	xchg	eax,[edi]
	xchg	edx,[edi+4]
	xchg	cl,[edi+13]
	sub	[edi],eax
	sbb	[edi+4],edx
	sbb	[edi+13],cl
	jp	neg_sign_ok
	call	recoverable_overflow
      neg_sign_ok:
	add	edi,14h
	jmp	calculation_loop
      calculate_rva:
	cmp	word [edi+8],0
	jne	invalid_expression
	mov	al,[output_format]
	cmp	al,5
	je	calculate_gotoff
	cmp	al,4
	je	calculate_coff_rva
	cmp	al,3
	jne	invalid_expression
	test	[format_flags],8
	jnz	pe64_rva
	mov	al,2
	bt	[resolver_flags],0
	jc	rva_type_ok
	xor	al,al
      rva_type_ok:
	cmp	byte [edi+12],al
	je	rva_ok
	call	recoverable_misuse
      rva_ok:
	mov	byte [edi+12],0
	mov	eax,[code_start]
	mov	eax,[eax+34h]
	xor	edx,edx
      finish_rva:
	sub	[edi],eax
	sbb	[edi+4],edx
	sbb	byte [edi+13],0
	jp	rva_finished
	call	recoverable_overflow
      rva_finished:
	add	edi,14h
	jmp	calculation_loop
      pe64_rva:
	mov	al,4
	bt	[resolver_flags],0
	jc	pe64_rva_type_ok
	xor	al,al
      pe64_rva_type_ok:
	cmp	byte [edi+12],al
	je	pe64_rva_ok
	call	recoverable_misuse
      pe64_rva_ok:
	mov	byte [edi+12],0
	mov	eax,[code_start]
	mov	edx,[eax+34h]
	mov	eax,[eax+30h]
	jmp	finish_rva
      calculate_gotoff:
	test	[format_flags],1
	jnz	calculate_elf_dyn_rva
	test	[format_flags],8
	jnz	invalid_expression
      calculate_coff_rva:
	mov	dl,5
	cmp	byte [edi+12],2
	je	change_value_type
      incorrect_change_of_value_type:
	call	recoverable_misuse
      change_value_type:
	mov	byte [edi+12],dl
	add	edi,14h
	jmp	calculation_loop
      calculate_elf_dyn_rva:
	xor	dl,dl
	test	byte [edi+12],1
	jnz	incorrect_change_of_value_type
	jmp	change_value_type
      calculate_plt:
	cmp	word [edi+8],0
	jne	invalid_expression
	cmp	[output_format],5
	jne	invalid_expression
	test	[format_flags],1
	jnz	invalid_expression
	mov	dl,6
	mov	dh,2
	test	[format_flags],8
	jz	check_value_for_plt
	mov	dh,4
      check_value_for_plt:
	mov	eax,[edi]
	or	eax,[edi+4]
	jnz	incorrect_change_of_value_type
	cmp	byte [edi+12],dh
	jne	incorrect_change_of_value_type
	mov	eax,[edi+16]
	cmp	byte [eax],80h
	jne	incorrect_change_of_value_type
	jmp	change_value_type
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
	cmp	byte [esi+13],0
	je	div_first_sign_ok
	mov	eax,[esi]
	mov	edx,[esi+4]
	not	eax
	not	edx
	add	eax,1
	adc	edx,0
	mov	[esi],eax
	mov	[esi+4],edx
	or	eax,edx
	jz	value_out_of_range
	xor	bx,-1
      div_first_sign_ok:
	cmp	byte [edi+13],0
	je	div_second_sign_ok
	mov	eax,[edi]
	mov	edx,[edi+4]
	not	eax
	not	edx
	add	eax,1
	adc	edx,0
	mov	[edi],eax
	mov	[edi+4],edx
	or	eax,edx
	jz	value_out_of_range
	xor	bl,-1
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
	push	ebx
	mov	eax,[esi+4]
	xor	edx,edx
	div	dword [edi+4]
	mov	ebx,[esi]
	mov	[esi],eax
	and	dword [esi+4],0
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
	pop	ebx
	jmp	div_done
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
	mov	ecx,eax
	or	ecx,edx
	jnz	remainder_ok
	not	bh
      remainder_ok:
	or	bl,bl
	jz	div_ok
	not	dword [esi]
	not	dword [esi+4]
	add	dword [esi],1
	adc	dword [esi+4],0
	mov	ecx,[esi]
	or	ecx,[esi+4]
	jnz	div_ok
	not	bl
      div_ok:
	mov	[esi+13],bl
	ret
      store_label_reference:
	cmp	[symbols_file],0
	je	label_reference_ok
	cmp	[next_pass_needed],0
	jne	label_reference_ok
	mov	eax,[tagged_blocks]
	mov	dword [eax-4],2
	mov	dword [eax-8],4
	sub	eax,8+4
	cmp	eax,edi
	jbe	out_of_memory
	mov	[tagged_blocks],eax
	mov	[eax],ebx
      label_reference_ok:
	ret
      convert_fp:
	inc	esi
	and	word [edi+8],0
	and	word [edi+12],0
	mov	al,[value_size]
	cmp	al,2
	je	convert_fp_word
	cmp	al,4
	je	convert_fp_dword
	test	al,not 8
	jz	convert_fp_qword
	call	recoverable_misuse
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
	cmp	bx,7FFh
	jge	value_out_of_range
	cmp	bx,0
	jg	fp_qword_exp_ok
	or	edx,1 shl 20
	mov	cx,bx
	neg	cx
	inc	cx
	cmp	cx,52+1
	ja	value_out_of_range
	cmp	cx,32
	jb	fp_qword_small_shift
	sub	cx,32
	mov	eax,edx
	xor	edx,edx
	shr	eax,cl
	jmp	fp_qword_shift_done
      fp_qword_small_shift:
	mov	ebx,edx
	shr	edx,cl
	shrd	eax,ebx,cl
      fp_qword_shift_done:
	mov	bx,0
	jnc	fp_qword_exp_ok
	add	eax,1
	adc	edx,0
	test	edx,1 shl 20
	jz	fp_qword_exp_ok
	and	edx,1 shl 20 - 1
	inc	bx
      fp_qword_exp_ok:
	shl	ebx,20
	or	edx,ebx
	jnz	fp_qword_store
	or	eax,eax
	jz	value_out_of_range
      fp_qword_store:
	mov	bl,[esi+11]
	shl	ebx,31
	or	edx,ebx
	mov	[edi],eax
	mov	[edi+4],edx
	add	esi,13
	ret
      convert_fp_word:
	xor	eax,eax
	cmp	word [esi+8],8000h
	je	fp_word_store
	mov	bx,[esi+8]
	mov	ax,[esi+6]
	shl	ax,1
	shr	ax,6
	jnc	fp_word_ok
	inc	ax
	bt	ax,10
	jnc	fp_word_ok
	and	ax,1 shl 10 - 1
	inc	bx
	shr	ax,1
      fp_word_ok:
	add	bx,0Fh
	cmp	bx,01Fh
	jge	value_out_of_range
	cmp	bx,0
	jg	fp_word_exp_ok
	or	ax,1 shl 10
	mov	cx,bx
	neg	cx
	inc	cx
	cmp	cx,10+1
	ja	value_out_of_range
	xor	bx,bx
	shr	ax,cl
	jnc	fp_word_exp_ok
	inc	ax
	test	ax,1 shl 10
	jz	fp_word_exp_ok
	and	ax,1 shl 10 - 1
	inc	bx
      fp_word_exp_ok:
	shl	bx,10
	or	ax,bx
	jz	value_out_of_range
      fp_word_store:
	mov	bl,[esi+11]
	shl	bx,15
	or	ax,bx
	mov	[edi],eax
	xor	eax,eax
	mov	[edi+4],eax
	add	esi,13
	ret
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
	cmp	bx,0FFh
	jge	value_out_of_range
	cmp	bx,0
	jg	fp_dword_exp_ok
	or	eax,1 shl 23
	mov	cx,bx
	neg	cx
	inc	cx
	cmp	cx,23+1
	ja	value_out_of_range
	xor	bx,bx
	shr	eax,cl
	jnc	fp_dword_exp_ok
	inc	eax
	test	eax,1 shl 23
	jz	fp_dword_exp_ok
	and	eax,1 shl 23 - 1
	inc	bx
      fp_dword_exp_ok:
	shl	ebx,23
	or	eax,ebx
	jz	value_out_of_range
      fp_dword_store:
	mov	bl,[esi+11]
	shl	ebx,31
	or	eax,ebx
	mov	[edi],eax
	xor	eax,eax
	mov	[edi+4],eax
	add	esi,13
	ret
      get_string_value:
	inc	esi
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
	and	word [edi+8],0
	and	word [edi+12],0
	ret

get_byte_value:
	mov	[value_size],1
	or	[operand_flags],1
	call	calculate_value
	or	al,al
	jz	check_byte_value
	call	recoverable_misuse
      check_byte_value:
	mov	eax,[edi]
	mov	edx,[edi+4]
	cmp	byte [edi+13],0
	je	byte_positive
	cmp	edx,-1
	jne	range_exceeded
	cmp	eax,-100h
	jb	range_exceeded
	ret
      byte_positive:
	test	edx,edx
	jnz	range_exceeded
	cmp	eax,100h
	jae	range_exceeded
      return_byte_value:
	ret
      range_exceeded:
	xor	eax,eax
	xor	edx,edx
      recoverable_overflow:
	cmp	[error_line],0
	jne	ignore_overflow
	push	[current_line]
	pop	[error_line]
	mov	[error],value_out_of_range
	or	[value_undefined],-1
      ignore_overflow:
	ret
      recoverable_misuse:
	cmp	[error_line],0
	jne	ignore_misuse
	push	[current_line]
	pop	[error_line]
	mov	[error],invalid_use_of_symbol
      ignore_misuse:
	ret
get_word_value:
	mov	[value_size],2
	or	[operand_flags],1
	call	calculate_value
	cmp	al,2
	jb	check_word_value
	call	recoverable_misuse
      check_word_value:
	mov	eax,[edi]
	mov	edx,[edi+4]
	cmp	byte [edi+13],0
	je	word_positive
	cmp	edx,-1
	jne	range_exceeded
	cmp	eax,-10000h
	jb	range_exceeded
	ret
      word_positive:
	test	edx,edx
	jnz	range_exceeded
	cmp	eax,10000h
	jae	range_exceeded
	ret
get_dword_value:
	mov	[value_size],4
	or	[operand_flags],1
	call	calculate_value
	cmp	al,4
	jne	check_dword_value
	mov	[value_type],2
	mov	eax,[edi]
	cdq
	cmp	edx,[edi+4]
	jne	range_exceeded
	mov	ecx,edx
	shr	ecx,31
	cmp	cl,[value_sign]
	jne	range_exceeded
	ret
      check_dword_value:
	mov	eax,[edi]
	mov	edx,[edi+4]
	cmp	byte [edi+13],0
	je	dword_positive
	cmp	edx,-1
	jne	range_exceeded
	ret
      dword_positive:
	test	edx,edx
	jne	range_exceeded
	ret
get_pword_value:
	mov	[value_size],6
	or	[operand_flags],1
	call	calculate_value
	cmp	al,4
	jne	check_pword_value
	call	recoverable_misuse
      check_pword_value:
	mov	eax,[edi]
	mov	edx,[edi+4]
	cmp	byte [edi+13],0
	je	pword_positive
	cmp	edx,-10000h
	jb	range_exceeded
	ret
      pword_positive:
	cmp	edx,10000h
	jae	range_exceeded
	ret
get_qword_value:
	mov	[value_size],8
	or	[operand_flags],1
	call	calculate_value
      check_qword_value:
	mov	eax,[edi]
	mov	edx,[edi+4]
	ret
get_count_value:
	mov	[value_size],8
	or	[operand_flags],1
	call	calculate_expression
	cmp	word [edi+8],0
	jne	invalid_value
	mov	[value_sign],0
	mov	al,[edi+12]
	or	al,al
	jz	check_count_value
	call	recoverable_misuse
      check_count_value:
	cmp	byte [edi+13],0
	jne	invalid_count_value
	mov	eax,[edi]
	mov	edx,[edi+4]
	or	edx,edx
	jnz	invalid_count_value
	ret
      invalid_count_value:
	cmp	[error_line],0
	jne	zero_count
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_value
      zero_count:
	xor	eax,eax
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
	mov	[value_size],al
	call	calculate_value
	mov	eax,[edi]
	mov	edx,[edi+4]
	ret
      calculate_value:
	call	calculate_expression
	cmp	word [edi+8],0
	jne	invalid_value
	mov	eax,[edi+16]
	mov	[symbol_identifier],eax
	mov	al,[edi+13]
	mov	[value_sign],al
	mov	al,[edi+12]
	mov	[value_type],al
	ret
      value_qword:
	call	get_qword_value
      truncated_value:
	mov	[value_sign],0
	ret
      value_pword:
	call	get_pword_value
	movzx	edx,dx
	jmp	truncated_value
      value_dword:
	call	get_dword_value
	xor	edx,edx
	jmp	truncated_value
      value_word:
	call	get_word_value
	xor	edx,edx
	movzx	eax,ax
	jmp	truncated_value
      value_byte:
	call	get_byte_value
	xor	edx,edx
	movzx	eax,al
	jmp	truncated_value
get_address_word_value:
	mov	[address_size],2
	mov	[value_size],2
	mov	[free_address_range],0
	jmp	calculate_address
get_address_dword_value:
	mov	[address_size],4
	mov	[value_size],4
	mov	[free_address_range],0
	jmp	calculate_address
get_address_qword_value:
	mov	[address_size],8
	mov	[value_size],8
	mov	[free_address_range],0
	jmp	calculate_address
get_address_value:
	mov	[address_size],0
	mov	[value_size],8
	or	[free_address_range],-1
      calculate_address:
	cmp	byte [esi],'.'
	je	invalid_address
	call	calculate_expression
	mov	eax,[edi+16]
	mov	[address_symbol],eax
	mov	al,[edi+13]
	mov	[address_sign],al
	mov	al,[edi+12]
	mov	[value_type],al
	cmp	al,0
	je	address_size_ok
	jg	get_address_symbol_size
	neg	al
      get_address_symbol_size:
	cmp	al,6
	je	special_address_type_32bit
	cmp	al,5
	je	special_address_type_32bit
	ja	invalid_address_type
	test	al,1
	jnz	invalid_address_type
	shl	al,5
	jmp	address_symbol_ok
      invalid_address_type:
	call	recoverable_misuse
      special_address_type_32bit:
	mov	al,40h
      address_symbol_ok:
	mov	ah,[address_size]
	or	[address_size],al
	shr	al,4
	or	ah,ah
	jz	address_size_ok
	cmp	al,ah
	je	address_size_ok
	cmp	ax,0408h
	je	address_sizes_mixed
	cmp	ax,0804h
	jne	address_sizes_do_not_agree
      address_sizes_mixed:
	cmp	[value_type],4
	jne	address_sizes_mixed_type_ok
	mov	[value_type],2
      address_sizes_mixed_type_ok:
	mov	eax,[edi]
	cdq
	cmp	edx,[edi+4]
	je	address_size_ok
	cmp	[error_line],0
	jne	address_size_ok
	call	recoverable_overflow
      address_size_ok:
	xor	ebx,ebx
	xor	ecx,ecx
	mov	cl,[value_type]
	shl	ecx,16
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
	jne	check_vsib
      check_address_registers:
	or	al,ah
	cmp	al,0Ch
	jae	check_vsib
	cmp	al,6
	je	check_vsib
	cmp	al,7
	je	check_vsib
	mov	ah,[address_size]
	and	ah,0Fh
	jz	address_registers_sizes_ok
	cmp	al,ah
	jne	invalid_address
      address_registers_sizes_ok:
	cmp	al,4
	je	sib_allowed
	cmp	al,8
	je	sib_allowed
	cmp	al,9
	je	check_ip_relative_address
	cmp	cl,1
	ja	invalid_address
	cmp	[free_address_range],0
	jne	check_qword_value
	jmp	check_word_value
      address_sizes_do_not_match:
	cmp	al,0Fh
	jne	invalid_address
	mov	al,bh
	and	al,0Fh
	cmp	al,ah
	jne	invalid_address
      check_ip_relative_address:
	or	bl,bl
	jnz	invalid_address
	cmp	bh,98h
	je	check_rip_relative_address
	cmp	bh,94h
	jne	invalid_address
	cmp	[free_address_range],0
	je	check_dword_value
	mov	eax,[edi]
	mov	edx,[edi+4]
	ret
      check_rip_relative_address:
	mov	eax,[edi]
	cdq
	cmp	edx,[edi+4]
	jne	range_exceeded
	cmp	dl,[edi+13]
	jne	range_exceeded
	ret
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
	jnz	check_index_with_base
	cmp	cl,3
	je	special_index_scale
	cmp	cl,5
	je	special_index_scale
	cmp	cl,9
	je	special_index_scale
	cmp	cl,2
	jne	check_index_scale
	cmp	bl,45h
	jne	special_index_scale
	cmp	[code_type],64
	je	special_index_scale
	cmp	[segment_register],4
	jne	special_index_scale
	cmp	[value_type],0
	jne	check_index_scale
	mov	al,[edi]
	cbw
	cwde
	cmp	eax,[edi]
	jne	check_index_scale
	cdq
	cmp	edx,[edi+4]
	jne	check_immediate_address
      special_index_scale:
	mov	bh,bl
	dec	cl
      check_immediate_address:
	cmp	[free_address_range],0
	jne	check_qword_value
	mov	al,[address_size]
	and	al,0Fh
	cmp	al,2
	je	check_word_value
	cmp	al,4
	je	check_dword_value
	cmp	al,8
	je	check_qword_value
	or	al,al
	jnz	invalid_value
	cmp	[code_type],64
	jne	check_dword_value
	jmp	check_qword_value
      check_index_with_base:
	cmp	cl,1
	jne	check_index_scale
	cmp	bl,44h
	je	swap_base_with_index
	cmp	bl,84h
	je	swap_base_with_index
	cmp	[code_type],64
	je	check_for_rbp_base
	cmp	bl,45h
	jne	check_for_ebp_base
	cmp	[segment_register],3
	je	swap_base_with_index
	jmp	check_immediate_address
      check_for_ebp_base:
	cmp	bh,45h
	jne	check_immediate_address
	cmp	[segment_register],4
	jne	check_immediate_address
      swap_base_with_index:
	xchg	bl,bh
	jmp	check_immediate_address
      check_for_rbp_base:
	cmp	bh,45h
	je	swap_base_with_index
	cmp	bh,85h
	je	swap_base_with_index
	jmp	check_immediate_address
      check_index_scale:
	test	cl,not 1111b
	jnz	invalid_address
	mov	al,cl
	dec	al
	and	al,cl
	jz	check_immediate_address
	jmp	invalid_address
      check_vsib:
	xor	ah,ah
      check_vsib_base:
	test	bh,bh
	jz	check_vsib_index
	mov	al,bh
	shr	al,4
	cmp	al,4
	je	check_vsib_base_size
	cmp	[code_type],64
	jne	swap_vsib_registers
	cmp	al,8
	jne	swap_vsib_registers
      check_vsib_base_size:
	mov	ah,[address_size]
	and	ah,0Fh
	jz	check_vsib_index
	cmp	al,ah
	jne	invalid_address
      check_vsib_index:
	mov	al,bl
	and	al,0E0h
	cmp	al,0C0h
	jae	check_index_scale
	cmp	al,60h
	je	check_index_scale
	jmp	invalid_address
      swap_vsib_registers:
	xor	ah,-1
	jz	invalid_address
	cmp	cl,1
	ja	invalid_address
	xchg	bl,bh
	mov	cl,1
	jmp	check_vsib_base

calculate_relative_offset:
	cmp	[value_undefined],0
	jne	relative_offset_ok
	test	bh,bh
	setne	ch
	cmp	bx,[ds:ebp+10h]
	je	origin_registers_ok
	xchg	bh,bl
	xchg	ch,cl
	cmp	bx,[ds:ebp+10h]
	jne	invalid_value
      origin_registers_ok:
	cmp	cx,[ds:ebp+10h+2]
	jne	invalid_value
	mov	bl,[address_sign]
	add	eax,[ds:ebp]
	adc	edx,[ds:ebp+4]
	adc	bl,[ds:ebp+8]
	sub	eax,edi
	sbb	edx,0
	sbb	bl,0
	mov	[value_sign],bl
	mov	bl,[value_type]
	mov	ecx,[address_symbol]
	mov	[symbol_identifier],ecx
	test	bl,1
	jnz	relative_offset_unallowed
	cmp	bl,6
	je	plt_relative_offset
	mov	bh,[ds:ebp+9]
	cmp	bl,bh
	je	set_relative_offset_type
	cmp	bx,0402h
	je	set_relative_offset_type
      relative_offset_unallowed:
	call	recoverable_misuse
      set_relative_offset_type:
	cmp	[value_type],0
	je	relative_offset_ok
	mov	[value_type],0
	cmp	ecx,[ds:ebp+14h]
	je	relative_offset_ok
	mov	[value_type],3
      relative_offset_ok:
	ret
      plt_relative_offset:
	mov	[value_type],7
	cmp	byte [ds:ebp+9],2
	je	relative_offset_ok
	cmp	byte [ds:ebp+9],4
	jne	recoverable_misuse
	ret

calculate_logical_expression:
	xor	al,al
  calculate_embedded_logical_expression:
	mov	[logical_value_wrapping],al
	call	get_logical_value
      logical_loop:
	cmp	byte [esi],'|'
	je	logical_or
	cmp	byte [esi],'&'
	je	logical_and
	ret
      logical_or:
	inc	esi
	or	al,al
	jnz	logical_value_already_determined
	push	eax
	call	get_logical_value
	pop	ebx
	or	al,bl
	jmp	logical_loop
      logical_and:
	inc	esi
	or	al,al
	jz	logical_value_already_determined
	push	eax
	call	get_logical_value
	pop	ebx
	and	al,bl
	jmp	logical_loop
      logical_value_already_determined:
	push	eax
	call	skip_logical_value
	jc	invalid_expression
	pop	eax
	jmp	logical_loop
  get_value_for_comparison:
	mov	[value_size],8
	or	[operand_flags],1
	lods	byte [esi]
	call	calculate_expression
	cmp	byte [edi+8],0
	jne	first_register_size_ok
	mov	byte [edi+10],0
      first_register_size_ok:
	cmp	byte [edi+9],0
	jne	second_register_size_ok
	mov	byte [edi+11],0
      second_register_size_ok:
	mov	eax,[edi+16]
	mov	[symbol_identifier],eax
	mov	al,[edi+13]
	mov	[value_sign],al
	mov	bl,[edi+12]
	mov	eax,[edi]
	mov	edx,[edi+4]
	mov	ecx,[edi+8]
	ret
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
	cmp	al,91h
	je	logical_expression
	cmp	al,0FFh
	je	invalid_expression
	cmp	al,88h
	je	check_for_defined
	cmp	al,8Ah
	je	check_for_earlier_defined
	cmp	al,89h
	je	check_for_used
	cmp	al,'0'
	je	given_false
	cmp	al,'1'
	je	given_true
	cmp	al,'('
	jne	invalid_value
	call	get_value_for_comparison
	mov	bh,[value_sign]
	push	eax edx [symbol_identifier] ebx ecx
	mov	al,[esi]
	or	al,al
	jz	logical_number
	cmp	al,0Fh
	je	logical_number
	cmp	al,92h
	je	logical_number
	cmp	al,'&'
	je	logical_number
	cmp	al,'|'
	je	logical_number
	inc	esi
	mov	[compare_type],al
	cmp	byte [esi],'('
	jne	invalid_value
	call	get_value_for_comparison
	cmp	bl,[esp+4]
	jne	values_not_relative
	or	bl,bl
	jz	check_values_registers
	mov	ebx,[symbol_identifier]
	cmp	ebx,[esp+8]
	jne	values_not_relative
      check_values_registers:
	cmp	ecx,[esp]
	je	values_relative
	ror	ecx,16
	xchg	ch,cl
	ror	ecx,16
	xchg	ch,cl
	cmp	ecx,[esp]
	je	values_relative
      values_not_relative:
	cmp	[compare_type],0F8h
	jne	invalid_comparison
	add	esp,12+8
	jmp	return_false
      invalid_comparison:
	call	recoverable_misuse
      values_relative:
	pop	ebx
	shl	ebx,16
	mov	bx,[esp]
	add	esp,8
	pop	ecx ebp
	cmp	[compare_type],'='
	je	check_equal
	cmp	[compare_type],0F1h
	je	check_not_equal
	cmp	[compare_type],0F8h
	je	return_true
	test	ebx,0FFFF0000h
	jz	check_less_or_greater
	call	recoverable_misuse
      check_less_or_greater:
	cmp	[compare_type],'>'
	je	check_greater
	cmp	[compare_type],'<'
	je	check_less
	cmp	[compare_type],0F2h
	je	check_not_less
	cmp	[compare_type],0F3h
	je	check_not_greater
	jmp	invalid_expression
      check_equal:
	cmp	bh,[value_sign]
	jne	return_false
	cmp	eax,ebp
	jne	return_false
	cmp	edx,ecx
	jne	return_false
	jmp	return_true
      check_greater:
	cmp	bh,[value_sign]
	jg	return_true
	jl	return_false
	cmp	edx,ecx
	jb	return_true
	ja	return_false
	cmp	eax,ebp
	jb	return_true
	jae	return_false
      check_less:
	cmp	bh,[value_sign]
	jg	return_false
	jl	return_true
	cmp	edx,ecx
	jb	return_false
	ja	return_true
	cmp	eax,ebp
	jbe	return_false
	ja	return_true
      check_not_less:
	cmp	bh,[value_sign]
	jg	return_true
	jl	return_false
	cmp	edx,ecx
	jb	return_true
	ja	return_false
	cmp	eax,ebp
	jbe	return_true
	ja	return_false
      check_not_greater:
	cmp	bh,[value_sign]
	jg	return_false
	jl	return_true
	cmp	edx,ecx
	jb	return_false
	ja	return_true
	cmp	eax,ebp
	jb	return_false
	jae	return_true
      check_not_equal:
	cmp	bh,[value_sign]
	jne	return_true
	cmp	eax,ebp
	jne	return_true
	cmp	edx,ecx
	jne	return_true
	jmp	return_false
      logical_number:
	pop	ecx ebx eax edx eax
	or	bl,bl
	jnz	invalid_logical_number
	or	cx,cx
	jz	logical_number_ok
      invalid_logical_number:
	call	recoverable_misuse
      logical_number_ok:
	test	bh,bh
	jnz	return_true
	or	eax,edx
	jnz	return_true
	jmp	return_false
      check_for_earlier_defined:
	or	bh,-1
	jmp	check_if_expression_defined
      check_for_defined:
	xor	bh,bh
      check_if_expression_defined:
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
	cmp	al,'!'
	je	invalid_expression
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
	add	esi,12+1
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
	test	bh,bh
	jnz	no_prediction
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
	jmp	return_false
      given_false:
	inc	esi
      return_false:
	xor	al,al
	jmp	logical_value_ok
      given_true:
	inc	esi
      return_true:
	or	al,-1
	jmp	logical_value_ok
      logical_expression:
	lods	byte [esi]
	mov	dl,[logical_value_wrapping]
	push	edx
	call	calculate_embedded_logical_expression
	pop	edx
	mov	[logical_value_wrapping],dl
	push	eax
	lods	byte [esi]
	cmp	al,92h
	jne	invalid_expression
	pop	eax
      logical_value_ok:
	pop	ebx
	xor	al,bl
	ret

skip_symbol:
	lods	byte [esi]
	or	al,al
	jz	nothing_to_skip
	cmp	al,0Fh
	je	nothing_to_skip
	cmp	al,1
	je	skip_instruction
	cmp	al,2
	je	skip_label
	cmp	al,3
	je	skip_label
	cmp	al,4
	je	skip_special_label
	cmp	al,20h
	jb	skip_assembler_symbol
	cmp	al,'('
	je	skip_expression
	cmp	al,'['
	je	skip_address
      skip_done:
	clc
	ret
      skip_label:
	add	esi,2
      skip_instruction:
	add	esi,2
      skip_assembler_symbol:
	inc	esi
	jmp	skip_done
      skip_special_label:
	add	esi,4
	jmp	skip_done
      skip_address:
	mov	al,[esi]
	and	al,11110000b
	cmp	al,60h
	jb	skip_expression
	cmp	al,70h
	ja	skip_expression
	inc	esi
	jmp	skip_address
      skip_expression:
	lods	byte [esi]
	or	al,al
	jz	skip_string
	cmp	al,'.'
	je	skip_fp_value
	cmp	al,')'
	je	skip_done
	cmp	al,']'
	je	skip_done
	cmp	al,'!'
	je	skip_expression
	cmp	al,0Fh
	je	skip_expression
	cmp	al,10h
	je	skip_register
	cmp	al,11h
	je	skip_label_value
	cmp	al,80h
	jae	skip_expression
	movzx	eax,al
	add	esi,eax
	jmp	skip_expression
      skip_label_value:
	add	esi,3
      skip_register:
	inc	esi
	jmp	skip_expression
      skip_fp_value:
	add	esi,12
	jmp	skip_done
      skip_string:
	lods	dword [esi]
	add	esi,eax
	inc	esi
	jmp	skip_done
      nothing_to_skip:
	dec	esi
	stc
	ret

expand_path:
	lods	byte [esi]
	cmp	al,'%'
	je	environment_variable
	stos	byte [edi]
	or	al,al
	jnz	expand_path
	cmp	edi,[memory_end]
	ja	out_of_memory
	ret
      environment_variable:
	mov	ebx,esi
      find_variable_end:
	lods	byte [esi]
	or	al,al
	jz	not_environment_variable
	cmp	al,'%'
	jne	find_variable_end
	mov	byte [esi-1],0
	push	esi
	mov	esi,ebx
	call	get_environment_variable
	pop	esi
	mov	byte [esi-1],'%'
	jmp	expand_path
      not_environment_variable:
	mov	al,'%'
	stos	byte [edi]
	mov	esi,ebx
	jmp	expand_path
get_include_directory:
	lods	byte [esi]
	cmp	al,';'
	je	include_directory_ok
	stos	byte [edi]
	or	al,al
	jnz	get_include_directory
	dec	esi
	dec	edi
      include_directory_ok:
	cmp	byte [edi-1],'/'
	je	path_separator_ok
	cmp	byte [edi-1],'\'
	je	path_separator_ok
	mov	al,'/'
	stos	byte [edi]
      path_separator_ok:
	ret
