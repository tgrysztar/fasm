
; flat assembler core
; Copyright (c) 1999-2003, Tomasz Grysztar.
; All rights reserved.

assembler:
	mov	ecx,[memory_end]
	mov	edi,ecx
	sub	ecx,[labels_list]
	imul	ecx,3
	sub	edi,ecx
	cmp	edi,[code_start]
	jbe	out_of_memory
	mov	[labels_list],edi
	shr	ecx,2
	xor	eax,eax
	rep	stos dword [edi]
	mov	[stub_size],eax
	mov	[number_of_sections],eax
	mov	[current_pass],ax
	mov	[times_working],al
      assembler_loop:
	mov	eax,[labels_list]
	mov	[display_buffer],eax
	mov	eax,[additional_memory]
	mov	[free_additional_memory],eax
	mov	eax,[additional_memory_end]
	mov	[structures_buffer],eax
	mov	esi,[source_start]
	mov	edi,[code_start]
	xor	eax,eax
	mov	[org_origin],edi
	mov	[org_start],edi
	mov	[org_registers],eax
	mov	[org_symbol],eax
	mov	[error_line],eax
	mov	[counter],eax
	mov	[format_flags],eax
	mov	[number_of_relocations],eax
	mov	[undefined_data_end],eax
	mov	[next_pass_needed],al
	mov	[output_format],al
	mov	[reloc_labels],al
	mov	[virtual_data],al
	mov	[code_type],16
      pass_loop:
	call	assemble_line
	jnc	pass_loop
	mov	eax,[structures_buffer]
	cmp	eax,[additional_memory_end]
	jne	unexpected_end_of_file
	call	close_pass
	mov	eax,[labels_list]
      check_symbols:
	cmp	eax,[memory_end]
	jae	symbols_checked
	test	byte [eax+8],8
	jz	symbol_defined_ok
	mov	cx,[current_pass]
	cmp	cx,[eax+18]
	jne	symbol_defined_ok
	test	byte [eax+8],1
	jz	symbol_defined_ok
	sub	cx,[eax+16]
	cmp	cx,1
	jne	symbol_defined_ok
	and	byte [eax+8],not 1
	or	[next_pass_needed],-1
      symbol_defined_ok:
	test	byte [eax+8],10h
	jz	use_prediction_ok
	mov	cx,[current_pass]
	and	byte [eax+8],not 10h
	test	byte [eax+8],20h
	jnz	check_use_prediction
	cmp	cx,[eax+18]
	jne	use_prediction_ok
	test	byte [eax+8],8
	jz	use_prediction_ok
	jmp	use_misprediction
      check_use_prediction:
	test	byte [eax+8],8
	jz	use_misprediction
	cmp	cx,[eax+18]
	je	use_prediction_ok
      use_misprediction:
	or	[next_pass_needed],-1
      use_prediction_ok:
	test	byte [eax+8],40h
	jz	check_next_symbol
	mov	cx,[current_pass]
	and	byte [eax+8],not 40h
	test	byte [eax+8],80h
	jnz	check_define_prediction
	cmp	cx,[eax+16]
	jne	check_next_symbol
	test	byte [eax+8],1
	jz	check_next_symbol
	jmp	define_misprediction
      check_define_prediction:
	test	byte [eax+8],1
	jz	define_misprediction
	cmp	cx,[eax+16]
	je	check_next_symbol
      define_misprediction:
	or	[next_pass_needed],-1
      check_next_symbol:
	add	eax,24
	jmp	check_symbols
      symbols_checked:
	cmp	[next_pass_needed],0
	jne	next_pass
	mov	eax,[error_line]
	or	eax,eax
	jz	assemble_ok
	mov	[current_line],eax
	jmp	near [error]
      next_pass:
	inc	[current_pass]
	cmp	[current_pass],0
	je	code_cannot_be_generated
	jmp	assembler_loop
      assemble_ok:
	ret

assemble_line:
	mov	eax,[display_buffer]
	sub	eax,100h
	cmp	edi,eax
	ja	out_of_memory
	lods	byte [esi]
	cmp	al,1
	je	assemble_instruction
	jb	source_end
	cmp	al,3
	jb	define_label
	je	define_constant
	cmp	al,0Fh
	je	new_line
	cmp	al,13h
	je	code_type_setting
	cmp	al,10h
	jne	illegal_instruction
	lods	byte [esi]
	jmp	segment_prefix
      code_type_setting:
	lods	byte [esi]
	mov	[code_type],al
	jmp	line_assembled
      new_line:
	lods	dword [esi]
	mov	[current_line],eax
      continue_line:
	cmp	byte [esi],0Fh
	je	line_assembled
	jmp	assemble_line
      define_label:
	lods	dword [esi]
	cmp	eax,0Fh
	jb	invalid_use_of_symbol
	je	reserved_word_used_as_symbol
	mov	ebx,eax
	lods	byte [esi]
	mov	cl,al
	mov	eax,edi
	sub	eax,[org_origin]
	cdq
	xor	ch,ch
	cmp	[reloc_labels],0
	je	make_label
	mov	ch,2
      make_label:
	cmp	eax,[ebx]
	mov	[ebx],eax
	setne	ah
	cmp	edx,[ebx+4]
	mov	[ebx+4],edx
	setne	al
	or	ah,al
	cmp	cl,[ebx+10]
	mov	[ebx+10],cl
	setne	al
	or	ah,al
	cmp	ch,[ebx+11]
	mov	[ebx+11],ch
	setne	al
	or	ah,al
	mov	edx,[org_registers]
	cmp	edx,[ebx+12]
	mov	[ebx+12],edx
	setne	al
	or	ah,al
	or	ch,ch
	jz	label_symbol_ok
	mov	edx,[org_symbol]
	cmp	edx,[ebx+20]
	mov	[ebx+20],edx
	setne	al
	or	ah,al
      label_symbol_ok:
	mov	cx,[current_pass]
	xchg	[ebx+16],cx
	test	byte [ebx+8],1
	jz	new_label
	cmp	cx,[ebx+16]
	je	symbol_already_defined
	inc	cx
	sub	cx,[ebx+16]
	setnz	al
	or	ah,al
	jz	continue_line
	test	byte [ebx+8],8
	jz	continue_line
	mov	cx,[current_pass]
	cmp	cx,[ebx+18]
	jne	continue_line
	or	[next_pass_needed],-1
	jmp	continue_line
      new_label:
	or	byte [ebx+8],1
	jmp	continue_line
      define_constant:
	lods	dword [esi]
	inc	esi
	or	eax,eax
	jz	org_directive
	cmp	eax,0Fh
	jb	invalid_use_of_symbol
	je	reserved_word_used_as_symbol
	mov	edx,[eax+8]
	push	edx
	cmp	[current_pass],0
	je	get_constant_value
	test	dl,4
	jnz	get_constant_value
	mov	cx,[current_pass]
	cmp	cx,[eax+16]
	je	get_constant_value
	and	dl,not 1
	mov	[eax+8],dl
      get_constant_value:
	push	eax
	mov	al,byte [esi-1]
	push	eax
	call	get_value
	pop	ebx
	mov	ch,bl
	pop	ebx
	pop	dword [ebx+8]
	or	ebx,ebx
	jz	set_org
	cmp	ebx,0Fh
	jb	invalid_use_of_symbol
	je	reserved_word_used_as_symbol
	xor	cl,cl
	mov	ch,[value_type]
	cmp	ch,3
	je	invalid_use_of_symbol
      make_constant:
	cmp	eax,[ebx]
	mov	[ebx],eax
	setne	ah
	cmp	edx,[ebx+4]
	mov	[ebx+4],edx
	setne	al
	or	ah,al
	cmp	cl,[ebx+10]
	mov	[ebx+10],cl
	setne	al
	or	ah,al
	cmp	ch,[ebx+11]
	mov	[ebx+11],ch
	setne	al
	or	ah,al
	xor	edx,edx
	cmp	edx,[ebx+12]
	mov	[ebx+12],edx
	setne	al
	or	ah,al
	or	ch,ch
	jz	constant_symbol_ok
	mov	edx,[symbol_identifier]
	cmp	edx,[ebx+20]
	mov	[ebx+20],edx
	setne	al
	or	ah,al
      constant_symbol_ok:
	mov	cx,[current_pass]
	xchg	[ebx+16],cx
	test	byte [ebx+8],1
	jz	new_constant
	cmp	cx,[ebx+16]
	jne	redeclare_constant
	test	byte [ebx+8],2
	jz	symbol_already_defined
	or	byte [ebx+8],4
	jmp	continue_line
      redeclare_constant:
	inc	cx
	sub	cx,[ebx+16]
	setnz	al
	or	ah,al
	jz	continue_line
	test	byte [ebx+8],4
	jnz	continue_line
	test	byte [ebx+8],8
	jz	continue_line
	mov	cx,[current_pass]
	cmp	cx,[ebx+18]
	jne	continue_line
	or	[next_pass_needed],-1
	jmp	continue_line
      new_constant:
	or	byte [ebx+8],1+2
	jmp	continue_line
      assemble_instruction:
	mov	[operand_size],0
	mov	[forced_size],0
	movzx	ebx,word [esi]
	mov	al,[esi+2]
	add	ebx,assembler
	add	esi,3
	jmp	near ebx
      instruction_assembled:
	mov	al,[esi]
	cmp	al,0Fh
	je	line_assembled
	or	al,al
	jnz	extra_characters_on_line
      line_assembled:
	clc
	ret
      source_end:
	dec	esi
	stc
	ret
skip_line:
	call	skip_symbol
	jnc	skip_line
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

org_directive:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
      set_org:
	mov	[reloc_labels],0
	mov	dl,[value_type]
	or	dl,dl
	jz	org_ok
	cmp	dl,2
	jne	invalid_use_of_symbol
	mov	[reloc_labels],1
      org_ok:
	mov	ecx,edi
	sub	ecx,eax
	mov	[org_origin],ecx
	mov	[org_registers],0
	mov	[org_start],edi
	mov	eax,[symbol_identifier]
	mov	[org_symbol],eax
	jmp	instruction_assembled
label_directive:
	lods	byte [esi]
	cmp	al,2
	jne	invalid_argument
	lods	dword [esi]
	cmp	eax,0Fh
	jb	invalid_use_of_symbol
	je	reserved_word_used_as_symbol
	inc	esi
	mov	ebx,eax
	xor	cl,cl
	lods	byte [esi]
	cmp	al,':'
	je	get_label_size
	dec	esi
	cmp	al,11h
	jne	label_size_ok
      get_label_size:
	lods	word [esi]
	cmp	al,11h
	jne	invalid_argument
	mov	cl,ah
      label_size_ok:
	mov	eax,edi
	sub	eax,[org_origin]
	cdq
	mov	ebp,[org_registers]
	cmp	byte [esi],80h
	jne	define_free_label
	inc	esi
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	push	dword [ebx+8]
	push	ebx ecx
	and	byte [ebx+8],not 1
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_address_value
	or	bh,bh
	setnz	ch
	xchg	ch,cl
	mov	bp,cx
	shl	ebp,16
	xchg	bl,bh
	mov	bp,bx
	pop	ecx ebx
	pop	dword [ebx+8]
	cdq
	mov	ch,[value_type]
	or	ch,ch
	jz	make_free_label
	cmp	ch,2
	jne	invalid_use_of_symbol
	jmp	make_free_label
      define_free_label:
	xor	ch,ch
	cmp	[reloc_labels],0
	je	make_free_label
	mov	ch,2
	push	[org_symbol]
	pop	[symbol_identifier]
      make_free_label:
	cmp	eax,[ebx]
	mov	[ebx],eax
	setne	ah
	cmp	edx,[ebx+4]
	mov	[ebx+4],edx
	setne	al
	or	ah,al
	cmp	cl,[ebx+10]
	mov	[ebx+10],cl
	setne	al
	or	ah,al
	cmp	ch,[ebx+11]
	mov	[ebx+11],ch
	setne	al
	or	ah,al
	cmp	ebp,[ebx+12]
	mov	[ebx+12],ebp
	setne	al
	or	ah,al
	or	ch,ch
	jz	free_label_symbol_ok
	mov	edx,[symbol_identifier]
	cmp	edx,[ebx+20]
	mov	[ebx+20],edx
	setne	al
	or	ah,al
      free_label_symbol_ok:
	mov	cx,[current_pass]
	xchg	[ebx+16],cx
	test	byte [ebx+8],1
	jz	new_free_label
	cmp	cx,[ebx+16]
	je	symbol_already_defined
	inc	cx
	sub	cx,[ebx+16]
	setnz	al
	or	ah,al
	jz	instruction_assembled
	test	byte [ebx+8],8
	jz	instruction_assembled
	mov	cx,[current_pass]
	cmp	cx,[ebx+18]
	jne	instruction_assembled
	or	[next_pass_needed],-1
	jmp	instruction_assembled
      new_free_label:
	or	byte [ebx+8],1
	jmp	instruction_assembled
load_directive:
	lods	byte [esi]
	cmp	al,2
	jne	invalid_argument
	lods	dword [esi]
	cmp	eax,0Fh
	jb	invalid_use_of_symbol
	je	reserved_word_used_as_symbol
	inc	esi
	push	eax
	mov	al,1
	cmp	byte [esi],11h
	jne	load_size_ok
	lods	byte [esi]
	lods	byte [esi]
      load_size_ok:
	cmp	al,8
	ja	invalid_value
	mov	[operand_size],al
	mov	dword [value],0
	mov	dword [value+4],0
	lods	word [esi]
	cmp	ax,82h+'(' shl 8
	jne	invalid_argument
      load_from_code:
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_relative_offset
	neg	eax
	cmp	[next_pass_needed],0
	jne	load_address_ok
	cmp	[value_type],0
	jne	invalid_use_of_symbol
      load_address_ok:
	push	esi edi
	mov	esi,edi
	sub	esi,eax
	jc	bad_load_address
	cmp	esi,[org_start]
	jb	bad_load_address
	mov	edi,value
	movzx	ecx,[operand_size]
	cmp	ecx,eax
	ja	bad_load_address
	rep	movs byte [edi],[esi]
	jmp	value_loaded
      bad_load_address:
	cmp	[next_pass_needed],0
	je	value_out_of_range
      value_loaded:
	pop	edi esi
	mov	eax,dword [value]
	mov	edx,dword [value+4]
	pop	ebx
	xor	cx,cx
	jmp	make_constant
store_directive:
	cmp	byte [esi],11h
	je	sized_store
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	call	get_byte_value
	xor	edx,edx
	movzx	eax,al
	mov	[operand_size],1
	jmp	store_value_ok
      sized_store:
	call	get_value
      store_value_ok:
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	dword [value],eax
	mov	dword [value+4],edx
	lods	word [esi]
	cmp	ax,80h+'(' shl 8
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_relative_offset
	neg	eax
	cmp	[next_pass_needed],0
	jne	store_address_ok
	cmp	[value_type],0
	jne	invalid_use_of_symbol
      store_address_ok:
	push	esi edi
	sub	edi,eax
	jc	bad_store_address
	cmp	edi,[org_start]
	jb	bad_store_address
	mov	esi,value
	movzx	ecx,[operand_size]
	cmp	ecx,eax
	ja	bad_store_address
	rep	movs byte [edi],[esi]
	mov	eax,edi
	pop	edi esi
	cmp	edi,[undefined_data_end]
	jne	instruction_assembled
	cmp	eax,[undefined_data_start]
	jbe	instruction_assembled
	mov	[undefined_data_start],eax
	jmp	instruction_assembled
      bad_store_address:
	cmp	[next_pass_needed],0
	je	value_out_of_range
	pop	edi esi
	jmp	instruction_assembled

display_directive:
	push	esi edi
      prepare_display:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],0
	jne	display_byte
	inc	esi
	lods	dword [esi]
	mov	ecx,eax
	rep	movs byte [edi],[esi]
	inc	esi
	jmp	display_next
      display_byte:
	call	get_byte_value
	stos	byte [edi]
      display_next:
	cmp	edi,[display_buffer]
	ja	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	do_display
	cmp	al,0Fh
	je	do_display
	cmp	al,','
	jne	extra_characters_on_line
	jmp	prepare_display
      do_display:
	dec	esi
	mov	ebp,edi
	pop	edi ebx
	push	esi edi
	mov	esi,edi
	mov	ecx,ebp
	sub	ecx,esi
	mov	edi,[display_buffer]
	sub	edi,ecx
	sub	edi,4
	cmp	edi,esi
	jbe	out_of_memory
	mov	[display_buffer],edi
	mov	eax,ecx
	rep	movs byte [edi],[esi]
	stos	dword [edi]
	pop	edi esi
	jmp	instruction_assembled
flush_display_buffer:
	mov	eax,[display_buffer]
	or	eax,eax
	jz	display_done
	mov	esi,[labels_list]
	cmp	esi,eax
	je	display_done
      display_messages:
	sub	esi,4
	mov	ecx,[esi]
	sub	esi,ecx
	push	esi
	call	display_block
	pop	esi
	cmp	esi,[display_buffer]
	jne	display_messages
	mov	eax,[labels_list]
	mov	[display_buffer],eax
      display_done:
	ret
times_directive:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[next_pass_needed],0
	jne	times_value_ok
	cmp	[value_type],0
	jne	invalid_use_of_symbol
      times_value_ok:
	cmp	eax,0
	je	zero_times
	jl	negative_times
	cmp	byte [esi],':'
	jne	times_argument_ok
	inc	esi
      times_argument_ok:
	push	[counter] [counter_limit]
	mov	[counter_limit],eax
	mov	[counter],1
      times_loop:
	push	esi
	or	[times_working],-1
	call	continue_line
	mov	eax,[counter_limit]
	cmp	[counter],eax
	je	times_done
	inc	[counter]
	pop	esi
	jmp	times_loop
      times_done:
	mov	[times_working],0
	pop	eax
	pop	[counter_limit] [counter]
	jmp	instruction_assembled
      negative_times:
	cmp	[error_line],0
	jne	zero_times
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_value
      zero_times:
	call	skip_line
	jmp	instruction_assembled

virtual_directive:
	lods	byte [esi]
	cmp	al,80h
	jne	virtual_at_current
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_address_value
	mov	ebp,[symbol_identifier]
	xor	ch,ch
	or	bh,bh
	jz	set_virtual
	mov	ch,1
	jmp	set_virtual
      virtual_at_current:
	dec	esi
	mov	eax,edi
	sub	eax,[org_origin]
	xor	bx,bx
	xor	cx,cx
	mov	[value_type],0
	cmp	[reloc_labels],0
	je	set_virtual
	mov	[value_type],2
	mov	ebp,[org_symbol]
      set_virtual:
	mov	edx,[org_registers]
	mov	byte [org_registers],bh
	mov	byte [org_registers+1],bl
	mov	byte [org_registers+2],ch
	mov	byte [org_registers+3],cl
	call	allocate_structure_data
	mov	word [ebx],virtual_directive-assembler
	neg	eax
	add	eax,edi
	xchg	[org_origin],eax
	mov	[ebx+4],eax
	mov	[ebx+8],edx
	mov	al,[virtual_data]
	mov	[ebx+2],al
	mov	al,[reloc_labels]
	mov	[ebx+3],al
	mov	eax,edi
	xchg	eax,[org_start]
	mov	[ebx+0Ch],eax
	xchg	ebp,[org_symbol]
	mov	[ebx+10h],ebp
	mov	[ebx+14h],edi
	or	[virtual_data],-1
	mov	[reloc_labels],0
	cmp	[value_type],0
	je	instruction_assembled
	cmp	[value_type],2
	jne	invalid_use_of_symbol
	or	[reloc_labels],-1
	jmp	instruction_assembled
      allocate_structure_data:
	mov	ebx,[structures_buffer]
	sub	ebx,18h
	cmp	ebx,[free_additional_memory]
	jb	out_of_memory
	mov	[structures_buffer],ebx
	ret
      find_structure_data:
	mov	ebx,[structures_buffer]
      scan_structures:
	cmp	ebx,[additional_memory_end]
	je	no_such_structure
	cmp	ax,[ebx]
	jne	next_structure
	clc
	ret
      next_structure:
	cmp	ax,repeat_directive-assembler
	jne	if_structure_ok
	cmp	word [ebx],if_directive-assembler
	je	no_such_structure
      if_structure_ok:
	cmp	ax,if_directive-assembler
	jne	repeat_structure_ok
	cmp	word [ebx],repeat_directive-assembler
	je	no_such_structure
      repeat_structure_ok:
	add	ebx,18h
	jmp	scan_structures
      no_such_structure:
	stc
	ret
      end_virtual:
	call	find_structure_data
	jc	unexpected_instruction
	mov	al,[ebx+2]
	mov	[virtual_data],al
	mov	al,[ebx+3]
	mov	[reloc_labels],al
	mov	eax,[ebx+4]
	mov	[org_origin],eax
	mov	eax,[ebx+8]
	mov	[org_registers],eax
	mov	eax,[ebx+0Ch]
	mov	[org_start],eax
	mov	eax,[ebx+10h]
	mov	[org_symbol],eax
	mov	edi,[ebx+14h]
      remove_structure_data:
	push	esi edi
	mov	esi,[structures_buffer]
	mov	ecx,ebx
	sub	ecx,esi
	lea	edi,[esi+18h]
	mov	[structures_buffer],edi
	shr	ecx,2
	rep	movs dword [edi],[esi]
	pop	edi esi
	jmp	instruction_assembled
repeat_directive:
	cmp	[times_working],0
	jne	unexpected_instruction
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[next_pass_needed],0
	jne	repeat_value_ok
	cmp	[value_type],0
	jne	invalid_use_of_symbol
      repeat_value_ok:
	cmp	eax,0
	je	zero_repeat
	jl	negative_repeat
	call	allocate_structure_data
	mov	word [ebx],repeat_directive-assembler
	xchg	eax,[counter_limit]
	mov	[ebx+4],eax
	mov	eax,1
	xchg	eax,[counter]
	mov	[ebx+8],eax
	mov	[ebx+0Ch],esi
	jmp	instruction_assembled
      end_repeat:
	cmp	[times_working],0
	jne	unexpected_instruction
	call	find_structure_data
	jc	unexpected_instruction
	mov	eax,[counter_limit]
	inc	[counter]
	cmp	[counter],eax
	jbe	continue_repeating
	mov	eax,[ebx+4]
	mov	[counter_limit],eax
	mov	eax,[ebx+8]
	mov	[counter],eax
	jmp	remove_structure_data
      continue_repeating:
	mov	esi,[ebx+0Ch]
	jmp	instruction_assembled
      negative_repeat:
	cmp	[error_line],0
	jne	zero_repeat
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_value
      zero_repeat:
	mov	al,[esi]
	or	al,al
	jz	unexpected_end_of_file
	cmp	al,0Fh
	jne	extra_characters_on_line
	call	find_end_repeat
	jmp	instruction_assembled
      find_end_repeat:
	call	find_structure_end
	cmp	ax,repeat_directive-assembler
	jne	unexpected_instruction
	ret
      find_structure_end:
	call	skip_line
	lods	byte [esi]
	cmp	al,0Fh
	jne	unexpected_end_of_file
	lods	dword [esi]
	mov	[current_line],eax
      skip_labels:
	cmp	byte [esi],2
	jne	labels_ok
	add	esi,6
	jmp	skip_labels
      labels_ok:
	cmp	byte [esi],1
	jne	find_structure_end
	mov	ax,[esi+1]
	cmp	ax,prefix_instruction-assembler
	je	find_structure_end
	add	esi,4
	cmp	ax,repeat_directive-assembler
	je	skip_repeat
	cmp	ax,if_directive-assembler
	je	skip_if
	cmp	ax,else_directive-assembler
	je	structure_end
	cmp	ax,end_directive-assembler
	jne	find_structure_end
	cmp	byte [esi],1
	jne	find_structure_end
	mov	ax,[esi+1]
	add	esi,4
	cmp	ax,repeat_directive-assembler
	je	structure_end
	cmp	ax,if_directive-assembler
	jne	find_structure_end
      structure_end:
	ret
      skip_repeat:
	call	find_end_repeat
	jmp	find_structure_end
if_directive:
	cmp	[times_working],0
	jne	unexpected_instruction
	call	calculate_logical_expression
	mov	dl,al
	mov	al,[esi]
	or	al,al
	jz	unexpected_end_of_file
	cmp	al,0Fh
	jne	extra_characters_on_line
	or	dl,dl
	jnz	if_true
	call	find_else
	jc	instruction_assembled
	mov	al,[esi]
	cmp	al,1
	jne	else_true
	cmp	word [esi+1],if_directive-assembler
	jne	else_true
	add	esi,4
	jmp	if_directive
      if_true:
	call	allocate_structure_data
	mov	word [ebx],if_directive-assembler
	mov	byte [ebx+2],0
	jmp	instruction_assembled
      else_true:
	or	al,al
	jz	unexpected_end_of_file
	cmp	al,0Fh
	jne	extra_characters_on_line
	call	allocate_structure_data
	mov	word [ebx],if_directive-assembler
	or	byte [ebx+2],-1
	jmp	instruction_assembled
      else_directive:
	cmp	[times_working],0
	jne	unexpected_instruction
	mov	ax,if_directive-assembler
	call	find_structure_data
	jc	unexpected_instruction
	cmp	byte [ebx+2],0
	jne	unexpected_instruction
      found_else:
	mov	al,[esi]
	cmp	al,1
	jne	skip_else
	cmp	word [esi+1],if_directive-assembler
	jne	skip_else
	add	esi,4
	call	find_else
	jnc	found_else
	jmp	remove_structure_data
      skip_else:
	or	al,al
	jz	unexpected_end_of_file
	cmp	al,0Fh
	jne	extra_characters_on_line
	call	find_end_if
	jmp	remove_structure_data
      end_if:
	cmp	[times_working],0
	jne	unexpected_instruction
	call	find_structure_data
	jc	unexpected_instruction
	jmp	remove_structure_data
      skip_if:
	call	find_else
	jc	find_structure_end
	cmp	byte [esi],1
	jne	skip_after_else
	cmp	word [esi+1],if_directive-assembler
	jne	skip_after_else
	add	esi,4
	jmp	skip_if
      skip_after_else:
	call	find_end_if
	jmp	find_structure_end
      find_else:
	call	find_structure_end
	cmp	ax,else_directive-assembler
	je	else_found
	cmp	ax,if_directive-assembler
	jne	unexpected_instruction
	stc
	ret
      else_found:
	clc
	ret
      find_end_if:
	call	find_structure_end
	cmp	ax,if_directive-assembler
	jne	unexpected_instruction
	ret
end_directive:
	lods	byte [esi]
	cmp	al,1
	jne	invalid_argument
	lods	word [esi]
	inc	esi
	cmp	ax,virtual_directive-assembler
	je	end_virtual
	cmp	ax,repeat_directive-assembler
	je	end_repeat
	cmp	ax,if_directive-assembler
	je	end_if
	cmp	ax,data_directive-assembler
	je	end_data
	jmp	invalid_argument

data_bytes:
	lods	byte [esi]
	cmp	al,'('
	je	get_byte
	cmp	al,'?'
	jne	invalid_argument
	mov	eax,edi
	mov	byte [edi],0
	inc	edi
	call	undefined_data
	jmp	byte_ok
      get_byte:
	cmp	byte [esi],0
	je	get_string
	call	get_byte_value
	stos	byte [edi]
      byte_ok:
	cmp	edi,[display_buffer]
	ja	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	data_end
	cmp	al,0Fh
	je	data_end
	cmp	al,','
	jne	extra_characters_on_line
	jmp	data_bytes
      data_end:
	dec	esi
	jmp	instruction_assembled
      get_string:
	inc	esi
	lods	dword [esi]
	mov	ecx,eax
	lea	eax,[edi+ecx]
	cmp	eax,[display_buffer]
	ja	out_of_memory
	rep	movs byte [edi],[esi]
	inc	esi
	jmp	byte_ok
      undefined_data:
	cmp	[virtual_data],0
	je	mark_undefined_data
	ret
      mark_undefined_data:
	cmp	eax,[undefined_data_end]
	je	undefined_data_ok
	mov	[undefined_data_start],eax
      undefined_data_ok:
	mov	[undefined_data_end],edi
	ret
data_unicode:
	or	[base_code],-1
	jmp	get_words_data
data_words:
	mov	[base_code],0
      get_words_data:
	lods	byte [esi]
	cmp	al,'('
	je	get_word
	cmp	al,'?'
	jne	invalid_argument
	mov	eax,edi
	mov	word [edi],0
	scas	word [edi]
	call	undefined_data
	jmp	word_ok
      get_word:
	cmp	[base_code],0
	je	word_data_value
	cmp	byte [esi],0
	je	word_string
      word_data_value:
	call	get_word_value
	call	mark_relocation
	stos	word [edi]
      word_ok:
	cmp	edi,[display_buffer]
	ja	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	data_end
	cmp	al,0Fh
	je	data_end
	cmp	al,','
	jne	extra_characters_on_line
	jmp	get_words_data
      word_string:
	inc	esi
	lods	dword [esi]
	mov	ecx,eax
	jecxz	word_string_ok
	lea	eax,[edi+ecx*2]
	cmp	eax,[display_buffer]
	ja	out_of_memory
	xor	ah,ah
      copy_word_string:
	lods	byte [esi]
	stos	word [edi]
	loop	copy_word_string
      word_string_ok:
	inc	esi
	jmp	word_ok
data_dwords:
	lods	byte [esi]
	cmp	al,'('
	je	get_dword
	cmp	al,'?'
	jne	invalid_argument
	mov	eax,edi
	mov	dword [edi],0
	scas	dword [edi]
	call	undefined_data
	jmp	dword_ok
      get_dword:
	push	esi
	call	get_dword_value
	pop	ebx
	cmp	byte [esi],':'
	je	complex_dword
	call	mark_relocation
	stos	dword [edi]
	jmp	dword_ok
      complex_dword:
	mov	esi,ebx
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_word_value
	push	eax
	inc	esi
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	mov	al,[value_type]
	push	eax
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_word_value
	call	mark_relocation
	stos	word [edi]
	pop	eax
	mov	[value_type],al
	pop	eax
	call	mark_relocation
	stos	word [edi]
      dword_ok:
	cmp	edi,[display_buffer]
	ja	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	data_end
	cmp	al,0Fh
	je	data_end
	cmp	al,','
	jne	extra_characters_on_line
	jmp	data_dwords
data_pwords:
	lods	byte [esi]
	cmp	al,'('
	je	get_pword
	cmp	al,'?'
	jne	invalid_argument
	mov	eax,edi
	mov	dword [edi],0
	scas	dword [edi]
	mov	word [edi],0
	scas	word [edi]
	call	undefined_data
	jmp	pword_ok
      get_pword:
	push	esi
	call	get_pword_value
	pop	ebx
	cmp	byte [esi],':'
	je	complex_pword
	call	mark_relocation
	stos	dword [edi]
	mov	ax,dx
	stos	word [edi]
	jmp	pword_ok
      complex_pword:
	mov	esi,ebx
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_word_value
	push	eax
	inc	esi
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	mov	al,[value_type]
	push	eax
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	call	mark_relocation
	stos	dword [edi]
	pop	eax
	mov	[value_type],al
	pop	eax
	call	mark_relocation
	stos	word [edi]
      pword_ok:
	cmp	edi,[display_buffer]
	ja	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	data_end
	cmp	al,0Fh
	je	data_end
	cmp	al,','
	jne	extra_characters_on_line
	jmp	data_pwords
data_qwords:
	lods	byte [esi]
	cmp	al,'('
	je	get_qword
	cmp	al,'?'
	jne	invalid_argument
	mov	eax,edi
	mov	dword [edi],0
	scas	dword [edi]
	mov	dword [edi],0
	scas	dword [edi]
	call	undefined_data
	jmp	qword_ok
      get_qword:
	call	get_qword_value
	call	mark_relocation
	stos	dword [edi]
	mov	eax,edx
	stos	dword [edi]
      qword_ok:
	cmp	edi,[display_buffer]
	ja	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	data_end
	cmp	al,0Fh
	je	data_end
	cmp	al,','
	jne	extra_characters_on_line
	jmp	data_qwords
data_twords:
	lods	byte [esi]
	cmp	al,'('
	je	get_tbyte
	cmp	al,'?'
	jne	invalid_argument
	mov	eax,edi
	mov	dword [edi],0
	scas	dword [edi]
	mov	dword [edi],0
	scas	dword [edi]
	mov	word [edi],0
	scas	word [edi]
	call	undefined_data
	jmp	tbyte_ok
      get_tbyte:
	lods	byte [esi]
	cmp	al,'.'
	jne	invalid_value
	cmp	word [esi+8],8000h
	je	fp_zero_tbyte
	mov	eax,[esi]
	stos	dword [edi]
	mov	eax,[esi+4]
	stos	dword [edi]
	mov	ax,[esi+8]
	add	ax,3FFFh
	cmp	ax,8000h
	jae	value_out_of_range
	mov	bl,[esi+11]
	shl	bx,15
	or	ax,bx
	stos	word [edi]
	add	esi,12
	jmp	tbyte_ok
      fp_zero_tbyte:
	xor	eax,eax
	stos	dword [edi]
	stos	dword [edi]
	mov	al,[esi+11]
	shl	ax,15
	stos	word [edi]
	add	esi,12
      tbyte_ok:
	cmp	edi,[display_buffer]
	ja	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	data_end
	cmp	al,0Fh
	je	data_end
	cmp	al,','
	jne	extra_characters_on_line
	jmp	data_twords
data_file:
	lods	word [esi]
	cmp	ax,'('
	jne	invalid_argument
	add	esi,4
	call	open_binary_file
	mov	eax,[esi-4]
	lea	esi,[esi+eax+1]
	mov	al,2
	xor	edx,edx
	call	lseek
	push	eax
	xor	edx,edx
	cmp	byte [esi],':'
	jne	position_ok
	inc	esi
	cmp	byte [esi],'('
	jne	invalid_argument
	inc	esi
	cmp	byte [esi],'.'
	je	invalid_value
	push	ebx
	call	get_dword_value
	pop	ebx
	mov	edx,eax
	sub	[esp],edx
      position_ok:
	cmp	byte [esi],','
	jne	size_ok
	inc	esi
	cmp	byte [esi],'('
	jne	invalid_argument
	inc	esi
	cmp	byte [esi],'.'
	je	invalid_value
	push	ebx edx
	call	get_dword_value
	pop	edx ebx
	mov	[esp],eax
      size_ok:
	xor	al,al
	call	lseek
	pop	ecx
	mov	edx,edi
	add	edi,ecx
	jc	out_of_memory
	cmp	edi,[display_buffer]
	ja	out_of_memory
	call	read
	jc	error_reading_file
	call	close
	lods	byte [esi]
	cmp	al,','
	je	data_file
	dec	esi
	jmp	instruction_assembled
      open_binary_file:
	push	esi
	push	edi
	mov	esi,[current_line]
	mov	esi,[esi]
      get_current_path:
	lodsb
	stosb
	or	al,al
	jnz	get_current_path
      cut_current_path:
	cmp	edi,[esp]
	je	current_path_ok
	cmp	byte [edi-1],'\'
	je	current_path_ok
	cmp	byte [edi-1],'/'
	je	current_path_ok
	dec	edi
	jmp	cut_current_path
      current_path_ok:
	mov	esi,[esp+4]
	call	preprocess_path
	pop	edx
	mov	esi,edx
	call	open
	jnc	file_opened
	mov	edi,esi
	mov	esi,[esp]
	push	edi
	call	preprocess_path
	pop	edx
	mov	esi,edx
	call	open
	jc	file_not_found
      file_opened:
	mov	edi,esi
	pop	esi
	ret
reserve_bytes:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[next_pass_needed],0
	jne	rb_value_ok
	cmp	[value_type],0
	jne	invalid_use_of_symbol
      rb_value_ok:
	cmp	eax,0
	jl	reserve_negative
	mov	ecx,eax
	mov	edx,ecx
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[display_buffer]
	ja	out_of_memory
	push	edi
	cmp	[next_pass_needed],0
	je	zero_bytes
	add	edi,ecx
	jmp	reserved_data
      zero_bytes:
	xor	eax,eax
	shr	ecx,1
	jnc	bytes_stosb_ok
	stos	byte [edi]
      bytes_stosb_ok:
	shr	ecx,1
	jnc	bytes_stosw_ok
	stos	word [edi]
      bytes_stosw_ok:
	rep	stos dword [edi]
      reserved_data:
	pop	eax
	call	undefined_data
	jmp	instruction_assembled
      reserve_negative:
	cmp	[error_line],0
	jne	instruction_assembled
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],invalid_value
	jmp	instruction_assembled
reserve_words:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[next_pass_needed],0
	jne	rw_value_ok
	cmp	[value_type],0
	jne	invalid_use_of_symbol
      rw_value_ok:
	cmp	eax,0
	jl	reserve_negative
	mov	ecx,eax
	mov	edx,ecx
	shl	edx,1
	jc	out_of_memory
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[display_buffer]
	ja	out_of_memory
	push	edi
	cmp	[next_pass_needed],0
	je	zero_words
	lea	edi,[edi+ecx*2]
	jmp	reserved_data
      zero_words:
	xor	eax,eax
	shr	ecx,1
	jnc	words_stosw_ok
	stos	word [edi]
      words_stosw_ok:
	rep	stos dword [edi]
	jmp	reserved_data
reserve_dwords:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[next_pass_needed],0
	jne	rd_value_ok
	cmp	[value_type],0
	jne	invalid_use_of_symbol
      rd_value_ok:
	cmp	eax,0
	jl	reserve_negative
	mov	ecx,eax
	mov	edx,ecx
	shl	edx,1
	jc	out_of_memory
	shl	edx,1
	jc	out_of_memory
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[display_buffer]
	ja	out_of_memory
	push	edi
	cmp	[next_pass_needed],0
	je	zero_dwords
	lea	edi,[edi+ecx*4]
	jmp	reserved_data
      zero_dwords:
	xor	eax,eax
	rep	stos dword [edi]
	jmp	reserved_data
reserve_pwords:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[next_pass_needed],0
	jne	rp_value_ok
	cmp	[value_type],0
	jne	invalid_use_of_symbol
      rp_value_ok:
	cmp	eax,0
	jl	reserve_negative
	mov	ecx,eax
	shl	ecx,1
	jc	out_of_memory
	add	ecx,eax
	mov	edx,ecx
	shl	edx,1
	jc	out_of_memory
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[display_buffer]
	ja	out_of_memory
	push	edi
	cmp	[next_pass_needed],0
	je	zero_words
	lea	edi,[edi+ecx*2]
	jmp	reserved_data
reserve_qwords:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[next_pass_needed],0
	jne	rq_value_ok
	cmp	[value_type],0
	jne	invalid_use_of_symbol
      rq_value_ok:
	cmp	eax,0
	jl	reserve_negative
	mov	ecx,eax
	shl	ecx,1
	jc	out_of_memory
	mov	edx,ecx
	shl	edx,1
	jc	out_of_memory
	shl	edx,1
	jc	out_of_memory
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[display_buffer]
	ja	out_of_memory
	push	edi
	cmp	[next_pass_needed],0
	je	zero_dwords
	lea	edi,[edi+ecx*4]
	jmp	reserved_data
reserve_twords:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[next_pass_needed],0
	jne	rt_value_ok
	cmp	[value_type],0
	jne	invalid_use_of_symbol
      rt_value_ok:
	cmp	eax,0
	jl	reserve_negative
	mov	ecx,eax
	shl	ecx,2
	jc	out_of_memory
	add	ecx,eax
	mov	edx,ecx
	shl	edx,1
	jc	out_of_memory
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[display_buffer]
	ja	out_of_memory
	push	edi
	cmp	[next_pass_needed],0
	je	zero_words
	lea	edi,[edi+ecx*2]
	jmp	reserved_data
align_directive:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	edx,eax
	dec	edx
	test	eax,edx
	jnz	invalid_value
	or	eax,eax
	jz	invalid_value
	cmp	eax,1
	je	instruction_assembled
	mov	ecx,edi
	sub	ecx,[org_origin]
	cmp	[org_registers],0
	jne	section_not_aligned_enough
	cmp	[reloc_labels],0
	je	make_alignment
	cmp	[output_format],3
	je	pe_alignment
	mov	ebx,[org_symbol]
	cmp	byte [ebx],0
	jne	section_not_aligned_enough
	cmp	eax,[ebx+10h]
	jbe	make_alignment
	jmp	section_not_aligned_enough
      pe_alignment:
	cmp	eax,1000h
	ja	section_not_aligned_enough
      make_alignment:
	dec	eax
	and	ecx,eax
	jz	instruction_assembled
	neg	ecx
	add	ecx,eax
	inc	ecx
	mov	edx,ecx
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[display_buffer]
	ja	out_of_memory
	push	edi
	cmp	[next_pass_needed],0
	je	nops
	add	edi,ecx
	jmp	reserved_data
      nops:
	mov	eax,90909090h
	shr	ecx,1
	jnc	nops_stosb_ok
	stos	byte [edi]
      nops_stosb_ok:
	shr	ecx,1
	jnc	nops_stosw_ok
	stos	word [edi]
      nops_stosw_ok:
	rep	stos dword [edi]
	jmp	reserved_data
