
create_symbol:
	cmp	ecx,256
	jae	name_too_long
	push	esi edi
	push	eax edx
	mov	esi,ebx
	push	esi ecx
	call	check_symbol_name
	cmp	byte [esi],'.'
	je	section_ok
	inc	[current_section]
      section_ok:
	mov	ecx,[esp]
	call	get_symbol
	cmp	cl,1
	jbe	add_symbol
	cmp	cl,3
	jne	reserved_word_used_as_symbol
	add	esp,8
	mov	al,[current_pass]
	cmp	al,[ebx+9]
	je	symbol_already_defined
	mov	[ebx+9],al
	pop	edx eax
	cmp	[ebx],eax
	jne	redefine_symbol
	cmp	[ebx+4],edx
	jne	redefine_symbol
	pop	edi esi
	ret
      redefine_symbol:
	mov	[ebx],eax
	mov	[ebx+4],edx
	mov	[next_pass_needed],1
	pop	edi esi
	ret
      add_symbol:
	pop	ecx esi
	mov	edi,[symbols_list]
	sub	edi,ecx
	sub	edi,16
	lea	ebx,[edi-1]
	mov	[ebx],cl
	rep	movsb
	mov	[symbols_list],ebx
	mov	ebx,edi
	mov	eax,[current_section]
	mov	[ebx+Ch],eax
	mov	al,[current_pass]
	mov	[ebx+9],al
	pop	edx eax
	mov	[ebx],eax
	mov	[ebx+4],edx
	mov	byte [ebx+10],0
	pop	edi esi
	cmp	ebx,edi
	jbe	out_of_memory
	ret
check_symbol_name:
	cmp	[current_pass],0
	jne	symbol_name_ok
	mov	ebp,edi
	call	get_number
	jnc	illegal_instruction
	mov	edi,general_registers
	call	get_register
	jnc	reserved_word_used_as_symbol
	mov	edi,segment_registers
	call	get_register
	jnc	reserved_word_used_as_symbol
	mov	edi,control_registers
	call	get_register
	jnc	reserved_word_used_as_symbol
	mov	edi,debug_registers
	call	get_register
	jnc	reserved_word_used_as_symbol
	mov	edi,fpu_registers
	call	get_register
	jnc	reserved_word_used_as_symbol
	mov	edi,size_operators
	call	get_operator
	or	al,al
	jnz	reserved_word_used_as_symbol
	mov	edi,single_operand_operators
	call	get_operator
	or	al,al
	jnz	reserved_word_used_as_symbol
	mov	edi,jump_operators
	call	get_operator
	or	al,al
	jnz	reserved_word_used_as_symbol
	cmp	ecx,1
	jne	symbol_name_ok
	cmp	byte [esi],'$'
	je	reserved_word_used_as_symbol
      symbol_name_ok:
	ret
get_symbol:
	cmp	cl,1
	jne	get_symbol_from_list
	cmp	byte [esi],'$'
	jne	get_symbol_from_list
	mov	al,[esi+1]
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	jne	get_symbol_from_list
	inc	esi
	mov	eax,[current_offset]
	sub	eax,[org_start]
	xor	edx,edx
	mov	cx,2
	cmp	[program_relocatable],0
	je	got_symbol
	mov	ch,1
      got_symbol:
	ret
      get_symbol_from_list:
	mov	edi,[symbols_list]
	mov	edx,esi
	mov	ebp,ecx
      check_symbol:
	mov	esi,edx
	movzx	eax,byte [edi]
	or	al,al
	jz	no_symbol
	mov	ecx,ebp
	inc	edi
	mov	ebx,edi
	add	ebx,eax
	cmp	cl,al
	jne	next_symbol
	mov	ah,[esi]
	repe	cmpsb
	je	symbol_found
      next_symbol:
	mov	edi,ebx
	add	edi,16
	jmp	check_symbol
      no_symbol:
	mov	esi,edx
	cmp	[current_pass],0
	je	no_symbol_yet
      symbol_undefined:
	xor	cx,cx
	ret
      no_symbol_yet:
	mov	esi,edx
	add	esi,ebp
	xor	eax,eax
	xor	edx,edx
	mov	cx,1
	ret
      symbol_found:
	cmp	ah,'.'
	jne	symbol_ok
	mov	eax,[edi+Ch]
	cmp	eax,[current_section]
	jne	next_symbol
      symbol_ok:
	mov	ebx,edi
	cmp	[operand_size],0
	jne	operand_size_ok
	mov	al,[ebx+8]
	mov	[operand_size],al
      operand_size_ok:
	mov	eax,[ebx]
	mov	edx,[ebx+4]
	mov	cl,3
	mov	ch,[ebx+10]
	cmp	ch,2
	je	imported_symbol
	cmp	ch,1
	jne	got_symbol
	cmp	[program_relocatable],0
	jne	got_symbol
	xor	ch,ch
	ret
      imported_symbol:
	mov	[import_value],eax
	xor	eax,eax
	ret

symbols_list dd 0
current_section dd 0
