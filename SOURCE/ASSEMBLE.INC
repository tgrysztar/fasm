
; flat assembler  version 1.05
; Copyright (c) 1999-2000, Tomasz Grysztar
; All rights reserved.

assembler:
	mov	edi,[last_label_id]
	mov	[labels_list],edi
	mov	ecx,[memory_end]
	sub	ecx,edi
	shr	ecx,2
	xor	eax,eax
	rep	stos dword [edi]
	mov	[current_pass],0
      assembler_loop:
	mov	[next_pass_needed],0
	mov	[output_format],0
	mov	[code_type],16
	mov	[reloc_labels],0
	mov	esi,[source_start]
	mov	edi,[code_start]
	mov	[org_start],edi
	mov	[error_line],0
	mov	[virtual_mode],0
	mov	[counter],0
	mov	[if_stack],0
	mov	[repeat_stack],0
	mov	[else_flag],0
	mov	[number_of_relocations],0
      pass_loop:
	call	assemble_line
	jnc	pass_loop
      pass_done:
	cmp	[virtual_mode],0
	jne	unexpected_end_of_file
	cmp	[next_pass_needed],0
	je	assemble_done
	inc	[current_pass]
	cmp	[current_pass],100
	jae	code_cannot_be_generated
	jmp	assembler_loop
      assemble_done:
	mov	eax,[error_line]
	or	eax,eax
	jz	assemble_ok
	mov	[current_line],eax
	mov	eax,[error_home_line]
	mov	[home_line],eax
	jmp	near [error]
      assemble_ok:
	cmp	[output_format],3
	je	assembled_pe
	mov	eax,edi
	sub	eax,[code_start]
	mov	[real_code_size],eax
	cmp	edi,[undefined_data_end]
	jne	calculate_code_size
	mov	edi,[undefined_data_start]
      calculate_code_size:
	sub	edi,[code_start]
	mov	[code_size],edi
	ret
      assembled_pe:
	call	close_section
	call	make_pe_reloc
	mov	eax,edi
	sub	eax,[code_start]
	mov	[code_size],eax
	ret

assemble_line:
	mov	eax,[labels_list]
	sub	eax,100h
	cmp	edi,eax
	jae	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	source_end
	cmp	al,1
	je	assemble_instruction
	cmp	al,2
	je	define_label
	cmp	al,3
	je	define_constant
	cmp	al,80h
	je	new_line
	cmp	al,81h
	je	new_home_line
	cmp	al,18h
	je	code_type_definition
	cmp	al,10h
	jne	illegal_instruction
	lods	byte [esi]
	mov	ah,al
	shr	ah,4
	cmp	ah,6
	jne	illegal_instruction
	and	al,1111b
	mov	[segment_register],al
	call	store_segment_prefix
	jmp	assemble_line
      code_type_definition:
	lods	byte [esi]
	mov	[code_type],al
	jmp	line_assembled
      new_line:
	lods	dword [esi]
	mov	[current_line],eax
	jmp	assemble_line
      new_home_line:
	lods	dword [esi]
	mov	[home_line],eax
	mov	[current_line],eax
	jmp	assemble_line
      define_label:
	lods	dword [esi]
	mov	ebx,eax
	lods	byte [esi]
	mov	dl,al
	xor	ch,ch
	cmp	[reloc_labels],0
	je	.reloc_ok
	mov	ch,2
      .reloc_ok:
	xchg	ch,[ebx+11]
	mov	al,[current_pass]
	cmp	byte [ebx+8],0
	je	.new
	cmp	al,[ebx+9]
	je	symbol_already_defined
	mov	[ebx+9],al
	mov	eax,edi
	sub	eax,[org_start]
	xchg	[ebx],eax
	xor	edx,edx
	xchg	[ebx+4],edx
	cmp	eax,[ebx]
	jne	.changed
	or	edx,edx
	jnz	.changed
	cmp	ch,[ebx+11]
	jne	.changed
	jmp	assemble_line
      .changed:
	mov	[next_pass_needed],1
	jmp	assemble_line
      .new:
	mov	byte [ebx+8],1
	mov	[ebx+9],al
	mov	byte [ebx+10],dl
	mov	eax,edi
	sub	eax,[org_start]
	mov	[ebx],eax
	mov	dword [ebx+4],0
	jmp	assemble_line
      define_constant:
	lods	dword [esi]
	push	eax
	lods	byte [esi]
	push	ax
	call	get_value
	pop	bx
	mov	ch,bl
	pop	ebx
	mov	cl,[current_pass]
	cmp	byte [ebx+8],0
	je	.new
	cmp	byte [ebx+12],1
	je	.redefine
	cmp	cl,[ebx+9]
	je	symbol_already_defined
      .redefine:
	mov	[ebx+9],cl
	xchg	[ebx],eax
	xchg	[ebx+4],edx
	jmp	assemble_line
      .new:
	mov	byte [ebx+12],1
	mov	byte [ebx+8],1
	mov	word [ebx+9],cx
	mov	[ebx],eax
	mov	dword [ebx+4],edx
	mov	al,[value_type]
	mov	[ebx+11],al
	jmp	assemble_line
      assemble_instruction:
	mov	[operand_size],0
	mov	[forced_size],0
	lods	word [esi]
	movzx	ebx,ax
	add	ebx,assembler
	lods	word [esi]
	jmp	near ebx
      instruction_assembled:
	mov	al,[esi]
	cmp	al,80h
	je	line_assembled
	cmp	al,81h
	je	line_assembled
	or	al,al
	jnz	extra_characters_on_line
      line_assembled:
	clc
	ret
      source_end:
	stc
	ret
skip_line:
	call	skip_symbol
	jnc	skip_line
	ret
skip_symbol:
	lods	byte [esi]
	or	al,al
	jz	.line_end
	cmp	al,80h
	je	.line_end
	cmp	al,81h
	je	.line_end
	cmp	al,1
	je	.instruction
	cmp	al,2
	je	.label
	cmp	al,3
	je	.label
	cmp	al,4
	je	.instruction
	cmp	al,20h
	jb	.symbol
	cmp	al,27h
	je	.string
	cmp	al,'('
	je	.expression
	cmp	al,'['
	je	.address
      .end:
	clc
	ret
      .label:
	inc	esi
      .instruction:
	add	esi,3
      .symbol:
	inc	esi
	jmp	.end
      .string:
	lods	dword [esi]
	add	esi,eax
	inc	esi
	jmp	.end
      .address:
	mov	al,[esi]
	and	al,11110000b
	cmp	al,60h
	jb	.expression
	cmp	al,70h
	ja	.expression
	inc	esi
	jmp	.address
      .expression:
	lods	byte [esi]
	cmp	al,'.'
	je	.fp
	cmp	al,')'
	je	.end
	cmp	al,']'
	je	.end
	cmp	al,10h
	je	.register
	cmp	al,11h
	je	.label_value
	cmp	al,80h
	jae	.expression
	movzx	eax,al
	add	esi,eax
	jmp	.expression
      .label_value:
	add	esi,3
      .register:
	inc	esi
	jmp	.expression
      .fp:
	add	esi,12
	jmp	.end
      .line_end:
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
	mov	[org_start],ecx
	jmp	instruction_assembled
label_directive:
	lods	byte [esi]
	cmp	al,4
	jne	invalid_argument
	lods	dword [esi]
	mov	ebx,eax
	call	get_operand_size
	mov	ch,[operand_size]
	mov	eax,edi
	sub	eax,[org_start]
	cmp	word [esi],13h
	jne	.define
	add	esi,2
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	mov	byte [ebx+11],0
	push	ebx cx
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	pop	cx ebx
	mov	dl,al
	mov	dh,[value_type]
	cmp	dh,1
	je	invalid_use_of_symbol
	jb	.reloc_ok
      .define:
	xor	dh,dh
	cmp	[reloc_labels],0
	je	.reloc_ok
	mov	dh,2
      .reloc_ok:
	xchg	dh,[ebx+11]
	mov	cl,[current_pass]
	cmp	byte [ebx+8],0
	je	.new
	cmp	cl,[ebx+9]
	je	symbol_already_defined
	mov	ch,dh
	mov	[ebx+9],cl
	xchg	[ebx],eax
	xor	edx,edx
	xchg	[ebx+4],edx
	cmp	eax,[ebx]
	jne	.changed
	or	edx,edx
	jnz	.changed
	cmp	ch,[ebx+11]
	jne	.changed
	jmp	instruction_assembled
      .changed:
	mov	[next_pass_needed],1
	jmp	instruction_assembled
      .new:
	mov	byte [ebx+8],1
	mov	[ebx+9],cl
	mov	byte [ebx+10],ch
	mov	[ebx],eax
	mov	dword [ebx+4],0
	jmp	instruction_assembled
virtual_directive:
	lods	byte [esi]
	cmp	al,13h
	jne	invalid_argument
	lods	byte [esi]
	or	al,al
	jnz	invalid_argument
	cmp	[virtual_mode],0
	jne	virtual_mode_org
	mov	eax,[org_start]
	mov	[stored_org],eax
	mov	[stored_offset],edi
	mov	al,[reloc_labels]
	mov	[stored_reloc_labels],al
      virtual_mode_org:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	mov	[virtual_mode],1
	mov	[reloc_labels],0
	mov	dl,[value_type]
	cmp	dl,1
	je	invalid_use_of_symbol
	cmp	dl,2
	jne	virtual_mode_ok
	mov	[reloc_labels],1
      virtual_mode_ok:
	mov	ecx,edi
	sub	ecx,eax
	mov	[org_start],ecx
	jmp	instruction_assembled
      end_virtual:
	cmp	[virtual_mode],0
	je	unexpected_instruction
	mov	[virtual_mode],0
	mov	eax,[stored_org]
	mov	[org_start],eax
	mov	edi,[stored_offset]
	mov	al,[stored_reloc_labels]
	mov	[reloc_labels],al
	jmp	instruction_assembled
times_directive:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	or	eax,eax
	jz	times_skip
	cmp	byte [esi],':'
	jne	times_argument_ok
	inc	esi
      times_argument_ok:
	push	[counter] [counter_limit]
	mov	[counter_limit],eax
	mov	[counter],1
      times_loop:
	push	esi
	call	assemble_line
	mov	eax,[counter_limit]
	cmp	[counter],eax
	je	times_done
	inc	[counter]
	pop	esi
	jmp	times_loop
      times_done:
	pop	eax
	pop	[counter_limit] [counter]
	jmp	instruction_assembled
      times_skip:
	call	skip_line
	jmp	instruction_assembled
repeat_directive:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	or	eax,eax
	jz	repeat_skip
	push	[counter] [counter_limit]
	mov	[counter_limit],eax
	mov	[counter],1
      repeat_loop:
	push	esi
	inc	[repeat_stack]
	push	[repeat_stack]
      repeat_assemble:
	call	assemble_line
	jc	unexpected_end_of_file
	mov	ax,[esp]
	cmp	ax,[repeat_stack]
	je	repeat_assemble
	pop	ax
	mov	eax,[counter_limit]
	cmp	[counter],eax
	je	repeat_done
	inc	[counter]
	pop	esi
	jmp	repeat_loop
      repeat_done:
	pop	eax
	pop	[counter_limit] [counter]
	jmp	instruction_assembled
      end_repeat:
	cmp	[repeat_stack],0
	je	unexpected_instruction
	dec	[repeat_stack]
	jmp	instruction_assembled
      repeat_skip:
	call	find_end_repeat
	jmp	instruction_assembled
      find_end_repeat:
	lods	byte [esi]
	add	esi,4
	cmp	al,80h
	je	.skip_labels
	cmp	al,81h
	je	.skip_labels
	or	al,al
	jz	unexpected_end_of_file
	sub	esi,5
      .skip_labels:
	cmp	byte [esi],2
	jne	.labels_ok
	add	esi,6
	jmp	.skip_labels
      .labels_ok:
	cmp	byte [esi],1
	jne	no_end_repeat
	cmp	word [esi+1],repeat_directive-assembler
	je	skip_repeat
	cmp	word [esi+1],end_directive-assembler
	jne	no_end_repeat
	cmp	byte [esi+5],1
	jne	no_end_repeat
	cmp	dword [esi+6],repeat_directive-assembler
	jne	no_end_repeat
	add	esi,10
	ret
      skip_repeat:
	call	skip_line
	call	find_end_repeat
      no_end_repeat:
	call	skip_symbol
	jmp	find_end_repeat
end_directive:
	lods	byte [esi]
	cmp	al,1
	jne	invalid_argument
	lods	dword [esi]
	cmp	ax,virtual_directive-assembler
	je	end_virtual
	cmp	ax,repeat_directive-assembler
	je	end_repeat
	cmp	ax,if_directive-assembler
	je	end_if
	jmp	invalid_argument
if_directive:
	call	calculate_logical_expression
	mov	bl,[esi]
	or	bl,bl
	jz	logical_expression_ok
	cmp	bl,80h
	je	logical_expression_ok
	cmp	bl,81h
	je	logical_expression_ok
	jmp	extra_characters_on_line
      logical_expression_ok:
	or	al,al
	jz	if_false
      if_true:
	inc	[if_stack]
	push	[if_stack]
      if_loop:
	call	assemble_line
	jc	unexpected_end_of_file
	cmp	[else_flag],0
	jne	else_false
	mov	ax,[esp]
	cmp	ax,[if_stack]
	je	if_loop
	pop	ax
	jmp	instruction_assembled
      else_false:
	pop	ax
	xor	al,al
	xchg	[else_flag],al
	dec	[if_stack]
	cmp	al,1
	je	skip_else
	call	find_end_if
	jnc	instruction_assembled
      skip_else:
	call	find_end_if
	jnc	instruction_assembled
	jmp	unexpected_instruction
      if_false:
	mov	[else_flag],3
	call	find_end_if
	mov	[else_flag],0
	jc	else_true
	jmp	instruction_assembled
      else_true:
	mov	al,byte [esi]
	cmp	al,80h
	je	else_only
	cmp	al,81h
	je	else_only
	or	al,al
	jz	unexpected_end_of_file
	cmp	al,1
	jne	extra_characters_on_line
	inc	esi
	lods	word [esi]
	cmp	ax,if_directive-assembler
	jne	extra_characters_on_line
	lods	word [esi]
	jmp	if_directive
      else_only:
	inc	[if_stack]
	push	[if_stack]
      else_loop:
	call	assemble_line
	jc	unexpected_end_of_file
	cmp	[else_flag],0
	jne	unexpected_instruction
	mov	ax,[esp]
	cmp	ax,[if_stack]
	je	else_loop
	pop	ax
	jmp	instruction_assembled
      end_if:
	cmp	[if_stack],0
	je	unexpected_instruction
	dec	[if_stack]
	jmp	instruction_assembled
      else_directive:
	cmp	[if_stack],0
	je	unexpected_instruction
	mov	[else_flag],1
	cmp	byte [esi],0
	je	unexpected_end_of_file
	cmp	byte [esi],80h
	je	instruction_assembled
	cmp	byte [esi],81h
	je	instruction_assembled
	cmp	byte [esi],1
	jne	extra_characters_on_line
	inc	esi
	lods	word [esi]
	cmp	ax,if_directive-assembler
	jne	extra_characters_on_line
	lods	word [esi]
	mov	[else_flag],2
	jmp	line_assembled
      find_end_if:
	lods	byte [esi]
	cmp	al,80h
	je	.new_line
	cmp	al,81h
	je	.new_home_line
	or	al,al
	jz	unexpected_end_of_file
	dec	esi
	jmp	.skip_labels
      .new_line:
	lods	dword [esi]
	mov	[current_line],eax
	jmp	.skip_labels
      .new_home_line:
	lods	dword [esi]
	mov	[home_line],eax
	mov	[current_line],eax
	jmp	.skip_labels
      .skip_labels:
	cmp	byte [esi],2
	jne	.labels_ok
	add	esi,6
	jmp	.skip_labels
      .labels_ok:
	cmp	byte [esi],1
	jne	no_end_if
	cmp	word [esi+1],if_directive-assembler
	je	found_if
	cmp	word [esi+1],else_directive-assembler
	je	found_else
	cmp	word [esi+1],end_directive-assembler
	jne	no_end_if
	cmp	byte [esi+5],1
	jne	no_end_if
	cmp	dword [esi+6],if_directive-assembler
	jne	no_end_if
	add	esi,10
	clc
	ret
      found_else:
	add	esi,5
	mov	al,byte [esi]
	or	al,al
	jz	unexpected_end_of_file
	cmp	al,80h
	je	found_else_only
	cmp	al,81h
	je	found_else_only
	cmp	[else_flag],3
	je	found_else_only
	cmp	al,1
	jne	extra_characters_on_line
	inc	esi
	lods	word [esi]
	cmp	ax,if_directive-assembler
	jne	extra_characters_on_line
	lods	word [esi]
	jmp	find_end_if
      found_else_only:
	stc
	ret
      found_if:
	call	skip_line
	call	find_end_if
	jnc	find_end_if
	call	find_end_if
	jnc	find_end_if
	jmp	unexpected_instruction
      no_end_if:
	call	skip_symbol
	jmp	find_end_if

data_bytes:
	lods	byte [esi]
	cmp	al,'('
	je	get_byte
	cmp	al,27h
	je	get_string
	cmp	al,'?'
	jne	invalid_operand
	mov	eax,edi
	mov	byte [edi],0
	inc	edi
	call	undefined_data
	jmp	byte_ok
      get_byte:
	call	get_byte_value
	stos	byte [edi]
      byte_ok:
	cmp	edi,[labels_list]
	jae	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	data_end
	cmp	al,80h
	je	data_end
	cmp	al,81h
	je	data_end
	cmp	al,','
	jne	extra_characters_on_line
	jmp	data_bytes
      data_end:
	dec	esi
	jmp	instruction_assembled
      get_string:
	lods	dword [esi]
	mov	ecx,eax
	rep	movs byte [edi],[esi]
	inc	esi
	jmp	byte_ok
      undefined_data:
	cmp	[virtual_mode],0
	je	mark_undefined_data
	ret
      mark_undefined_data:
	cmp	eax,[undefined_data_end]
	je	undefined_data_ok
	mov	[undefined_data_start],eax
      undefined_data_ok:
	mov	[undefined_data_end],edi
	ret
data_words:
	lods	byte [esi]
	cmp	al,'('
	je	get_word
	cmp	al,'?'
	jne	invalid_operand
	mov	eax,edi
	mov	word [edi],0
	add	edi,2
	call	undefined_data
	jmp	word_ok
      get_word:
	call	get_word_value
	call	mark_relocation
	stos	word [edi]
      word_ok:
	cmp	edi,[labels_list]
	jae	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	data_end
	cmp	al,80h
	je	data_end
	cmp	al,81h
	je	data_end
	cmp	al,','
	jne	extra_characters_on_line
	jmp	data_words
data_dwords:
	lods	byte [esi]
	cmp	al,'('
	je	get_dword
	cmp	al,'?'
	jne	invalid_operand
	mov	eax,edi
	mov	dword [edi],0
	add	edi,4
	call	undefined_data
	jmp	dword_ok
      get_dword:
	call	get_dword_value
	call	mark_relocation
	stos	dword [edi]
      dword_ok:
	cmp	edi,[labels_list]
	jae	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	data_end
	cmp	al,80h
	je	data_end
	cmp	al,81h
	je	data_end
	cmp	al,','
	jne	extra_characters_on_line
	jmp	data_dwords
data_pwords:
	lods	byte [esi]
	cmp	al,'('
	je	get_pword
	cmp	al,'?'
	jne	invalid_operand
	mov	eax,edi
	mov	dword [edi],0
	add	edi,4
	mov	word [edi],0
	add	edi,2
	call	undefined_data
	jmp	pword_ok
      get_pword:
	call	get_pword_value
	call	mark_relocation
	stos	dword [edi]
	mov	ax,dx
	stos	word [edi]
      pword_ok:
	cmp	edi,[labels_list]
	jae	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	data_end
	cmp	al,80h
	je	data_end
	cmp	al,81h
	je	data_end
	cmp	al,','
	jne	extra_characters_on_line
	jmp	data_pwords
data_qwords:
	lods	byte [esi]
	cmp	al,'('
	je	get_qword
	cmp	al,'?'
	jne	invalid_operand
	mov	eax,edi
	mov	dword [edi],0
	add	edi,4
	mov	dword [edi],0
	add	edi,4
	call	undefined_data
	jmp	qword_ok
      get_qword:
	call	get_qword_value
	call	mark_relocation
	stos	dword [edi]
	mov	eax,edx
	stos	dword [edi]
      qword_ok:
	cmp	edi,[labels_list]
	jae	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	data_end
	cmp	al,80h
	je	data_end
	cmp	al,81h
	je	data_end
	cmp	al,','
	jne	extra_characters_on_line
	jmp	data_qwords
data_tbyte:
	lods	byte [esi]
	cmp	al,'('
	je	get_tbyte
	cmp	al,'?'
	jne	invalid_operand
	mov	eax,edi
	mov	dword [edi],0
	add	edi,4
	mov	dword [edi],0
	add	edi,4
	mov	word [edi],0
	add	edi,2
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
	stos	word [edi]
	add	esi,12
      tbyte_ok:
	cmp	edi,[labels_list]
	jae	out_of_memory
	lods	byte [esi]
	or	al,al
	jz	data_end
	cmp	al,80h
	je	data_end
	cmp	al,81h
	je	data_end
	cmp	al,','
	jne	extra_characters_on_line
	jmp	data_tbyte
data_file:
	lods	byte [esi]
	cmp	al,27h
	jne	invalid_operand
	lea	edx,[esi+4]
	mov	eax,[esi]
	lea	esi,[esi+4+eax+1]
	call	open
	jc	file_not_found
	mov	al,2
	xor	edx,edx
	call	lseek
	push	eax
	cmp	[next_pass_needed],0
	jne	file_reserve
	xor	al,al
	xor	edx,edx
	call	lseek
	pop	ecx
	mov	edx,edi
	add	edi,ecx
	jc	out_of_memory
	cmp	edi,[labels_list]
	jae	out_of_memory
	call	read
	call	close
      check_for_next_name:
	lods	byte [esi]
	cmp	al,','
	je	data_file
	dec	esi
	jmp	instruction_assembled
      file_reserve:
	call	close
	pop	ecx
	add	edi,ecx
	jc	out_of_memory
	cmp	edi,[labels_list]
	jae	out_of_memory
	jmp	check_for_next_name
reserve_bytes:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	ecx,eax
	mov	edx,ecx
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[labels_list]
	jae	out_of_memory
	push	edi
	cmp	[next_pass_needed],0
	je	zero_bytes
	add	edi,ecx
	jmp	reserved_data
      zero_bytes:
	xor	al,al
	rep	stos byte [edi]
      reserved_data:
	pop	eax
	call	undefined_data
	jmp	instruction_assembled
reserve_words:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	ecx,eax
	mov	edx,ecx
	shl	edx,1
	jc	out_of_memory
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[labels_list]
	jae	out_of_memory
	push	edi
	cmp	[next_pass_needed],0
	je	zero_words
	lea	edi,[edi+ecx*2]
	jmp	reserved_data
      zero_words:
	xor	eax,eax
	rep	stos word [edi]
	jmp	reserved_data
reserve_dwords:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	ecx,eax
	mov	edx,ecx
	shl	edx,1
	jc	out_of_memory
	shl	edx,1
	jc	out_of_memory
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[labels_list]
	jae	out_of_memory
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
	jne	invalid_operand
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	ecx,eax
	shl	ecx,1
	jc	out_of_memory
	add	ecx,eax
	mov	edx,ecx
	shl	edx,1
	jc	out_of_memory
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[labels_list]
	jae	out_of_memory
	push	edi
	cmp	[next_pass_needed],0
	je	zero_pwords
	lea	edi,[edi+ecx*2]
	jmp	reserved_data
      zero_pwords:
	xor	eax,eax
	rep	stos word [edi]
	jmp	reserved_data
reserve_qwords:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
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
	cmp	edx,[labels_list]
	jae	out_of_memory
	push	edi
	cmp	[next_pass_needed],0
	je	zero_qwords
	lea	edi,[edi+ecx*4]
	jmp	reserved_data
      zero_qwords:
	xor	eax,eax
	rep	stos dword [edi]
	jmp	reserved_data
reserve_tbyte:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	ecx,eax
	shl	ecx,2
	jc	out_of_memory
	add	ecx,eax
	mov	edx,ecx
	shl	edx,1
	jc	out_of_memory
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[labels_list]
	jae	out_of_memory
	push	edi
	cmp	[next_pass_needed],0
	je	zero_tbyte
	lea	edi,[edi+ecx*2]
	jmp	reserved_data
      zero_tbyte:
	xor	eax,eax
	rep	stos word [edi]
	jmp	reserved_data

simple_instruction:
	stos	byte [edi]
	jmp	instruction_assembled
simple_instruction_16bit:
	cmp	[code_type],32
	je	size_prefix
	stos	byte [edi]
	jmp	instruction_assembled
      size_prefix:
	mov	ah,al
	mov	al,66h
	stos	word [edi]
	jmp	instruction_assembled
simple_instruction_32bit:
	cmp	[code_type],16
	je	size_prefix
	stos	byte [edi]
	jmp	instruction_assembled
simple_extended_instruction:
	mov	ah,al
	mov	al,0Fh
	stos	word [edi]
	jmp	instruction_assembled
prefix_instruction:
	stos	byte [edi]
	jmp	assemble_line
int_instruction:
	call	get_operand_size
	cmp	[operand_size],1
	ja	invalid_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	call	get_byte_value
	mov	ah,al
	mov	al,0CDh
	stos	word [edi]
	jmp	instruction_assembled
aa_instruction:
	push	ax
	call	get_operand_size
	mov	bl,10
	cmp	byte [esi],'('
	jne	.store
	inc	esi
	xor	al,al
	xchg	al,[operand_size]
	cmp	al,1
	ja	invalid_operand_size
	call	get_byte_value
	mov	bl,al
      .store:
	cmp	[operand_size],0
	jne	invalid_operand
	pop	ax
	mov	ah,bl
	stos	word [edi]
	jmp	instruction_assembled

basic_instruction:
	mov	[base_code],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	basic_reg
	cmp	al,'['
	jne	invalid_operand
      basic_mem:
	call	get_address
	push	edx bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	je	basic_mem_imm
	cmp	al,10h
	jne	invalid_operand
      basic_mem_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	al,[operand_size]
	cmp	al,1
	je	basic_mem_reg_8bit
	cmp	al,2
	je	basic_mem_reg_16bit
	cmp	al,4
	je	basic_mem_reg_32bit
	jmp	invalid_operand_size
      basic_mem_reg_8bit:
	call	store_instruction
	jmp	instruction_assembled
      basic_mem_reg_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      basic_mem_reg_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      basic_mem_imm:
	mov	al,[operand_size]
	cmp	al,1
	je	basic_mem_imm_8bit
	cmp	al,2
	je	basic_mem_imm_16bit
	cmp	al,4
	je	basic_mem_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
	jmp	basic_mem_imm_32bit
      basic_mem_imm_8bit:
	call	get_byte_value
	mov	byte [value],al
	mov	al,[base_code]
	shr	al,3
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	[base_code],80h
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      basic_mem_imm_16bit:
	call	get_word_value
	mov	word [value],ax
	mov	al,[base_code]
	shr	al,3
	mov	[postbyte_register],al
	call	operand_16bit_prefix
	pop	cx bx edx
	cmp	[value_type],0
	jne	.store
	cmp	word [value],80h
	jb	basic_mem_simm_8bit
	cmp	word [value],-80h
	jae	basic_mem_simm_8bit
      .store:
	mov	[base_code],81h
	call	store_instruction
	mov	ax,word [value]
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      basic_mem_simm_8bit:
	mov	[base_code],83h
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      basic_mem_imm_32bit:
	call	get_dword_value
	mov	dword [value],eax
	mov	al,[base_code]
	shr	al,3
	mov	[postbyte_register],al
	call	operand_32bit_prefix
	pop	cx bx edx
	cmp	[value_type],0
	jne	.store
	cmp	dword [value],80h
	jb	basic_mem_simm_8bit
	cmp	dword [value],-80h
	jae	basic_mem_simm_8bit
      .store:
	mov	[base_code],81h
	call	store_instruction
	mov	eax,dword [value]
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      basic_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	basic_reg_reg
	cmp	al,'('
	je	basic_reg_imm
	cmp	al,'['
	jne	invalid_operand
      basic_reg_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	basic_reg_mem_8bit
	cmp	al,2
	je	basic_reg_mem_16bit
	cmp	al,4
	je	basic_reg_mem_32bit
	jmp	invalid_operand_size
      basic_reg_mem_8bit:
	add	[base_code],2
	call	store_instruction
	jmp	instruction_assembled
      basic_reg_mem_16bit:
	call	operand_16bit_prefix
	add	[base_code],3
	call	store_instruction
	jmp	instruction_assembled
      basic_reg_mem_32bit:
	call	operand_32bit_prefix
	add	[base_code],3
	call	store_instruction
	jmp	instruction_assembled
      basic_reg_reg:
	lods	byte [esi]
	call	convert_register
	shl	al,3
	mov	bl,[postbyte_register]
	or	bl,al
	or	bl,11000000b
	mov	al,[operand_size]
	cmp	al,1
	je	basic_reg_reg_8bit
	cmp	al,2
	je	basic_reg_reg_16bit
	cmp	al,4
	je	basic_reg_reg_32bit
	jmp	invalid_operand_size
      basic_reg_reg_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	jmp	basic_reg_reg_8bit
      basic_reg_reg_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
      basic_reg_reg_8bit:
	mov	al,[base_code]
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      basic_reg_imm:
	mov	al,[operand_size]
	cmp	al,1
	je	basic_reg_imm_8bit
	cmp	al,2
	je	basic_reg_imm_16bit
	cmp	al,4
	je	basic_reg_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
	jmp	basic_reg_imm_32bit
      basic_reg_imm_8bit:
	call	get_byte_value
	mov	dl,al
	mov	ah,[base_code]
	or	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	bl,bl
	jz	basic_al_imm
	or	ah,bl
	mov	al,80h
	stos	word [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      basic_al_imm:
	mov	al,[base_code]
	add	al,4
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      basic_reg_imm_16bit:
	call	get_word_value
	mov	dx,ax
	call	operand_16bit_prefix
	mov	ah,[base_code]
	or	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	ah,bl
	cmp	[value_type],0
	jne	.store
	cmp	dx,80h
	jb	basic_reg_simm_8bit
	cmp	dx,-80h
	jae	basic_reg_simm_8bit
      .store:
	or	bl,bl
	jz	basic_ax_imm
	mov	al,81h
	stos	word [edi]
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      basic_reg_simm_8bit:
	mov	al,83h
	stos	word [edi]
	mov	ax,dx
	stos	byte [edi]
	jmp	instruction_assembled
      basic_ax_imm:
	mov	al,[base_code]
	add	al,5
	stos	byte [edi]
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      basic_reg_imm_32bit:
	call	get_dword_value
	mov	edx,eax
	call	operand_32bit_prefix
	mov	ah,[base_code]
	or	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	ah,bl
	cmp	[value_type],0
	jne	.store
	cmp	edx,80h
	jb	basic_reg_simm_8bit
	cmp	edx,-80h
	jae	basic_reg_simm_8bit
      .store:
	or	bl,bl
	jz	basic_eax_imm
	mov	al,81h
	stos	word [edi]
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      basic_eax_imm:
	mov	al,[base_code]
	add	al,5
	stos	byte [edi]
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
single_operand_instruction:
	mov	[base_code],0F6h
	mov	[postbyte_register],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	single_reg
	cmp	al,'['
	jne	invalid_operand
      single_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	single_mem_8bit
	cmp	al,2
	je	single_mem_16bit
	cmp	al,4
	je	single_mem_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      single_mem_8bit:
	call	store_instruction
	jmp	instruction_assembled
      single_mem_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      single_mem_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      single_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	al,[operand_size]
	cmp	al,1
	je	single_reg_8bit
	cmp	al,2
	je	single_reg_16bit
	cmp	al,4
	je	single_reg_32bit
	jmp	invalid_operand_size
      single_reg_8bit:
	mov	ah,bl
	mov	al,0F6h
	stos	word [edi]
	jmp	instruction_assembled
      single_reg_16bit:
	call	operand_16bit_prefix
	mov	ah,bl
	mov	al,0F7h
	stos	word [edi]
	jmp	instruction_assembled
      single_reg_32bit:
	call	operand_32bit_prefix
	mov	ah,bl
	mov	al,0F7h
	stos	word [edi]
	jmp	instruction_assembled
mov_instruction:
	mov	[base_code],88h
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	mov_reg
	cmp	al,'['
	jne	invalid_operand
      mov_mem:
	call	get_address
	push	edx bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	je	mov_mem_imm
	cmp	al,10h
	jne	invalid_operand
      mov_mem_reg:
	lods	byte [esi]
	cmp	al,60h
	jae	mov_mem_sreg
	call	convert_register
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	ah,[operand_size]
	cmp	ah,1
	je	mov_mem_reg_8bit
	cmp	ah,2
	je	mov_mem_reg_16bit
	cmp	ah,4
	je	mov_mem_reg_32bit
	jmp	invalid_operand_size
      mov_mem_reg_8bit:
	or	al,bl
	or	al,bh
	jz	mov_mem_al
	call	store_instruction
	jmp	instruction_assembled
      mov_mem_al:
	cmp	ch,2
	je	mov_mem_address16_al
	test	ch,4
	jnz	mov_mem_address32_al
	or	ch,ch
	jnz	invalid_address_size
	cmp	edx,8000h
	jb	mov_mem_address16_al
	cmp	edx,-8000h
	jae	mov_mem_address16_al
      mov_mem_address32_al:
	call	address_32bit_prefix
	call	store_segment_prefix
	mov	al,0A2h
      store_mov_address32:
	stos	byte [edi]
	push	instruction_assembled
	jmp	store_address_32bit_value
      mov_mem_address16_al:
	call	address_16bit_prefix
	call	store_segment_prefix
	mov	al,0A2h
      store_mov_address16:
	stos	byte [edi]
	mov	eax,edx
	stos	word [edi]
	cmp	edx,8000h
	jge	value_out_of_range
	cmp	edx,-8000h
	jl	value_out_of_range
	jmp	instruction_assembled
      mov_mem_reg_16bit:
	call	operand_16bit_prefix
	mov	al,[postbyte_register]
	or	al,bl
	or	al,bh
	jz	mov_mem_ax
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      mov_mem_ax:
	cmp	ch,2
	je	mov_mem_address16_ax
	test	ch,4
	jnz	mov_mem_address32_ax
	or	ch,ch
	jnz	invalid_address_size
	cmp	edx,8000h
	jb	mov_mem_address16_ax
	cmp	edx,-8000h
	jae	mov_mem_address16_ax
      mov_mem_address32_ax:
	call	address_32bit_prefix
	call	store_segment_prefix
	mov	al,0A3h
	jmp	store_mov_address32
      mov_mem_address16_ax:
	call	address_16bit_prefix
	call	store_segment_prefix
	mov	al,0A3h
	jmp	store_mov_address16
      mov_mem_reg_32bit:
	call	operand_32bit_prefix
	mov	al,[postbyte_register]
	or	al,bl
	or	al,bh
	jz	mov_mem_ax
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      mov_mem_sreg:
	cmp	al,70h
	jae	invalid_operand
	sub	al,61h
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	ah,[operand_size]
	or	ah,ah
	jz	mov_mem_sreg_size_ok
	cmp	ah,2
	jne	invalid_operand_size
      mov_mem_sreg_size_ok:
	call	operand_16bit_prefix
	mov	[base_code],8Ch
	call	store_instruction
	jmp	instruction_assembled
      mov_mem_imm:
	mov	al,[operand_size]
	cmp	al,1
	je	mov_mem_imm_8bit
	cmp	al,2
	je	mov_mem_imm_16bit
	cmp	al,4
	je	mov_mem_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
	jmp	mov_mem_imm_32bit
      mov_mem_imm_8bit:
	call	get_byte_value
	mov	byte [value],al
	mov	[postbyte_register],0
	mov	[base_code],0C6h
	pop	cx bx edx
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      mov_mem_imm_16bit:
	call	get_word_value
	mov	word [value],ax
	mov	[postbyte_register],0
	mov	[base_code],0C7h
	call	operand_16bit_prefix
	pop	cx bx edx
	call	store_instruction
	mov	ax,word [value]
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      mov_mem_imm_32bit:
	call	get_dword_value
	mov	dword [value],eax
	mov	[postbyte_register],0
	mov	[base_code],0C7h
	call	operand_32bit_prefix
	pop	cx bx edx
	call	store_instruction
	mov	eax,dword [value]
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      mov_reg:
	lods	byte [esi]
	cmp	al,60h
	jae	mov_sreg
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	je	mov_reg_mem
	cmp	al,'('
	je	mov_reg_imm
	cmp	al,10h
	jne	invalid_operand
      mov_reg_reg:
	lods	byte [esi]
	cmp	al,60h
	jae	mov_reg_sreg
	call	convert_register
	shl	al,3
	mov	bl,[postbyte_register]
	or	bl,al
	or	bl,11000000b
	mov	al,[operand_size]
	cmp	al,1
	je	mov_reg_reg_8bit
	cmp	al,2
	je	mov_reg_reg_16bit
	cmp	al,4
	je	mov_reg_reg_32bit
	jmp	invalid_operand_size
      mov_reg_reg_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	jmp	mov_reg_reg_8bit
      mov_reg_reg_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
      mov_reg_reg_8bit:
	mov	al,[base_code]
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      mov_reg_sreg:
	cmp	al,70h
	jae	mov_reg_creg
	sub	al,61h
	mov	bl,[postbyte_register]
	shl	al,3
	or	bl,al
	or	bl,11000000b
	cmp	[operand_size],2
	jne	invalid_operand_size
	mov	al,8Ch
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      mov_reg_creg:
	cmp	al,80h
	jae	mov_reg_dreg
	sub	al,70h
	mov	bh,20h
	jmp	mov_reg_xrx
      mov_reg_dreg:
	cmp	al,90h
	jae	mov_reg_treg
	sub	al,80h
	mov	bh,21h
	jmp	mov_reg_xrx
      mov_reg_treg:
	sub	al,90h
	mov	bh,24h
      mov_reg_xrx:
	mov	bl,[postbyte_register]
	shl	al,3
	or	bl,al
	or	bl,11000000b
	cmp	[operand_size],4
	jne	invalid_operand_size
	mov	ah,bh
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      mov_reg_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	mov_reg_mem_8bit
	cmp	al,2
	je	mov_reg_mem_16bit
	cmp	al,4
	je	mov_reg_mem_32bit
	jmp	invalid_operand_size
      mov_reg_mem_8bit:
	mov	al,[postbyte_register]
	or	al,bl
	or	al,bh
	jz	mov_al_mem
	add	[base_code],2
	call	store_instruction
	jmp	instruction_assembled
      mov_al_mem:
	cmp	ch,2
	je	mov_al_mem_address16
	test	ch,4
	jnz	mov_al_mem_address32
	or	ch,ch
	jnz	invalid_address_size
	cmp	edx,8000h
	jb	mov_al_mem_address16
	cmp	edx,-8000h
	jae	mov_al_mem_address16
      mov_al_mem_address32:
	call	address_32bit_prefix
	call	store_segment_prefix
	mov	al,0A0h
	jmp	store_mov_address32
      mov_al_mem_address16:
	call	address_16bit_prefix
	call	store_segment_prefix
	mov	al,0A0h
	jmp	store_mov_address16
      mov_reg_mem_16bit:
	call	operand_16bit_prefix
	mov	al,[postbyte_register]
	or	al,bl
	or	al,bh
	jz	mov_ax_mem
	add	[base_code],3
	call	store_instruction
	jmp	instruction_assembled
      mov_ax_mem:
	cmp	ch,2
	je	mov_ax_mem_address16
	test	ch,4
	jnz	mov_ax_mem_address32
	or	ch,ch
	jnz	invalid_address_size
	cmp	edx,8000h
	jb	mov_ax_mem_address16
	cmp	edx,-8000h
	jae	mov_ax_mem_address16
      mov_ax_mem_address32:
	call	address_32bit_prefix
	call	store_segment_prefix
	mov	al,0A1h
	jmp	store_mov_address32
      mov_ax_mem_address16:
	call	address_16bit_prefix
	call	store_segment_prefix
	mov	al,0A1h
	jmp	store_mov_address16
      mov_reg_mem_32bit:
	call	operand_32bit_prefix
	mov	al,[postbyte_register]
	or	al,bl
	or	al,bh
	jz	mov_ax_mem
	add	[base_code],3
	call	store_instruction
	jmp	instruction_assembled
      mov_reg_imm:
	mov	al,[operand_size]
	cmp	al,1
	je	mov_reg_imm_8bit
	cmp	al,2
	je	mov_reg_imm_16bit
	cmp	al,4
	je	mov_reg_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
	jmp	mov_reg_imm_32bit
      mov_reg_imm_8bit:
	call	get_byte_value
	mov	ah,al
	mov	al,[postbyte_register]
	and	al,111b
	add	al,0B0h
	stos	word [edi]
	jmp	instruction_assembled
      mov_reg_imm_16bit:
	call	get_word_value
	mov	dx,ax
	call	operand_16bit_prefix
	mov	al,[postbyte_register]
	and	al,111b
	add	al,0B8h
	stos	byte [edi]
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      mov_reg_imm_32bit:
	call	get_dword_value
	mov	edx,eax
	call	operand_32bit_prefix
	mov	al,[postbyte_register]
	and	al,111b
	add	al,0B8h
	stos	byte [edi]
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      mov_sreg:
	cmp	al,70h
	jae	mov_creg
	sub	al,61h
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	je	mov_sreg_mem
	cmp	al,10h
	jne	invalid_operand
      mov_sreg_reg:
	lods	byte [esi]
	call	convert_register
	mov	ah,[operand_size]
	or	ah,ah
	jz	mov_sreg_reg_size_ok
	cmp	ah,2
	jne	invalid_operand_size
      mov_sreg_reg_size_ok:
	mov	bl,11000000b
	or	bl,al
	mov	al,[postbyte_register]
	shl	al,3
	or	bl,al
	mov	al,8Eh
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      mov_sreg_mem:
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	mov_sreg_mem_size_ok
	cmp	al,2
	jne	invalid_operand_size
      mov_sreg_mem_size_ok:
	mov	[base_code],8Eh
	call	store_instruction
	jmp	instruction_assembled
      mov_creg:
	cmp	al,80h
	jae	mov_dreg
	sub	al,70h
	mov	dl,22h
	jmp	mov_xrx
      mov_dreg:
	cmp	al,90h
	jae	mov_treg
	sub	al,80h
	mov	dl,23h
	jmp	mov_xrx
      mov_treg:
	sub	al,90h
	mov	dl,26h
      mov_xrx:
	mov	bh,al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	[operand_size],4
	jne	invalid_operand_size
	mov	bl,11000000b
	or	bl,al
	mov	al,bh
	shl	al,3
	or	bl,al
	mov	al,0Fh
	mov	ah,dl
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
cmov_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	je	cmov_reg_mem
	cmp	al,10h
	jne	invalid_operand
      cmov_reg_reg:
	lods	byte [esi]
	call	convert_register
	shl	al,3
	mov	bl,[postbyte_register]
	or	bl,al
	or	bl,11000000b
	mov	al,[operand_size]
	cmp	al,2
	je	cmov_reg_reg_16bit
	cmp	al,4
	je	cmov_reg_reg_32bit
	jmp	invalid_operand_size
      cmov_reg_reg_32bit:
	call	operand_32bit_prefix
	jmp	cmov_reg_reg_store
      cmov_reg_reg_16bit:
	call	operand_16bit_prefix
      cmov_reg_reg_store:
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      cmov_reg_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	cmov_reg_mem_16bit
	cmp	al,4
	je	cmov_reg_mem_32bit
	jmp	invalid_operand_size
      cmov_reg_mem_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      cmov_reg_mem_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
test_instruction:
	mov	[base_code],84h
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	test_reg
	cmp	al,'['
	jne	invalid_operand
      test_mem:
	call	get_address
	push	edx bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	je	test_mem_imm
	cmp	al,10h
	jne	invalid_operand
      test_mem_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	al,[operand_size]
	cmp	al,1
	je	test_mem_reg_8bit
	cmp	al,2
	je	test_mem_reg_16bit
	cmp	al,4
	je	test_mem_reg_32bit
	jmp	invalid_operand_size
      test_mem_reg_8bit:
	call	store_instruction
	jmp	instruction_assembled
      test_mem_reg_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      test_mem_reg_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      test_mem_imm:
	mov	al,[operand_size]
	cmp	al,1
	je	test_mem_imm_8bit
	cmp	al,2
	je	test_mem_imm_16bit
	cmp	al,4
	je	test_mem_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
	jmp	test_mem_imm_32bit
      test_mem_imm_8bit:
	call	get_byte_value
	mov	byte [value],al
	mov	[postbyte_register],0
	mov	[base_code],0F6h
	pop	cx bx edx
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      test_mem_imm_16bit:
	call	get_word_value
	mov	word [value],ax
	mov	[postbyte_register],0
	mov	[base_code],0F7h
	call	operand_16bit_prefix
	pop	cx bx edx
	call	store_instruction
	mov	ax,word [value]
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      test_mem_imm_32bit:
	call	get_dword_value
	mov	dword [value],eax
	mov	[postbyte_register],0
	mov	[base_code],0F7h
	call	operand_32bit_prefix
	pop	cx bx edx
	call	store_instruction
	mov	eax,dword [value]
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      test_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	je	test_reg_imm
	cmp	al,10h
	jne	invalid_operand
      test_reg_reg:
	lods	byte [esi]
	call	convert_register
	shl	al,3
	mov	bl,[postbyte_register]
	or	bl,al
	or	bl,11000000b
	mov	al,[operand_size]
	cmp	al,1
	je	test_reg_reg_8bit
	cmp	al,2
	je	test_reg_reg_16bit
	cmp	al,4
	je	test_reg_reg_32bit
	jmp	invalid_operand_size
      test_reg_reg_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	jmp	basic_reg_reg_8bit
      test_reg_reg_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
      test_reg_reg_8bit:
	mov	al,[base_code]
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      test_reg_imm:
	mov	al,[operand_size]
	cmp	al,1
	je	test_reg_imm_8bit
	cmp	al,2
	je	test_reg_imm_16bit
	cmp	al,4
	je	test_reg_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
	jmp	test_reg_imm_32bit
      test_reg_imm_8bit:
	call	get_byte_value
	mov	dl,al
	mov	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	bl,bl
	jz	test_al_imm
	or	ah,bl
	mov	al,0F6h
	stos	word [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      test_al_imm:
	mov	al,0A8h
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      test_reg_imm_16bit:
	call	get_word_value
	mov	dx,ax
	call	operand_16bit_prefix
	mov	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	bl,bl
	jz	test_ax_imm
	or	ah,bl
	mov	al,0F7h
	stos	word [edi]
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      test_ax_imm:
	mov	al,0A9h
	stos	byte [edi]
	mov	ax,dx
	stos	word [edi]
	jmp	instruction_assembled
      test_reg_imm_32bit:
	call	get_dword_value
	mov	edx,eax
	call	operand_32bit_prefix
	mov	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	bl,bl
	jz	test_eax_imm
	or	ah,bl
	mov	al,0F7h
	stos	word [edi]
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      test_eax_imm:
	mov	al,0A9h
	stos	byte [edi]
	mov	eax,edx
	stos	dword [edi]
	jmp	instruction_assembled
xchg_instruction:
	mov	[base_code],86h
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	xchg_reg
	cmp	al,'['
	jne	invalid_operand
      xchg_mem:
	call	get_address
	push	edx bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
      xchg_mem_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	al,[operand_size]
	cmp	al,1
	je	xchg_mem_reg_8bit
	cmp	al,2
	je	xchg_mem_reg_16bit
	cmp	al,4
	je	xchg_mem_reg_32bit
	jmp	invalid_operand_size
      xchg_mem_reg_8bit:
	call	store_instruction
	jmp	instruction_assembled
      xchg_mem_reg_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      xchg_mem_reg_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      xchg_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	je	xchg_reg_mem
	cmp	al,10h
	jne	invalid_operand
      xchg_reg_reg:
	lods	byte [esi]
	call	convert_register
	mov	bh,al
	mov	bl,[postbyte_register]
	shl	[postbyte_register],3
	or	al,11000000b
	or	[postbyte_register],al
	mov	al,[operand_size]
	cmp	al,1
	je	xchg_reg_reg_8bit
	cmp	al,2
	je	xchg_reg_reg_16bit
	cmp	al,4
	je	xchg_reg_reg_32bit
	jmp	invalid_operand_size
      xchg_reg_reg_32bit:
	call	operand_32bit_prefix
	or	bh,bh
	jz	xchg_ax_reg
	xchg	bh,bl
	or	bh,bh
	jz	xchg_ax_reg
	inc	[base_code]
	jmp	xchg_reg_reg_8bit
      xchg_reg_reg_16bit:
	call	operand_16bit_prefix
	or	bh,bh
	jz	xchg_ax_reg
	xchg	bh,bl
	or	bh,bh
	jz	xchg_ax_reg
	inc	[base_code]
      xchg_reg_reg_8bit:
	mov	al,[base_code]
	mov	ah,[postbyte_register]
	stos	word [edi]
	jmp	instruction_assembled
      xchg_ax_reg:
	mov	al,90h
	add	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      xchg_reg_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	xchg_reg_mem_8bit
	cmp	al,2
	je	xchg_reg_mem_16bit
	cmp	al,4
	je	xchg_reg_mem_32bit
	jmp	invalid_operand_size
      xchg_reg_mem_8bit:
	call	store_instruction
	jmp	instruction_assembled
      xchg_reg_mem_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      xchg_reg_mem_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
push_instruction:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	push_reg
	cmp	al,'('
	je	push_imm
	cmp	al,'['
	jne	invalid_operand
      push_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	push_mem_16bit
	cmp	al,4
	je	push_mem_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      push_mem_16bit:
	call	operand_16bit_prefix
	mov	[base_code],0FFh
	mov	[postbyte_register],110b
	call	store_instruction
	jmp	push_done
      push_mem_32bit:
	call	operand_32bit_prefix
	mov	[base_code],0FFh
	mov	[postbyte_register],110b
	call	store_instruction
	jmp	push_done
      push_reg:
	lods	byte [esi]
	cmp	al,60h
	jae	push_sreg
	call	convert_register
	mov	dl,al
	add	dl,50h
	mov	al,[operand_size]
	cmp	al,2
	je	push_reg_16bit
	cmp	al,4
	je	push_reg_32bit
	jmp	invalid_operand_size
      push_reg_16bit:
	call	operand_16bit_prefix
	mov	al,dl
	stos	byte [edi]
	jmp	push_done
      push_reg_32bit:
	call	operand_32bit_prefix
	mov	al,dl
	stos	byte [edi]
	jmp	push_done
      push_sreg:
	cmp	al,70h
	jae	invalid_operand
	sub	al,61h
	cmp	al,4
	jae	push_sreg_386
	shl	al,3
	add	al,6
	stos	byte [edi]
	jmp	push_done
      push_sreg_386:
	sub	al,4
	shl	al,3
	mov	ah,0A0h
	add	ah,al
	mov	al,0Fh
	stos	word [edi]
	jmp	push_done
      push_imm:
	mov	al,[operand_size]
	cmp	al,2
	je	push_imm_16bit
	cmp	al,4
	je	push_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[code_type],16
	je	push_imm_optimized_16bit
      push_imm_optimized_32bit:
	call	get_dword_value
	mov	edx,eax
	cmp	[value_type],0
	jne	push_imm_32bit_forced
	cmp	eax,-80h
	jl	push_imm_32bit_forced
	cmp	eax,80h
	jge	push_imm_32bit_forced
      push_imm_8bit:
	mov	ah,al
	mov	al,6Ah
	stos	word [edi]
	jmp	push_done
      push_imm_optimized_16bit:
	call	get_word_value
	mov	dx,ax
	cmp	[value_type],0
	jne	push_imm_16bit_forced
	cmp	ax,-80h
	jl	push_imm_16bit_forced
	cmp	ax,80h
	jge	push_imm_16bit_forced
	jmp	push_imm_8bit
      push_imm_16bit:
	call	get_word_value
	mov	dx,ax
	call	operand_16bit_prefix
      push_imm_16bit_forced:
	mov	al,68h
	stos	byte [edi]
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	push_done
      push_imm_32bit:
	call	get_dword_value
	mov	edx,eax
	call	operand_32bit_prefix
      push_imm_32bit_forced:
	mov	al,68h
	stos	byte [edi]
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
      push_done:
	lods	byte [esi]
	dec	esi
	cmp	al,80h
	je	instruction_assembled
	cmp	al,81h
	je	instruction_assembled
	or	al,al
	jz	instruction_assembled
	mov	[operand_size],0
	mov	[forced_size],0
	jmp	push_instruction
pop_instruction:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	pop_reg
	cmp	al,'['
	jne	invalid_operand
      pop_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	pop_mem_16bit
	cmp	al,4
	je	pop_mem_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      pop_mem_16bit:
	call	operand_16bit_prefix
	mov	[base_code],08Fh
	mov	[postbyte_register],0
	call	store_instruction
	jmp	pop_done
      pop_mem_32bit:
	call	operand_32bit_prefix
	mov	[base_code],08Fh
	mov	[postbyte_register],0
	call	store_instruction
	jmp	pop_done
      pop_reg:
	lods	byte [esi]
	cmp	al,60h
	jae	pop_sreg
	call	convert_register
	mov	dl,al
	add	dl,58h
	mov	al,[operand_size]
	cmp	al,2
	je	pop_reg_16bit
	cmp	al,4
	je	pop_reg_32bit
	jmp	invalid_operand_size
      pop_reg_16bit:
	call	operand_16bit_prefix
	mov	al,dl
	stos	byte [edi]
	jmp	pop_done
      pop_reg_32bit:
	call	operand_32bit_prefix
	mov	al,dl
	stos	byte [edi]
	jmp	pop_done
      pop_sreg:
	cmp	al,70h
	jae	invalid_operand
	sub	al,61h
	cmp	al,1
	je	illegal_instruction
	cmp	al,4
	jae	pop_sreg_386
	shl	al,3
	add	al,7
	stos	byte [edi]
	jmp	pop_done
      pop_sreg_386:
	sub	al,4
	shl	al,3
	mov	ah,0A1h
	add	ah,al
	mov	al,0Fh
	stos	word [edi]
      pop_done:
	lods	byte [esi]
	dec	esi
	cmp	al,80h
	je	instruction_assembled
	cmp	al,81h
	je	instruction_assembled
	or	al,al
	jz	instruction_assembled
	mov	[operand_size],0
	mov	[forced_size],0
	jmp	pop_instruction
inc_instruction:
	mov	[base_code],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	inc_reg
	cmp	al,'['
	je	inc_mem
	jne	invalid_operand
      inc_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	inc_mem_8bit
	cmp	al,2
	je	inc_mem_16bit
	cmp	al,4
	je	inc_mem_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      inc_mem_8bit:
	mov	al,0FEh
	xchg	al,[base_code]
	mov	[postbyte_register],al
	call	store_instruction
	jmp	instruction_assembled
      inc_mem_16bit:
	call	operand_16bit_prefix
	mov	al,0FFh
	xchg	al,[base_code]
	mov	[postbyte_register],al
	call	store_instruction
	jmp	instruction_assembled
      inc_mem_32bit:
	call	operand_32bit_prefix
	mov	al,0FFh
	xchg	al,[base_code]
	mov	[postbyte_register],al
	call	store_instruction
	jmp	instruction_assembled
      inc_reg:
	lods	byte [esi]
	call	convert_register
	mov	dl,al
	shr	al,4
	mov	al,[operand_size]
	cmp	al,1
	je	inc_reg_8bit
	mov	dh,[base_code]
	shl	dh,3
	add	dl,dh
	add	dl,40h
	cmp	al,2
	je	inc_reg_16bit
	cmp	al,4
	je	inc_reg_32bit
	jmp	invalid_operand_size
      inc_reg_8bit:
	mov	al,0FEh
	mov	ah,[base_code]
	shl	ah,3
	or	ah,dl
	or	ah,11000000b
	stos	word [edi]
	jmp	instruction_assembled
      inc_reg_16bit:
	call	operand_16bit_prefix
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      inc_reg_32bit:
	call	operand_32bit_prefix
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
arpl_instruction:
	mov	[base_code],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	cmp	[operand_size],2
	jne	invalid_operand_size
	mov	[base_code],63h
	call	store_instruction
	jmp	instruction_assembled
bound_instruction:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	bound_16bit
	cmp	al,4
	je	bound_32bit
	jmp	invalid_operand_size
      bound_32bit:
	call	operand_32bit_prefix
	mov	[base_code],62h
	call	store_instruction
	jmp	instruction_assembled
      bound_16bit:
	call	operand_16bit_prefix
	mov	[base_code],62h
	call	store_instruction
	jmp	instruction_assembled
set_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	set_reg
	cmp	al,'['
	jne	invalid_operand
      set_mem:
	call	get_address
	cmp	[operand_size],1
	ja	invalid_operand_size
	mov	[postbyte_register],0
	call	store_instruction
	jmp	instruction_assembled
      set_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	cmp	[operand_size],1
	jne	invalid_operand_size
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,11000000b
	or	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
ret_instruction_16bit:
	mov	ah,al
	call	operand_16bit_prefix
	mov	al,ah
	jmp	ret_instruction
ret_instruction_32bit:
	mov	ah,al
	call	operand_32bit_prefix
	mov	al,ah
ret_instruction:
	mov	[base_code],al
	lods	byte [esi]
	dec	esi
	or	al,al
	jz	simple_ret
	cmp	al,80h
	je	simple_ret
	cmp	al,81h
	je	simple_ret
	call	get_operand_size
	mov	al,[operand_size]
	or	al,al
	jz	ret_imm
	cmp	al,2
	je	ret_imm
	jmp	invalid_operand_size
      ret_imm:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	call	get_word_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	dx,ax
	mov	al,[base_code]
	stos	byte [edi]
	mov	ax,dx
	stos	word [edi]
	jmp	instruction_assembled
      simple_ret:
	mov	al,[base_code]
	inc	al
	stos	byte [edi]
	jmp	instruction_assembled
lea_instruction:
	mov	[base_code],8Dh
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	al,[operand_size]
	push	ax
	mov	[operand_size],0
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	ax
	cmp	al,2
	je	lea_16bit
	cmp	al,4
	je	lea_32bit
	jmp	invalid_operand_size
      lea_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      lea_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
ls_instruction:
	or	al,al
	jz	les_instruction
	cmp	al,3
	jz	lds_instruction
	add	al,0B0h
	mov	[extended_code],al
	mov	[base_code],0Fh
	jmp	ls_code_ok
      les_instruction:
	mov	[base_code],0C4h
	jmp	ls_code_ok
      lds_instruction:
	mov	[base_code],0C5h
      ls_code_ok:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	add	[operand_size],2
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,4
	je	ls_16bit
	cmp	al,6
	je	ls_32bit
	jmp	invalid_operand_size
      ls_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	cmp	[operand_size],0
	je	instruction_assembled
	cmp	[operand_size],4
	jne	invalid_operand_size
	jmp	instruction_assembled
      ls_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	cmp	[operand_size],0
	je	instruction_assembled
	cmp	[operand_size],6
	jne	invalid_operand_size
	jmp	instruction_assembled
enter_instruction:
	call	get_operand_size
	mov	al,[operand_size]
	cmp	al,2
	je	enter_imm16_size_ok
	or	al,al
	jnz	invalid_operand_size
      enter_imm16_size_ok:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	call	get_word_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	push	ax
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	mov	al,[operand_size]
	cmp	al,1
	je	enter_imm8_size_ok
	or	al,al
	jnz	invalid_operand_size
      enter_imm8_size_ok:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	call	get_byte_value
	mov	dl,al
	pop	bx
	mov	al,0C8h
	stos	byte [edi]
	mov	ax,bx
	stos	word [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
sh_instruction:
	mov	[postbyte_register],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	sh_reg
	cmp	al,'['
	jne	invalid_operand
      sh_mem:
	call	get_address
	push	edx bx cx
	mov	al,[operand_size]
	push	ax
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	je	sh_mem_imm
	cmp	al,10h
	jne	invalid_operand
      sh_mem_reg:
	lods	byte [esi]
	cmp	al,11h
	jne	invalid_operand
	pop	ax cx bx edx
	cmp	al,1
	je	sh_mem_cl_8bit
	cmp	al,2
	je	sh_mem_cl_16bit
	cmp	al,4
	je	sh_mem_cl_32bit
	or	ah,ah
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      sh_mem_cl_8bit:
	mov	[base_code],0D2h
	call	store_instruction
	jmp	instruction_assembled
      sh_mem_cl_16bit:
	mov	[base_code],0D3h
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      sh_mem_cl_32bit:
	mov	[base_code],0D3h
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      sh_mem_imm:
	mov	al,[operand_size]
	or	al,al
	jz	sh_mem_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      sh_mem_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	pop	ax cx bx edx
	cmp	al,1
	je	sh_mem_imm_8bit
	cmp	al,2
	je	sh_mem_imm_16bit
	cmp	al,4
	je	sh_mem_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      sh_mem_imm_8bit:
	cmp	byte [value],1
	je	sh_mem_1_8bit
	mov	[base_code],0C0h
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      sh_mem_1_8bit:
	mov	[base_code],0D0h
	call	store_instruction
	jmp	instruction_assembled
      sh_mem_imm_16bit:
	cmp	byte [value],1
	je	sh_mem_1_16bit
	mov	[base_code],0C1h
	call	operand_16bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      sh_mem_1_16bit:
	mov	[base_code],0D1h
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      sh_mem_imm_32bit:
	cmp	byte [value],1
	je	sh_mem_1_32bit
	mov	[base_code],0C1h
	call	operand_32bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      sh_mem_1_32bit:
	mov	[base_code],0D1h
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      sh_reg:
	lods	byte [esi]
	call	convert_register
	shl	[postbyte_register],3
	or	al,11000000b
	or	[postbyte_register],al
	mov	al,[operand_size]
	push	ax
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	je	sh_reg_imm
	cmp	al,10h
	jne	invalid_operand
      sh_reg_reg:
	lods	byte [esi]
	cmp	al,11h
	jne	invalid_operand
	pop	ax
	mov	bl,[postbyte_register]
	cmp	al,1
	je	sh_reg_cl_8bit
	cmp	al,2
	je	sh_reg_cl_16bit
	cmp	al,4
	je	sh_reg_cl_32bit
	jmp	invalid_operand_size
      sh_reg_cl_8bit:
	mov	al,0D2h
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      sh_reg_cl_16bit:
	call	operand_16bit_prefix
	mov	al,0D3h
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      sh_reg_cl_32bit:
	call	operand_32bit_prefix
	mov	al,0D3h
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      sh_reg_imm:
	mov	al,[operand_size]
	or	al,al
	jz	sh_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      sh_reg_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	pop	ax
	mov	bl,[postbyte_register]
	cmp	al,1
	je	sh_reg_imm_8bit
	cmp	al,2
	je	sh_reg_imm_16bit
	cmp	al,4
	je	sh_reg_imm_32bit
	jmp	invalid_operand_size
      sh_reg_imm_8bit:
	cmp	byte [value],1
	je	sh_reg_1_8bit
	mov	al,0C0h
	stos	byte [edi]
	mov	al,bl
	mov	ah,byte [value]
	stos	word [edi]
	jmp	instruction_assembled
      sh_reg_1_8bit:
	mov	al,0D0h
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      sh_reg_imm_16bit:
	cmp	byte [value],1
	je	sh_reg_1_16bit
	call	operand_16bit_prefix
	mov	al,0C1h
	stos	byte [edi]
	mov	al,bl
	mov	ah,byte [value]
	stos	word [edi]
	jmp	instruction_assembled
      sh_reg_1_16bit:
	call	operand_16bit_prefix
	mov	al,0D1h
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      sh_reg_imm_32bit:
	cmp	byte [value],1
	je	sh_reg_1_32bit
	call	operand_32bit_prefix
	mov	al,0C1h
	stos	byte [edi]
	mov	al,bl
	mov	ah,byte [value]
	stos	word [edi]
	jmp	instruction_assembled
      sh_reg_1_32bit:
	call	operand_32bit_prefix
	mov	al,0D1h
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
shd_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	shd_reg
	cmp	al,'['
	jne	invalid_operand
      shd_mem:
	call	get_address
	push	edx bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	xor	al,al
	xchg	al,[operand_size]
	push	ax
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	je	shd_mem_reg_imm
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,11h
	jne	invalid_operand
	call	convert_register
	pop	ax cx bx edx
	cmp	al,2
	je	shd_mem_reg_cl_16bit
	cmp	al,4
	je	shd_mem_reg_cl_32bit
	jmp	invalid_operand_size
      shd_mem_reg_cl_16bit:
	call	operand_16bit_prefix
	inc	[extended_code]
	call	store_instruction
	jmp	instruction_assembled
      shd_mem_reg_cl_32bit:
	call	operand_32bit_prefix
	inc	[extended_code]
	call	store_instruction
	jmp	instruction_assembled
      shd_mem_reg_imm:
	mov	al,[operand_size]
	or	al,al
	jz	shd_mem_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      shd_mem_reg_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	pop	ax cx bx edx
	cmp	al,2
	je	shd_mem_reg_imm_16bit
	cmp	al,4
	je	shd_mem_reg_imm_32bit
	jmp	invalid_operand_size
      shd_mem_reg_imm_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      shd_mem_reg_imm_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      shd_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	al,3
	or	bl,al
	or	bl,11000000b
	mov	al,[operand_size]
	push	ax bx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	je	shd_reg_reg_imm
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,11h
	jne	invalid_operand
	call	convert_register
	pop	bx ax
	cmp	al,2
	je	shd_reg_reg_cl_16bit
	cmp	al,4
	je	shd_reg_reg_cl_32bit
	jmp	invalid_operand_size
      shd_reg_reg_cl_16bit:
	call	operand_16bit_prefix
	jmp	shd_reg_reg_cl_store
      shd_reg_reg_cl_32bit:
	call	operand_32bit_prefix
      shd_reg_reg_cl_store:
	mov	ah,[extended_code]
	inc	ah
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      shd_reg_reg_imm:
	mov	al,[operand_size]
	or	al,al
	jz	shd_reg_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      shd_reg_reg_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	pop	bx ax
	cmp	al,2
	je	shd_reg_reg_imm_16bit
	cmp	al,4
	je	shd_reg_reg_imm_32bit
	jmp	invalid_operand_size
      shd_reg_reg_imm_16bit:
	call	operand_16bit_prefix
	jmp	shd_reg_reg_imm_store
      shd_reg_reg_imm_32bit:
	call	operand_32bit_prefix
      shd_reg_reg_imm_store:
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
movx_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	mov	al,[operand_size]
	cmp	al,2
	je	movx_16bit
	cmp	al,4
	je	movx_32bit
	jmp	invalid_operand_size
      movx_16bit:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	movx_16bit_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	movx_16bit_mem_8bit
	or	al,al
	jnz	invalid_operand_size
      movx_16bit_mem_8bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      movx_16bit_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	cmp	[operand_size],1
	jne	invalid_operand_size
	call	operand_16bit_prefix
	mov	al,0Fh
	stos	byte [edi]
	mov	al,[extended_code]
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      movx_32bit:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	movx_32bit_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	movx_32bit_mem_8bit
	cmp	al,2
	je	movx_32bit_mem_16bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      movx_32bit_mem_8bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      movx_32bit_mem_16bit:
	inc	[extended_code]
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      movx_32bit_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	al,[operand_size]
	cmp	al,1
	je	movx_32bit_reg_8bit
	cmp	al,2
	je	movx_32bit_reg_16bit
	jmp	invalid_operand_size
      movx_32bit_reg_8bit:
	call	operand_32bit_prefix
	mov	al,0Fh
	stos	byte [edi]
	mov	al,[extended_code]
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      movx_32bit_reg_16bit:
	call	operand_32bit_prefix
	mov	al,0Fh
	stos	byte [edi]
	mov	al,[extended_code]
	inc	al
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
bt_instruction:
	mov	[postbyte_register],al
	shl	al,3
	add	al,83h
	mov	[extended_code],al
	mov	[base_code],0Fh
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	bt_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	push	eax bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	cmp	byte [esi],'('
	je	bt_mem_imm
	cmp	byte [esi],11h
	jne	bt_mem_reg
	cmp	byte [esi+2],'('
	je	bt_mem_imm
      bt_mem_reg:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	al,[operand_size]
	cmp	al,2
	je	bt_mem_reg_16bit
	cmp	al,4
	je	bt_mem_reg_32bit
	jmp	invalid_operand_size
      bt_mem_reg_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      bt_mem_reg_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      bt_mem_imm:
	xor	al,al
	xchg	al,[operand_size]
	push	ax
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	mov	al,[operand_size]
	or	al,al
	jz	bt_mem_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      bt_mem_imm_size_ok:
	mov	[extended_code],0BAh
	call	get_byte_value
	mov	byte [value],al
	pop	ax
	cmp	al,2
	je	bt_mem_imm_16bit
	cmp	al,4
	je	bt_mem_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
	jmp	bt_mem_imm_32bit
      bt_mem_imm_16bit:
	call	operand_16bit_prefix
	pop	cx bx edx
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      bt_mem_imm_32bit:
	call	operand_32bit_prefix
	pop	cx bx edx
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      bt_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	cmp	byte [esi],'('
	je	bt_reg_imm
	cmp	byte [esi],11h
	jne	bt_reg_reg
	cmp	byte [esi+2],'('
	je	bt_reg_imm
      bt_reg_reg:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	al,3
	or	bl,al
	or	bl,11000000b
	mov	al,[operand_size]
	cmp	al,2
	je	bt_reg_reg_16bit
	cmp	al,4
	je	bt_reg_reg_32bit
	jmp	invalid_operand_size
      bt_reg_reg_16bit:
	call	operand_16bit_prefix
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      bt_reg_reg_32bit:
	call	operand_32bit_prefix
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      bt_reg_imm:
	xor	al,al
	xchg	al,[operand_size]
	push	ax
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	mov	al,[operand_size]
	or	al,al
	jz	bt_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      bt_reg_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	pop	ax
	cmp	al,2
	je	bt_reg_imm_16bit
	cmp	al,4
	je	bt_reg_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
	jmp	bt_reg_imm_32bit
      bt_reg_imm_16bit:
	call	operand_16bit_prefix
	jmp	bt_reg_imm_store
      bt_reg_imm_32bit:
	call	operand_32bit_prefix
      bt_reg_imm_store:
	mov	ax,0BA0Fh
	stos	word [edi]
	mov	al,11000000b
	or	al,[postbyte_register]
	mov	ah,[extended_code]
	sub	ah,83h
	or	al,ah
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
bs_instruction:
	mov	[extended_code],al
	mov	[base_code],0Fh
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,10h
	je	bs_reg_reg
	cmp	al,'['
	jne	invalid_argument
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	bs_reg_mem_16bit
	cmp	al,4
	je	bs_reg_mem_32bit
	jmp	invalid_operand_size
      bs_reg_mem_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      bs_reg_mem_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      bs_reg_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	al,[operand_size]
	cmp	al,2
	je	bs_reg_reg_16bit
	cmp	al,4
	je	bs_reg_reg_32bit
	jmp	invalid_operand_size
      bs_reg_reg_16bit:
	call	operand_16bit_prefix
	jmp	bs_reg_reg_store
      bs_reg_reg_32bit:
	call	operand_32bit_prefix
      bs_reg_reg_store:
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled

pm_word_instruction:
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[base_code],0Fh
	mov	[extended_code],ah
	mov	[postbyte_register],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	pm_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	.store
	or	al,al
	jnz	invalid_operand_size
      .store:
	call	store_instruction
	jmp	instruction_assembled
      pm_reg:
	lods	byte [esi]
	call	convert_register
	cmp	[operand_size],2
	jne	invalid_operand_size
	mov	bl,al
	mov	al,0Fh
	mov	ah,[extended_code]
	stos	word [edi]
	mov	al,[postbyte_register]
	shl	al,3
	or	al,bl
	or	al,11000000b
	stos	byte [edi]
	jmp	instruction_assembled
pm_pword_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],1
	mov	[postbyte_register],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,6
	je	.store
	or	al,al
	jnz	invalid_operand_size
      .store:
	call	store_instruction
	jmp	instruction_assembled

imul_instruction:
	mov	[base_code],0F6h
	mov	[postbyte_register],5
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	imul_reg
	cmp	al,'['
	jne	invalid_operand
      imul_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	imul_mem_8bit
	cmp	al,2
	je	imul_mem_16bit
	cmp	al,4
	je	imul_mem_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      imul_mem_8bit:
	call	store_instruction
	jmp	instruction_assembled
      imul_mem_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      imul_mem_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      imul_reg:
	lods	byte [esi]
	call	convert_register
	cmp	byte [esi],','
	je	imul_reg_
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	ah,[operand_size]
	cmp	ah,1
	je	imul_reg_8bit
	cmp	ah,2
	je	imul_reg_16bit
	cmp	ah,4
	je	imul_reg_32bit
	jmp	invalid_operand_size
      imul_reg_8bit:
	mov	ah,bl
	mov	al,0F6h
	stos	word [edi]
	jmp	instruction_assembled
      imul_reg_16bit:
	call	operand_16bit_prefix
	mov	ah,bl
	mov	al,0F7h
	stos	word [edi]
	jmp	instruction_assembled
      imul_reg_32bit:
	call	operand_32bit_prefix
	mov	ah,bl
	mov	al,0F7h
	stos	word [edi]
	jmp	instruction_assembled
      imul_reg_:
	mov	[postbyte_register],al
	inc	esi
	cmp	byte [esi],'('
	je	imul_reg_imm
	cmp	byte [esi],11h
	jne	imul_reg__
	cmp	byte [esi+2],'('
	je	imul_reg_imm
      imul_reg__:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	imul_reg_reg
	cmp	al,'['
	je	imul_reg_mem
	jne	invalid_operand
      imul_reg_mem:
	call	get_address
	push	edx bx cx
	cmp	byte [esi],','
	je	imul_reg_mem_imm
	mov	al,[operand_size]
	cmp	al,2
	je	imul_reg_mem_16bit
	cmp	al,4
	je	imul_reg_mem_32bit
	jmp	invalid_operand_size
      imul_reg_mem_16bit:
	call	operand_16bit_prefix
	jmp	imul_reg_mem_store
      imul_reg_mem_32bit:
	call	operand_32bit_prefix
      imul_reg_mem_store:
	pop	cx bx edx
	mov	[base_code],0Fh
	mov	[extended_code],0AFh
	call	store_instruction
	jmp	instruction_assembled
      imul_reg_mem_imm:
	inc	esi
	xor	cl,cl
	xchg	cl,[operand_size]
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	mov	al,[operand_size]
	mov	[operand_size],cl
	cmp	al,1
	je	imul_reg_mem_imm_8bit
	cmp	al,2
	je	imul_reg_mem_imm_16bit
	cmp	al,4
	je	imul_reg_mem_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	cl,2
	je	imul_reg_mem_imm_16bit
	cmp	cl,4
	je	imul_reg_mem_imm_32bit
	jmp	invalid_operand_size
      imul_reg_mem_imm_8bit:
	call	get_byte_value
	mov	byte [value],al
	pop	cx bx edx
	mov	[base_code],6Bh
	cmp	[operand_size],2
	je	imul_reg_mem_16bit_imm_8bit
	cmp	[operand_size],4
	je	imul_reg_mem_32bit_imm_8bit
	jmp	invalid_operand_size
      imul_reg_mem_16bit_imm_8bit:
	call	operand_16bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      imul_reg_mem_32bit_imm_8bit:
	call	operand_32bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      imul_reg_mem_imm_16bit:
	call	get_word_value
	mov	word [value],ax
	pop	cx bx edx
	mov	[base_code],69h
	cmp	[operand_size],2
	jne	invalid_operand_size
	call	operand_16bit_prefix
	call	store_instruction
	mov	ax,word [value]
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      imul_reg_mem_imm_32bit:
	call	get_dword_value
	mov	dword [value],eax
	pop	cx bx edx
	mov	[base_code],69h
	cmp	[operand_size],4
	jne	invalid_operand_size
	call	operand_32bit_prefix
	call	store_instruction
	mov	eax,dword [value]
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      imul_reg_imm:
	mov	dl,[postbyte_register]
	mov	bl,dl
	dec	esi
	jmp	imul_reg_reg_imm
      imul_reg_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	mov	dl,al
	cmp	byte [esi],','
	je	imul_reg_reg_imm
	mov	al,[operand_size]
	cmp	al,2
	je	imul_reg_reg_16bit
	cmp	al,4
	je	imul_reg_reg_32bit
	jmp	invalid_operand_size
      imul_reg_reg_16bit:
	call	operand_16bit_prefix
	jmp	imul_reg_reg_store
      imul_reg_reg_32bit:
	call	operand_32bit_prefix
      imul_reg_reg_store:
	mov	ax,0AF0Fh
	stos	word [edi]
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stos	byte [edi]
	jmp	instruction_assembled
      imul_reg_reg_imm:
	inc	esi
	xor	cl,cl
	xchg	cl,[operand_size]
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	mov	al,[operand_size]
	mov	[operand_size],cl
	cmp	al,1
	je	imul_reg_reg_imm_8bit
	cmp	al,2
	je	imul_reg_reg_imm_16bit
	cmp	al,4
	je	imul_reg_reg_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	cl,2
	je	imul_reg_reg_imm_16bit
	cmp	cl,4
	je	imul_reg_reg_imm_32bit
	jmp	invalid_operand_size
      imul_reg_reg_imm_8bit:
	push	bx dx
	call	get_byte_value
	pop	dx bx
      imul_reg_reg_imm_8bit_store:
	mov	byte [value],al
	cmp	[operand_size],2
	je	imul_reg_reg_16bit_imm_8bit
	cmp	[operand_size],4
	je	imul_reg_reg_32bit_imm_8bit
	jmp	invalid_operand_size
      imul_reg_reg_16bit_imm_8bit:
	call	operand_16bit_prefix
	mov	al,6Bh
	stos	byte [edi]
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      imul_reg_reg_32bit_imm_8bit:
	call	operand_32bit_prefix
	mov	al,6Bh
	stos	byte [edi]
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      imul_reg_reg_imm_16bit:
	push	bx dx
	call	get_word_value
	pop	dx bx
	cmp	[value_type],0
	jne	imul_reg_reg_imm_16bit_forced
	cmp	ax,-80h
	jl	imul_reg_reg_imm_16bit_forced
	cmp	ax,80h
	jl	imul_reg_reg_imm_8bit_store
      imul_reg_reg_imm_16bit_forced:
	mov	word [value],ax
	call	operand_16bit_prefix
	mov	al,69h
	stos	byte [edi]
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stos	byte [edi]
	mov	ax,word [value]
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      imul_reg_reg_imm_32bit:
	push	bx dx
	call	get_dword_value
	pop	dx bx
	cmp	[value_type],0
	jne	imul_reg_reg_imm_32bit_forced
	cmp	ax,-80h
	jl	imul_reg_reg_imm_32bit_forced
	cmp	ax,80h
	jl	imul_reg_reg_imm_8bit_store
      imul_reg_reg_imm_32bit_forced:
	mov	dword [value],eax
	call	operand_32bit_prefix
	mov	al,69h
	stos	byte [edi]
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stos	byte [edi]
	mov	eax,dword [value]
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
in_instruction:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	or	al,al
	jnz	invalid_operand
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	al,[operand_size]
	push	ax
	mov	[operand_size],0
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	je	in_imm
	cmp	al,10h
	je	in_reg
	jmp	invalid_operand
      in_reg:
	lods	byte [esi]
	cmp	al,22h
	jne	invalid_operand
	call	convert_register
	pop	ax
	cmp	al,1
	je	in_al_dx
	cmp	al,2
	je	in_ax_dx
	cmp	al,4
	je	in_eax_dx
	jmp	invalid_operand_size
      in_al_dx:
	mov	al,0ECh
	stos	byte [edi]
	jmp	instruction_assembled
      in_ax_dx:
	call	operand_16bit_prefix
	mov	al,0EDh
	stos	byte [edi]
	jmp	instruction_assembled
      in_eax_dx:
	call	operand_32bit_prefix
	mov	al,0EDh
	stos	byte [edi]
	jmp	instruction_assembled
      in_imm:
	mov	al,[operand_size]
	or	al,al
	jz	in_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      in_imm_size_ok:
	call	get_byte_value
	mov	dl,al
	pop	ax
	cmp	al,1
	je	in_al_imm
	cmp	al,2
	je	in_ax_imm
	cmp	al,4
	je	in_eax_imm
	jmp	invalid_operand_size
      in_al_imm:
	mov	al,0E4h
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      in_ax_imm:
	call	operand_16bit_prefix
	mov	al,0E5h
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      in_eax_imm:
	call	operand_32bit_prefix
	mov	al,0E5h
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
out_instruction:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	je	out_imm
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,22h
	jne	invalid_operand
	call	convert_register
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	or	al,al
	jnz	invalid_operand
	mov	al,[operand_size]
	cmp	al,1
	je	out_dx_al
	cmp	al,2
	je	out_dx_ax
	cmp	al,4
	je	out_dx_eax
	jmp	invalid_operand_size
      out_dx_al:
	mov	al,0EEh
	stos	byte [edi]
	jmp	instruction_assembled
      out_dx_ax:
	call	operand_16bit_prefix
	mov	al,0EFh
	stos	byte [edi]
	jmp	instruction_assembled
      out_dx_eax:
	call	operand_32bit_prefix
	mov	al,0EFh
	stos	byte [edi]
	jmp	instruction_assembled
      out_imm:
	mov	al,[operand_size]
	or	al,al
	jz	out_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      out_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	or	al,al
	jnz	invalid_operand
	mov	al,[operand_size]
	cmp	al,1
	je	out_imm_al
	cmp	al,2
	je	out_imm_ax
	cmp	al,4
	je	out_imm_eax
	jmp	invalid_operand_size
      out_imm_al:
	mov	al,0E6h
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      out_imm_ax:
	call	operand_16bit_prefix
	mov	al,0E7h
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      out_imm_eax:
	call	operand_32bit_prefix
	mov	al,0E7h
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
lar_instruction:
	mov	[extended_code],al
	mov	[base_code],0Fh
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	lar_reg_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	lar_16bit
	cmp	al,4
	je	lar_32bit
	jmp	invalid_operand_size
      lar_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      lar_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      lar_reg_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	al,[operand_size]
	cmp	al,2
	je	lar_reg_reg_16bit
	cmp	al,4
	je	lar_reg_reg_32bit
	jmp	invalid_operand_size
      lar_reg_reg_32bit:
	call	operand_32bit_prefix
	jmp	lar_reg_reg_store
      lar_reg_reg_16bit:
	call	operand_16bit_prefix
      lar_reg_reg_store:
	mov	al,0Fh
	mov	ah,[extended_code]
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
invlpg_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],1
	mov	[postbyte_register],7
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	call	store_instruction
	jmp	instruction_assembled
basic_486_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	basic_486_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	push	edx bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	al,[operand_size]
	cmp	al,1
	je	basic_486_mem_reg_8bit
	cmp	al,2
	je	basic_486_mem_reg_16bit
	cmp	al,4
	je	basic_486_mem_reg_32bit
	jmp	invalid_operand_size
      basic_486_mem_reg_8bit:
	call	store_instruction
	jmp	instruction_assembled
      basic_486_mem_reg_16bit:
	call	operand_16bit_prefix
	inc	[extended_code]
	call	store_instruction
	jmp	instruction_assembled
      basic_486_mem_reg_32bit:
	call	operand_32bit_prefix
	inc	[extended_code]
	call	store_instruction
	jmp	instruction_assembled
      basic_486_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	al,3
	or	bl,al
	or	bl,11000000b
	mov	al,[operand_size]
	cmp	al,1
	je	basic_486_reg_reg_8bit
	cmp	al,2
	je	basic_486_reg_reg_16bit
	cmp	al,4
	je	basic_486_reg_reg_32bit
	jmp	invalid_operand_size
      basic_486_reg_reg_32bit:
	call	operand_32bit_prefix
	inc	[extended_code]
	jmp	basic_486_reg_reg_8bit
      basic_486_reg_reg_16bit:
	call	operand_16bit_prefix
	inc	[extended_code]
      basic_486_reg_reg_8bit:
	mov	al,0Fh
	mov	ah,[extended_code]
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
bswap_instruction:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	ah,al
	add	ah,0C8h
	cmp	[operand_size],4
	jne	invalid_operand_size
	call	operand_32bit_prefix
	mov	al,0Fh
	stos	word [edi]
	jmp	instruction_assembled
cmpxchg8b_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0C7h
	mov	[postbyte_register],1
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,8
	je	.store
	or	al,al
	jnz	invalid_operand_size
      .store:
	call	store_instruction
	jmp	instruction_assembled

conditional_jump:
	mov	[base_code],al
	call	get_jump_operator
	call	get_operand_size
	cmp	[jump_type],2
	je	invalid_operand
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],1
	je	invalid_use_of_symbol
	sub	eax,edi
	add	eax,[org_start]
	sub	eax,2
	mov	bl,[operand_size]
	cmp	bl,1
	je	conditional_jump_8bit
	cmp	bl,2
	je	conditional_jump_16bit
	cmp	bl,4
	je	conditional_jump_32bit
	or	bl,bl
	jnz	invalid_operand_size
	cmp	eax,80h
	jb	conditional_jump_8bit
	cmp	eax,-80h
	jae	conditional_jump_8bit
	cmp	[code_type],16
	je	conditional_jump_16bit
      conditional_jump_32bit:
	sub	eax,4
	mov	edx,eax
	mov	ecx,edi
	call	operand_32bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	ah,[base_code]
	add	ah,10h
	mov	al,0Fh
	stos	word [edi]
	mov	eax,edx
	stos	dword [edi]
	jmp	instruction_assembled
      conditional_jump_16bit:
	sub	eax,2
	mov	edx,eax
	mov	ecx,edi
	call	operand_16bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	ah,[base_code]
	add	ah,10h
	mov	al,0Fh
	stos	word [edi]
	mov	eax,edx
	stos	word [edi]
	cmp	eax,10000h
	jge	jump_out_of_range
	cmp	eax,-8000h
	jl	jump_out_of_range
	jmp	instruction_assembled
      conditional_jump_8bit:
	mov	edx,eax
	mov	ah,al
	mov	al,[base_code]
	stos	word [edi]
	cmp	edx,80h
	jge	jump_out_of_range
	cmp	edx,-80h
	jl	jump_out_of_range
	jmp	instruction_assembled
      jump_out_of_range:
	cmp	[error_line],0
	jne	instruction_assembled
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	eax,[home_line]
	mov	[error_home_line],eax
	mov	[error],relative_jump_out_of_range
	jmp	instruction_assembled
loop_instruction_16bit:
	mov	cl,al
	call	address_16bit_prefix
	mov	al,cl
	jmp	loop_instruction
loop_instruction_32bit:
	mov	cl,al
	call	address_32bit_prefix
	mov	al,cl
loop_instruction:
	mov	[base_code],al
	call	get_jump_operator
	call	get_operand_size
	cmp	[jump_type],2
	je	invalid_operand
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],1
	je	invalid_use_of_symbol
	sub	eax,edi
	add	eax,[org_start]
	mov	bl,[operand_size]
	cmp	bl,1
	je	loop_8bit
	or	bl,bl
	jnz	invalid_operand_size
      loop_8bit:
	sub	eax,2
	mov	edx,eax
	mov	al,[base_code]
	stos	byte [edi]
	mov	eax,edx
	stos	byte [edi]
	cmp	eax,80h
	jge	jump_out_of_range
	cmp	eax,-80h
	jl	jump_out_of_range
	jmp	instruction_assembled
call_instruction:
	mov	[postbyte_register],10b
	mov	[base_code],0E8h
	mov	[extended_code],9Ah
	jmp	process_jmp
jmp_instruction:
	mov	[postbyte_register],100b
	mov	[base_code],0E9h
	mov	[extended_code],0EAh
      process_jmp:
	call	get_jump_operator
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	jmp_reg
	cmp	al,'('
	je	jmp_imm
	cmp	al,'['
	jne	invalid_operand
      jmp_mem:
	call	get_address
	mov	[base_code],0FFh
	mov	edx,eax
	mov	al,[operand_size]
	or	al,al
	jz	jmp_mem_size_not_specified
	cmp	al,2
	je	jmp_mem_16bit
	cmp	al,4
	je	jmp_mem_32bit
	cmp	al,6
	je	jmp_mem_48bit
	jmp	invalid_operand_size
      jmp_mem_size_not_specified:
	cmp	[jump_type],2
	je	jmp_mem_far
	cmp	[jump_type],1
	je	jmp_mem_near
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      jmp_mem_near:
	cmp	[code_type],16
	je	jmp_mem_16bit
	jmp	jmp_mem_near_32bit
      jmp_mem_far:
	cmp	[code_type],16
	je	jmp_mem_far_32bit
      jmp_mem_48bit:
	cmp	[jump_type],1
	je	invalid_operand_size
	call	operand_32bit_prefix
	inc	[postbyte_register]
	call	store_instruction
	jmp	instruction_assembled
      jmp_mem_32bit:
	cmp	[jump_type],2
	je	jmp_mem_far_32bit
	cmp	[jump_type],1
	je	jmp_mem_near_32bit
	cmp	[code_type],16
	je	jmp_mem_far_32bit
      jmp_mem_near_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      jmp_mem_far_32bit:
	call	operand_16bit_prefix
	inc	[postbyte_register]
	call	store_instruction
	jmp	instruction_assembled
      jmp_mem_16bit:
	cmp	[jump_type],2
	je	invalid_operand_size
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      jmp_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	or	bl,11000000b
	mov	al,[operand_size]
	cmp	al,2
	je	jmp_reg_16bit
	cmp	al,4
	je	jmp_reg_32bit
	jmp	invalid_operand_size
      jmp_reg_32bit:
	cmp	[jump_type],2
	je	jmp_reg_far32bit
	cmp	[jump_type],1
	je	jmp_reg_near32bit
	cmp	[code_type],16
	je	jmp_reg_far32bit
      jmp_reg_near32bit:
	call	operand_32bit_prefix
	mov	al,[postbyte_register]
	shl	al,3
	or	bl,al
	mov	ah,bl
	mov	al,0FFh
	stos	word [edi]
	jmp	instruction_assembled
      jmp_reg_far32bit:
	call	operand_32bit_prefix
	mov	al,[postbyte_register]
	inc	al
	shl	al,3
	or	bl,al
	mov	ah,bl
	mov	al,0FFh
	stos	word [edi]
	jmp	instruction_assembled
      jmp_reg_16bit:
	cmp	[jump_type],2
	je	invalid_operand_size
	call	operand_16bit_prefix
	mov	al,[postbyte_register]
	shl	al,3
	or	bl,al
	mov	ah,bl
	mov	al,0FFh
	stos	word [edi]
	jmp	instruction_assembled
      jmp_imm:
	push	esi
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	pop	ebx
	cmp	byte [esi],':'
	je	jmp_far
	cmp	[value_type],1
	je	invalid_use_of_symbol
	cmp	[jump_type],2
	je	invalid_operand
	sub	eax,edi
	add	eax,[org_start]
	sub	eax,2
	mov	bl,[operand_size]
	cmp	bl,1
	je	jmp_8bit
	cmp	bl,2
	je	jmp_16bit
	cmp	bl,4
	je	jmp_32bit
	or	bl,bl
	jnz	invalid_operand_size
	cmp	[base_code],0E9h
	jne	jmp_no8bit
	cmp	eax,80h
	jb	jmp_8bit
	cmp	eax,-80h
	jae	jmp_8bit
      jmp_no8bit:
	cmp	[code_type],32
	je	jmp_32bit
      jmp_16bit:
	dec	eax
	mov	edx,eax
	mov	ecx,edi
	call	operand_16bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	al,[base_code]
	stos	byte [edi]
	mov	eax,edx
	stos	word [edi]
	cmp	eax,10000h
	jge	jump_out_of_range
	cmp	eax,-8000h
	jl	jump_out_of_range
	jmp	instruction_assembled
      jmp_32bit:
	sub	eax,3
	mov	edx,eax
	mov	ecx,edi
	call	operand_32bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	al,[base_code]
	stos	byte [edi]
	mov	eax,edx
	stos	dword [edi]
	jmp	instruction_assembled
      jmp_8bit:
	cmp	[base_code],0E9h
	jne	invalid_operand_size
	mov	edx,eax
	mov	ah,al
	mov	al,0EBh
	stos	word [edi]
	cmp	edx,80h
	jge	jump_out_of_range
	cmp	edx,-80h
	jl	jump_out_of_range
	jmp	instruction_assembled
      jmp_far:
	cmp	[jump_type],1
	je	invalid_operand
	mov	esi,ebx
	call	get_word_value
	mov	dx,ax
	mov	bl,[operand_size]
	cmp	bl,2
	je	jmp_far_size_ok
	or	bl,bl
	jnz	invalid_operand_size
      jmp_far_size_ok:
	mov	[operand_size],0
	inc	esi
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	mov	al,[value_type]
	push	ax
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],1
	je	invalid_use_of_symbol
	mov	ebx,eax
	pop	ax
	mov	[value_type],al
	mov	cl,[operand_size]
	cmp	cl,2
	je	jmp_far_16bit
	cmp	cl,4
	je	jmp_far_32bit
	or	cl,cl
	jnz	invalid_operand_size
	cmp	[code_type],32
	je	jmp_far_32bit
      jmp_far_16bit:
	call	operand_16bit_prefix
	mov	al,[extended_code]
	stos	byte [edi]
	mov	ax,bx
	stos	word [edi]
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      jmp_far_32bit:
	call	operand_32bit_prefix
	mov	al,[extended_code]
	stos	byte [edi]
	mov	eax,ebx
	stos	dword [edi]
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled

ins_instruction:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
	jnz	invalid_address
	cmp	bh,27h
	je	ins_16bit
	cmp	bh,47h
	jne	invalid_address
	call	address_32bit_prefix
	jmp	ins_store
      ins_16bit:
	call	address_16bit_prefix
      ins_store:
	cmp	[segment_register],1
	ja	invalid_address
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,22h
	jne	invalid_operand
	mov	al,6Ch
	cmp	[operand_size],1
	je	simple_instruction
	inc	al
	cmp	[operand_size],2
	je	simple_instruction_16bit
	cmp	[operand_size],4
	je	simple_instruction_32bit
	cmp	[operand_size],0
	je	operand_size_not_specified
	jmp	invalid_operand_size
outs_instruction:
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,22h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
	jnz	invalid_address
	cmp	bh,26h
	je	outs_16bit
	cmp	bh,46h
	jne	invalid_address
	call	address_32bit_prefix
	jmp	outs_store
      outs_16bit:
	call	address_16bit_prefix
      outs_store:
	cmp	[segment_register],4
	je	outs_segment_ok
	call	store_segment_prefix
      outs_segment_ok:
	mov	al,6Eh
	cmp	[operand_size],1
	je	simple_instruction
	inc	al
	cmp	[operand_size],2
	je	simple_instruction_16bit
	cmp	[operand_size],4
	je	simple_instruction_32bit
	cmp	[operand_size],0
	je	operand_size_not_specified
	jmp	invalid_operand_size
movs_instruction:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
	jnz	invalid_address
	cmp	[segment_register],1
	ja	invalid_address
	push	bx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	dx
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
	jnz	invalid_address
	mov	al,dh
	mov	ah,bh
	shr	al,4
	shr	ah,4
	cmp	al,ah
	jne	address_sizes_do_not_agree
	and	bh,111b
	and	dh,111b
	cmp	bh,6
	jne	invalid_address
	cmp	dh,7
	jne	invalid_address
	cmp	al,2
	je	movs_16bit
	cmp	al,4
	jne	invalid_address
	call	address_32bit_prefix
	jmp	movs_store
      movs_16bit:
	call	address_16bit_prefix
      movs_store:
	cmp	[segment_register],4
	je	movs_segment_ok
	call	store_segment_prefix
      movs_segment_ok:
	mov	al,0A4h
	mov	bl,[operand_size]
	cmp	bl,1
	je	simple_instruction
	inc	al
	cmp	bl,2
	je	simple_instruction_16bit
	cmp	bl,4
	je	simple_instruction_32bit
	jmp	invalid_operand_size
lods_instruction:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
	jnz	invalid_address
	cmp	bh,26h
	je	lods_16bit
	cmp	bh,46h
	jne	invalid_address
	call	address_32bit_prefix
	jmp	lods_store
      lods_16bit:
	call	address_16bit_prefix
      lods_store:
	cmp	[segment_register],4
	je	lods_segment_ok
	call	store_segment_prefix
      lods_segment_ok:
	mov	al,0ACh
	cmp	[operand_size],1
	je	simple_instruction
	inc	al
	cmp	[operand_size],2
	je	simple_instruction_16bit
	cmp	[operand_size],4
	je	simple_instruction_32bit
	cmp	[operand_size],0
	je	operand_size_not_specified
	jmp	invalid_operand_size
stos_instruction:
	mov	[base_code],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
	jnz	invalid_address
	cmp	bh,27h
	je	stos_16bit
	cmp	bh,47h
	jne	invalid_address
	call	address_32bit_prefix
	jmp	stos_store
      stos_16bit:
	call	address_16bit_prefix
      stos_store:
	cmp	[segment_register],1
	ja	invalid_address
	mov	al,[base_code]
	cmp	[operand_size],1
	je	simple_instruction
	inc	al
	cmp	[operand_size],2
	je	simple_instruction_16bit
	cmp	[operand_size],4
	je	simple_instruction_32bit
	cmp	[operand_size],0
	je	operand_size_not_specified
	jmp	invalid_operand_size
cmps_instruction:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
	jnz	invalid_address
	mov	al,[segment_register]
	push	ax bx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
	jnz	invalid_address
	pop	dx ax
	cmp	[segment_register],1
	ja	invalid_address
	mov	[segment_register],al
	mov	al,dh
	mov	ah,bh
	shr	al,4
	shr	ah,4
	cmp	al,ah
	jne	address_sizes_do_not_agree
	and	bh,111b
	and	dh,111b
	cmp	bh,7
	jne	invalid_address
	cmp	dh,6
	jne	invalid_address
	cmp	al,2
	je	cmps_16bit
	cmp	al,4
	jne	invalid_address
	call	address_32bit_prefix
	jmp	cmps_store
      cmps_16bit:
	call	address_16bit_prefix
      cmps_store:
	cmp	[segment_register],4
	je	cmps_segment_ok
	call	store_segment_prefix
      cmps_segment_ok:
	mov	al,0A6h
	mov	bl,[operand_size]
	cmp	bl,1
	je	simple_instruction
	inc	al
	cmp	bl,2
	je	simple_instruction_16bit
	cmp	bl,4
	je	simple_instruction_32bit
	jmp	invalid_operand_size
xlat_instruction:
	call	get_operand_size
	cmp	byte [esi],'['
	jne	xlatb_instruction
	inc	esi
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
	jnz	invalid_address
	cmp	bh,23h
	je	xlat_16bit
	cmp	bh,43h
	jne	invalid_address
	call	address_32bit_prefix
	jmp	xlat_store
      xlat_16bit:
	call	address_16bit_prefix
      xlat_store:
	call	store_segment_prefix
	mov	al,0D7h
	cmp	[operand_size],1
	jbe	simple_instruction
	jmp	invalid_operand_size
      xlatb_instruction:
	cmp	[operand_size],0
	jne	invalid_operand
	mov	al,0D7h
	jmp	simple_instruction

basic_fpu_instruction:
	mov	[postbyte_register],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,14h
	je	basic_fpu_freg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,4
	je	basic_fpu_mem_32bit
	cmp	al,8
	je	basic_fpu_mem_64bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      basic_fpu_mem_32bit:
	mov	[base_code],0D8h
	call	store_instruction
	jmp	instruction_assembled
      basic_fpu_mem_64bit:
	mov	[base_code],0DCh
	call	store_instruction
	jmp	instruction_assembled
      basic_fpu_freg:
	cmp	[operand_size],0
	jne	invalid_operand
	lods	byte [esi]
	mov	ah,[postbyte_register]
	cmp	ah,2
	je	basic_fpu_single_freg
	cmp	ah,3
	je	basic_fpu_single_freg
	or	al,al
	jz	basic_fpu_st0
	shl	ah,3
	or	al,ah
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,14h
	jne	invalid_operand
	lods	byte [esi]
	or	al,al
	jnz	invalid_operand
	mov	ah,[postbyte_register]
	or	ah,11000000b
	mov	al,0DCh
	stos	word [edi]
	jmp	instruction_assembled
      basic_fpu_st0:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,14h
	jne	invalid_operand
	lods	byte [esi]
	mov	ah,[postbyte_register]
	shl	ah,3
	or	ah,al
	or	ah,11000000b
	mov	al,0D8h
	stos	word [edi]
	jmp	instruction_assembled
      basic_fpu_single_freg:
	shl	ah,3
	or	ah,al
	or	ah,11000000b
	mov	al,0D8h
	stos	word [edi]
	jmp	instruction_assembled
simple_fpu_instruction:
	mov	ah,al
	or	ah,11000000b
	mov	al,0D9h
	stos	word [edi]
	jmp	instruction_assembled
fi_instruction:
	mov	[postbyte_register],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	fi_mem_16bit
	cmp	al,4
	je	fi_mem_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      fi_mem_32bit:
	mov	[base_code],0DAh
	call	store_instruction
	jmp	instruction_assembled
      fi_mem_16bit:
	mov	[base_code],0DEh
	call	store_instruction
	jmp	instruction_assembled
fld_instruction:
	mov	[postbyte_register],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,14h
	je	fld_freg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,4
	je	fld_mem_32bit
	cmp	al,8
	je	fld_mem_64bit
	cmp	al,10
	je	fld_mem_80bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      fld_mem_32bit:
	mov	[base_code],0D9h
	call	store_instruction
	jmp	instruction_assembled
      fld_mem_64bit:
	mov	[base_code],0DDh
	call	store_instruction
	jmp	instruction_assembled
      fld_mem_80bit:
	mov	al,[postbyte_register]
	cmp	al,0
	je	.store
	dec	[postbyte_register]
	cmp	al,3
	je	.store
	jmp	invalid_operand_size
      .store:
	add	[postbyte_register],5
	mov	[base_code],0DBh
	call	store_instruction
	jmp	instruction_assembled
      fld_freg:
	cmp	[operand_size],0
	jne	invalid_operand
	lods	byte [esi]
	mov	ah,[postbyte_register]
	shl	ah,3
	or	ah,al
	or	ah,11000000b
	cmp	[postbyte_register],2
	jae	fst_freg
	mov	al,0D9h
	stos	word [edi]
	jmp	instruction_assembled
      fst_freg:
	mov	al,0DDh
	stos	word [edi]
	jmp	instruction_assembled
fild_instruction:
	mov	[postbyte_register],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	fild_mem_16bit
	cmp	al,4
	je	fild_mem_32bit
	cmp	al,8
	je	fild_mem_64bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      fild_mem_32bit:
	mov	[base_code],0DBh
	call	store_instruction
	jmp	instruction_assembled
      fild_mem_16bit:
	mov	[base_code],0DFh
	call	store_instruction
	jmp	instruction_assembled
      fild_mem_64bit:
	mov	al,[postbyte_register]
	cmp	al,0
	je	.store
	dec	[postbyte_register]
	cmp	al,3
	je	.store
	jmp	invalid_operand_size
      .store:
	add	[postbyte_register],5
	mov	[base_code],0DFh
	call	store_instruction
	jmp	instruction_assembled
fbld_instruction:
	mov	[postbyte_register],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	fbld_mem_80bit
	cmp	al,10
	je	fbld_mem_80bit
	jmp	invalid_operand_size
      fbld_mem_80bit:
	mov	[base_code],0DFh
	call	store_instruction
	jmp	instruction_assembled
faddp_instruction:
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,14h
	jne	invalid_operand
	lods	byte [esi]
	mov	ah,[postbyte_register]
	shl	ah,3
	or	al,ah
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,14h
	jne	invalid_operand
	lods	byte [esi]
	or	al,al
	jnz	invalid_operand
	mov	ah,[postbyte_register]
	or	ah,11000000b
	mov	al,0DEh
	stos	word [edi]
	jmp	instruction_assembled
fcompp_instruction:
	mov	ax,0D9DEh
	stos	word [edi]
	jmp	instruction_assembled
fxch_instruction:
	mov	[base_code],0D9h
	mov	[postbyte_register],1
	jmp	fpu_streg
ffree_instruction:
	mov	[base_code],0DDh
	mov	[postbyte_register],al
      fpu_streg:
	lods	byte [esi]
	cmp	al,14h
	jne	invalid_operand
	lods	byte [esi]
	mov	ah,[postbyte_register]
	shl	ah,3
	or	ah,al
	or	ah,11000000b
	mov	al,[base_code]
	stos	word [edi]
	jmp	instruction_assembled
fldenv_instruction:
	mov	[base_code],0D9h
	jmp	fpu_mem
fsave_instruction:
	mov	[base_code],0DDh
      fpu_mem:
	mov	[postbyte_register],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	call	store_instruction
	jmp	instruction_assembled
fldcw_instruction:
	mov	[postbyte_register],al
	mov	[base_code],0D9h
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	fldcw_mem_16bit
	cmp	al,2
	je	fldcw_mem_16bit
	jmp	invalid_operand_size
      fldcw_mem_16bit:
	call	store_instruction
	jmp	instruction_assembled
fstsw_instruction:
	mov	[base_code],0DDh
	mov	[postbyte_register],7
	call	get_operand_size
	lods	byte [esi]
	cmp	al,10h
	je	fstsw_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	fstsw_mem_16bit
	cmp	al,2
	je	fstsw_mem_16bit
	jmp	invalid_operand_size
      fstsw_mem_16bit:
	call	store_instruction
	jmp	instruction_assembled
      fstsw_reg:
	lods	byte [esi]
	cmp	al,20h
	jne	invalid_operand
	call	convert_register
	mov	ax,0E0DFh
	stos	word [edi]
	jmp	instruction_assembled
finit_instruction:
	mov	ah,al
	mov	al,0DBh
	stos	word [edi]
	jmp	instruction_assembled

movd_instruction:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,16h
	je	movd_mmreg
	cmp	al,10h
	je	movd_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,16h
	jne	invalid_operand
	lods	byte [esi]
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	movd_mem_mmreg
	cmp	[operand_size],4
	jne	invalid_operand_size
      movd_mem_mmreg:
	mov	[base_code],0Fh
	mov	[extended_code],7Eh
	call	store_instruction
	jmp	instruction_assembled
      movd_reg:
	lods	byte [esi]
	call	convert_register
	cmp	[operand_size],4
	jne	invalid_operand_size
	push	ax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,16h
	jne	invalid_operand
	lods	byte [esi]
	pop	bx
	shl	al,3
	or	bl,al
	or	bl,11000000b
	mov	ax,7E0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      movd_mmreg:
	cmp	[operand_size],0
	jne	invalid_operand
	lods	byte [esi]
	push	ax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	je	movd_mmreg_mem
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	[operand_size],4
	jne	invalid_operand_size
	pop	bx
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	ax,6E0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      movd_mmreg_mem:
	call	get_address
	pop	ax
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	.store
	cmp	[operand_size],4
	jne	invalid_operand_size
      .store:
	mov	[base_code],0Fh
	mov	[extended_code],6Eh
	call	store_instruction
	jmp	instruction_assembled
movq_instruction:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,16h
	je	movq_mmreg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,16h
	jne	invalid_operand
	lods	byte [esi]
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	movq_mem_mmreg
	cmp	[operand_size],8
	jne	invalid_operand_size
      movq_mem_mmreg:
	mov	[base_code],0Fh
	mov	[extended_code],7Fh
	call	store_instruction
	jmp	instruction_assembled
      movq_mmreg:
	cmp	[operand_size],0
	jne	invalid_operand
	lods	byte [esi]
	push	ax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	ax
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	.store
	cmp	[operand_size],8
	jne	invalid_operand_size
      .store:
	mov	[base_code],0Fh
	mov	[extended_code],6Fh
	call	store_instruction
	jmp	instruction_assembled
mmx_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,16h
	jne	invalid_operand
	cmp	[operand_size],0
	jne	invalid_operand
	lods	byte [esi]
	push	ax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	cmp	byte [esi],16h
	je	mmx_mmreg_mmreg
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	ax
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	.store
	cmp	[operand_size],8
	jne	invalid_operand_size
      .store:
	call	store_instruction
	jmp	instruction_assembled
      mmx_mmreg_mmreg:
	inc	esi
	lods	byte [esi]
	pop	bx
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
mmx_32bit_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,16h
	jne	invalid_operand
	cmp	[operand_size],0
	jne	invalid_operand
	lods	byte [esi]
	push	ax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	cmp	byte [esi],16h
	je	mmx_mmreg_mmreg
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	ax
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	.store
	cmp	[operand_size],4
	jne	invalid_operand_size
      .store:
	call	store_instruction
	jmp	instruction_assembled
mmx_ps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	call	get_operand_size
	lods	byte [esi]
	cmp	al,16h
	jne	invalid_operand
	cmp	[operand_size],0
	jne	invalid_operand
	lods	byte [esi]
	push	ax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	cmp	byte [esi],16h
	je	mmx_ps_mmreg_mmreg
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	je	mmx_ps_mmreg_imm8
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	ax
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	.store
	cmp	[operand_size],8
	jne	invalid_operand_size
      .store:
	call	store_instruction
	jmp	instruction_assembled
      mmx_ps_mmreg_mmreg:
	inc	esi
	lods	byte [esi]
	pop	bx
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      mmx_ps_mmreg_imm8:
	call	get_byte_value
	mov	byte [value],al
	mov	al,[operand_size]
	cmp	al,1
	je	mmx_ps_imm8_size_ok
	or	al,al
	jnz	invalid_operand_size
      mmx_ps_imm8_size_ok:
	mov	al,[extended_code]
	mov	ah,al
	shr	al,4
	and	ah,1111b
	add	ah,70h
	mov	[extended_code],ah
	sub	al,0Ch
	shl	al,1
	pop	bx
	shl	al,3
	or	bl,al
	or	bl,11000000b
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled

get_operand_size:
	cmp	byte [esi],11h
	jne	operand_size_ok
	lods	word [esi]
	xchg	[operand_size],ah
	mov	[forced_size],1
	or	ah,ah
	jz	forced_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
	jmp	forced_ok
      operand_size_ok:
	cmp	byte [esi],'['
	jne	forced_ok
	mov	[forced_size],0
      forced_ok:
	ret
get_jump_operator:
	mov	[jump_type],0
	cmp	byte [esi],12h
	jne	jump_operator_ok
	lods	word [esi]
	mov	[jump_type],ah
      jump_operator_ok:
	ret
convert_register:
	cmp	al,60h
	jae	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
	cmp	ah,4
	ja	invalid_operand
	xchg	ah,[operand_size]
	or	ah,ah
	jz	register_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      register_size_ok:
	ret
operand_16bit_prefix:
	cmp	[code_type],16
	je	size_prefix_ok
	mov	al,66h
	stos	byte [edi]
	ret
operand_32bit_prefix:
	cmp	[code_type],32
	je	size_prefix_ok
	mov	al,66h
	stos	byte [edi]
      size_prefix_ok:
	ret
store_segment_prefix:
	mov	al,[segment_register]
	or	al,al
	jz	segment_prefix_ok
	cmp	al,5
	jae	segment_prefix_386
	dec	al
	shl	al,3
	add	al,26h
	stos	byte [edi]
	jmp	segment_prefix_ok
      segment_prefix_386:
	add	al,64h-5
	stos	byte [edi]
      segment_prefix_ok:
	ret
store_instruction:
	call	store_segment_prefix
	or	bx,bx
	jz	address_immediate
	mov	al,bl
	or	al,bh
	and	al,11110000b
	cmp	al,40h
	je	postbyte_32bit
	call	address_16bit_prefix
	call	store_instruction_code
	cmp	bx,2326h
	je	address_bx_si
	cmp	bx,2623h
	je	address_bx_si
	cmp	bx,2327h
	je	address_bx_di
	cmp	bx,2723h
	je	address_bx_di
	cmp	bx,2526h
	je	address_bp_si
	cmp	bx,2625h
	je	address_bp_si
	cmp	bx,2527h
	je	address_bp_di
	cmp	bx,2725h
	je	address_bp_di
	cmp	bx,2600h
	je	address_si
	cmp	bx,2700h
	je	address_di
	cmp	bx,2300h
	je	address_bx
	cmp	bx,2500h
	je	address_bp
	jmp	invalid_address
      address_bx_si:
	xor	al,al
	jmp	postbyte_16bit
      address_bx_di:
	mov	al,1
	jmp	postbyte_16bit
      address_bp_si:
	mov	al,10b
	jmp	postbyte_16bit
      address_bp_di:
	mov	al,11b
	jmp	postbyte_16bit
      address_si:
	mov	al,100b
	jmp	postbyte_16bit
      address_di:
	mov	al,101b
	jmp	postbyte_16bit
      address_bx:
	mov	al,111b
	jmp	postbyte_16bit
      address_bp:
	mov	al,110b
      postbyte_16bit:
	cmp	ch,1
	je	address_8bit_value
	cmp	ch,2
	je	address_16bit_value
	or	ch,ch
	jnz	address_sizes_do_not_agree
	or	edx,edx
	jz	address
	cmp	edx,80h
	jb	address_8bit_value
	cmp	edx,-80h
	jae	address_8bit_value
      address_16bit_value:
	or	al,10000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
	mov	eax,edx
	stos	word [edi]
	cmp	edx,10000h
	jge	value_out_of_range
	cmp	edx,-8000h
	jl	value_out_of_range
	ret
      address_8bit_value:
	or	al,01000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	cmp	edx,80h
	jge	value_out_of_range
	cmp	edx,-80h
	jl	value_out_of_range
	ret
      address:
	cmp	al,110b
	je	address_8bit_value
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
	ret
      postbyte_32bit:
	call	address_32bit_prefix
	call	store_instruction_code
	cmp	bl,44h
	je	invalid_address
	or	cl,cl
	jz	only_base_register
      base_and_index:
	mov	al,100b
	xor	ah,ah
	cmp	cl,1
	je	scale_ok
	cmp	cl,2
	je	scale_1
	cmp	cl,4
	je	scale_2
	or	ah,11000000b
	jmp	scale_ok
      scale_2:
	or	ah,10000000b
	jmp	scale_ok
      scale_1:
	or	ah,01000000b
      scale_ok:
	or	bh,bh
	jz	only_index_register
	and	bl,111b
	shl	bl,3
	or	ah,bl
	and	bh,111b
	or	ah,bh
	cmp	ch,1
	je	sib_address_8bit_value
	test	ch,4
	jnz	sib_address_32bit_value
	cmp	ch,2
	je	address_sizes_do_not_agree
	cmp	bh,5
	je	address_value
	or	edx,edx
	jz	sib_address
      address_value:
	cmp	edx,80h
	jb	sib_address_8bit_value
	cmp	edx,-80h
	jae	sib_address_8bit_value
      sib_address_32bit_value:
	or	al,10000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	word [edi]
	jmp	store_address_32bit_value
      sib_address_8bit_value:
	or	al,01000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	word [edi]
	mov	al,dl
	stos	byte [edi]
	cmp	edx,80h
	jge	value_out_of_range
	cmp	edx,-80h
	jl	value_out_of_range
	ret
      sib_address:
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	word [edi]
	ret
      only_index_register:
	or	ah,101b
	and	bl,111b
	shl	bl,3
	or	ah,bl
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	word [edi]
	test	ch,4
	jnz	store_address_32bit_value
	or	ch,ch
	jnz	invalid_address_size
	jmp	store_address_32bit_value
      zero_index_register:
	mov	bl,4
	mov	cl,1
	jmp	base_and_index
      only_base_register:
	mov	al,bh
	and	al,111b
	cmp	al,4
	je	zero_index_register
	cmp	ch,1
	je	simple_address_8bit_value
	test	ch,4
	jnz	simple_address_32bit_value
	cmp	ch,2
	je	address_sizes_do_not_agree
	or	edx,edx
	jz	simple_address
	cmp	edx,80h
	jb	simple_address_8bit_value
	cmp	edx,-80h
	jae	simple_address_8bit_value
      simple_address_32bit_value:
	or	al,10000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
	jmp	store_address_32bit_value
      simple_address_8bit_value:
	or	al,01000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	cmp	edx,80h
	jge	value_out_of_range
	cmp	edx,-80h
	jl	value_out_of_range
	ret
      simple_address:
	cmp	al,5
	je	simple_address_8bit_value
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
	ret
      address_immediate:
	test	ch,4
	jnz	address_immediate_32bit
	cmp	ch,2
	je	address_immediate_16bit
	or	ch,ch
	jnz	invalid_address_size
	cmp	[code_type],16
	je	addressing_16bit
	cmp	edx,10000h
	jb	address_immediate_16bit
      address_immediate_32bit:
	call	address_32bit_prefix
	call	store_instruction_code
	mov	al,101b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
      store_address_32bit_value:
	test	ch,80h
	jz	address_relocation_ok
	push	word [value_type]
	mov	[value_type],2
	call	mark_relocation
	pop	ax
	mov	[value_type],al
      address_relocation_ok:
	mov	eax,edx
	stos	dword [edi]
	ret
      addressing_16bit:
	cmp	edx,10000h
	jge	address_immediate_32bit
	cmp	edx,-8000h
	jl	address_immediate_32bit
	movzx	edx,dx
      address_immediate_16bit:
	call	address_16bit_prefix
	call	store_instruction_code
	mov	al,110b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
	mov	eax,edx
	stos	word [edi]
	cmp	edx,10000h
	jge	value_out_of_range
	cmp	edx,-8000h
	jl	value_out_of_range
	ret
      store_instruction_code:
	mov	al,[base_code]
	stos	byte [edi]
	cmp	al,0Fh
	jne	instruction_code_ok
	mov	al,[extended_code]
	stos	byte [edi]
      instruction_code_ok:
	ret
      address_16bit_prefix:
	cmp	[code_type],16
	je	instruction_prefix_ok
	mov	al,67h
	stos	byte [edi]
	ret
      address_32bit_prefix:
	cmp	[code_type],32
	je	instruction_prefix_ok
	mov	al,67h
	stos	byte [edi]
      instruction_prefix_ok:
	ret

output_format db ?
code_type db ?
current_pass db ?
next_pass_needed db ?
home_line dd ?
current_line dd ?
org_start dd ?
undefined_data_start dd ?
undefined_data_end dd ?
virtual_mode db ?
reloc_labels db ?
stored_org dd ?
stored_offset dd ?
stored_reloc_labels db ?
counter dd ?
counter_limit dd ?
repeat_stack dw ?
if_stack dw ?
else_flag db ?
error_line dd ?
error_home_line dd ?
error dd ?

base_code db ?
extended_code db ?
postbyte_register db ?
segment_register db ?
operand_size db ?
jump_type db ?
value dq ?
