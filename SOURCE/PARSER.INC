
; flat assembler source
; Copyright (c) 1999-2002, Tomasz Grysztar
; All rights reserved.

parser:
	mov	eax,[memory_end]
	mov	[labels_list],eax
	xor	eax,eax
	mov	[current_locals_prefix],eax
	mov	[anonymous_reverse],eax
	mov	[anonymous_forward],eax
	mov	esi,[source_start]
	mov	edi,[code_start]
	push	[additional_memory]
     parser_loop:
	mov	[current_line],esi
	cmp	edi,[labels_list]
	jae	out_of_memory
	mov	al,0Fh
	stos	byte [edi]
	mov	eax,esi
	stos	dword [edi]
	add	esi,12
	call	parse_line
	cmp	esi,[code_start]
	jb	parser_loop
	xor	al,al
	stos	byte [edi]
	cmp	[anonymous_forward],0
	jne	unexpected_end_of_file
	pop	[additional_memory]
	mov	eax,[code_start]
	mov	[source_start],eax
	mov	[code_start],edi
	ret

parse_line:
	mov	[parenthesis_stack],0
      instruction_start:
	cmp	byte [esi],1Ah
	jne	empty_instruction
	push	edi
	inc	esi
	movzx	ecx,byte [esi]
	inc	esi
	cmp	byte [esi+ecx],':'
	je	simple_label
	push	esi ecx
	add	esi,ecx
	cmp	byte [esi],1Ah
	je	check_for_data_label
	cmp	byte [esi],'='
	je	constant_label
	pop	ecx esi
	jmp	get_main_instruction
      check_for_data_label:
	inc	esi
	movzx	ecx,byte [esi]
	inc	esi
	push	edi
	mov	edi,data_directives
	call	get_symbol
	pop	edi
	jnc	data_label
	pop	ecx esi
      get_main_instruction:
	call	get_instruction
	jnc	parse_instruction
	mov	edi,data_directives
	call	get_symbol
	jnc	data_instruction
	mov	edi,symbols
	call	get_symbol
	pop	edi
	jc	unknown_instruction
	stos	word [edi]
	jmp	parse_arguments
      data_instruction:
	movzx	ebx,ah
	mov	bx,[data_handlers+ebx*2]
	jmp	parse_instruction
      unknown_instruction:
	sub	esi,2
	jmp	parse_arguments
      constant_label:
	pop	ecx esi
	pop	edi
	call	identify_label
	mov	byte [edi],3
	inc	edi
	stos	dword [edi]
	xor	al,al
	stos	byte [edi]
	inc	esi
	jmp	parse_arguments
      data_label:
	pop	ecx ebx
	pop	edi
	push	eax esi
	mov	esi,ebx
	call	identify_label
	mov	byte [edi],2
	inc	edi
	stos	dword [edi]
	pop	esi eax
	stos	byte [edi]
	push	edi
	jmp	data_instruction
      simple_label:
	pop	edi
	call	identify_label
	mov	byte [edi],2
	inc	edi
	stos	dword [edi]
	inc	esi
	xor	al,al
	stos	byte [edi]
	jmp	instruction_start
      identify_label:
	cmp	byte [esi],'.'
	je	local_label_name
	call	get_label_id
	cmp	eax,10h
	jb	label_identified
	mov	ebx,[eax+4]
	cmp	ebx,eax
	je	anonymous_label_name
	dec	ebx
	mov	[current_locals_prefix],ebx
      label_identified:
	ret
      anonymous_label_name:
	cmp	byte [esi-1],'@'
	jne	reserved_word_used_as_symbol
	ret
      local_label_name:
	call	get_label_id
	ret
      parse_prefix_instruction:
	cmp	byte [esi],1Ah
	jne	parse_arguments
	push	edi
	inc	esi
	movzx	ecx,byte [esi]
	inc	esi
	jmp	get_main_instruction
      parse_public_directive:
	cmp	byte [esi],1Ah
	jne	invalid_argument
	push	esi
	movzx	ecx,byte [esi+1]
	add	esi,2
	mov	ax,'('
	stos	word [edi]
	mov	eax,ecx
	stos	dword [edi]
	rep	movs byte [edi],[esi]
	xor	al,al
	stos	byte [edi]
	pop	esi
      parse_label_directive:
	push	edi
	lods	byte [esi]
	cmp	al,1Ah
	jne	invalid_argument
	movzx	ecx,byte [esi]
	lods	byte [esi]
	pop	edi
	mov	al,2
	stos	byte [edi]
	call	identify_label
	stos	dword [edi]
	xor	al,al
	stos	byte [edi]
	jmp	parse_arguments
      parse_instruction:
	pop	edi
	mov	dl,al
	mov	al,1
	stos	byte [edi]
	mov	ax,bx
	stos	word [edi]
	mov	al,dl
	stos	byte [edi]
	cmp	bx,prefix_instruction-assembler
	je	parse_prefix_instruction
	cmp	bx,end_directive-assembler
	je	parse_prefix_instruction
	cmp	bx,label_directive-assembler
	je	parse_label_directive
	cmp	bx,load_directive-assembler
	je	parse_label_directive
	cmp	bx,segment_directive-assembler
	je	parse_label_directive
	cmp	bx,public_directive-assembler
	je	parse_public_directive
	cmp	bx,extrn_directive-assembler
	je	parse_public_directive
      parse_arguments:
	lods	byte [esi]
	cmp	al,':'
	je	instruction_separator
	cmp	al,','
	je	separator
	cmp	al,'='
	je	separator
	cmp	al,'|'
	je	separator
	cmp	al,'&'
	je	separator
	cmp	al,'~'
	je	separator
	cmp	al,'>'
	je	greater
	cmp	al,'<'
	je	less
	cmp	al,')'
	je	close_expression
	or	al,al
	jz	line_parsed
	cmp	al,'['
	je	address_argument
	cmp	al,']'
	je	separator
	dec	esi
	cmp	al,1Ah
	jne	expression_argument
	push	edi
	mov	edi,directive_operators
	call	get_operator
	or	al,al
	jnz	operator_argument
	inc	esi
	movzx	ecx,byte [esi]
	inc	esi
	mov	edi,symbols
	call	get_symbol
	jnc	symbol_argument
	mov	edi,formatter_symbols
	call	get_symbol
	jnc	symbol_argument
	cmp	ecx,1
	jne	check_argument
	cmp	byte [esi],'?'
	jne	check_argument
	pop	edi
	movs	byte [edi],[esi]
	jmp	argument_parsed
      symbol_argument:
	pop	edi
	stos	word [edi]
	jmp	argument_parsed
      operator_argument:
	pop	edi
	cmp	al,85h
	je	ptr_argument
	stos	byte [edi]
	cmp	al,80h
	je	forced_expression
	jmp	argument_parsed
      ptr_argument:
	call	parse_address
	mov	al,']'
	stos	byte [edi]
	jmp	argument_parsed
      check_argument:
	push	esi ecx
	sub	esi,2
	mov	edi,single_operand_operators
	call	get_operator
	pop	ecx esi
	or	al,al
	jnz	not_instruction
	call	get_instruction
	jnc	parse_instruction
	mov	edi,data_directives
	call	get_symbol
	jnc	data_instruction
      not_instruction:
	pop	edi
	sub	esi,2
      expression_argument:
	cmp	byte [esi],22h
	jne	not_string
	mov	eax,[esi+1]
	cmp	eax,8
	ja	string_argument
	lea	ebx,[esi+5+eax]
	push	ebx ecx esi edi
	mov	al,'('
	stos	byte [edi]
	call	convert_expression
	mov	al,')'
	stos	byte [edi]
	pop	eax edx ecx ebx
	cmp	esi,ebx
	jne	expression_parsed
	mov	edi,eax
	mov	esi,edx
      string_argument:
	inc	esi
	mov	ax,'('
	stos	word [edi]
	lods	dword [esi]
	mov	ecx,eax
	stos	dword [edi]
	shr	ecx,1
	jnc	string_movsb_ok
	movs	byte [edi],[esi]
      string_movsb_ok:
	shr	ecx,1
	jnc	string_movsw_ok
	movs	word [edi],[esi]
      string_movsw_ok:
	rep	movs dword [edi],[esi]
	xor	al,al
	stos	byte [edi]
	jmp	argument_parsed
      not_string:
	cmp	byte [esi],'('
	jne	parse_expression
	push	esi edi
	inc	esi
	mov	al,'{'
	stos	byte [edi]
	inc	[parenthesis_stack]
	jmp	parse_arguments
      parse_expression:
	mov	al,'('
	stos	byte [edi]
	call	get_fp_value
	jc	expression
	mov	al,'.'
	stos	byte [edi]
	mov	eax,[fp_value]
	stos	dword [edi]
	mov	eax,[fp_value+4]
	stos	dword [edi]
	mov	eax,[fp_value+8]
	stos	dword [edi]
	jmp	expression_parsed
      forced_expression:
	mov	al,'('
	stos	byte [edi]
      expression:
	call	convert_expression
	mov	al,')'
	stos	byte [edi]
	jmp	expression_parsed
      address_argument:
	call	parse_address
	lods	byte [esi]
	cmp	al,']'
	jne	invalid_address
	stos	byte [edi]
	jmp	argument_parsed
      parse_address:
	mov	al,'['
	stos	byte [edi]
	cmp	word [esi],021Ah
	jne	convert_address
	push	esi
	add	esi,4
	lea	ebx,[esi+1]
	cmp	byte [esi],':'
	pop	esi
	jne	convert_address
	add	esi,2
	mov	ecx,2
	push	ebx edi
	mov	edi,symbols
	call	get_symbol
	pop	edi esi
	jc	invalid_address
	cmp	al,10h
	jne	invalid_address
	mov	al,ah
	and	ah,11110000b
	cmp	ah,60h
	jne	invalid_address
	stos	byte [edi]
      convert_address:
	cmp	byte [esi],1Ah
	jne	convert_expression
	push	esi
	lods	word [esi]
	movzx	ecx,ah
	push	edi
	mov	edi,address_sizes
	call	get_symbol
	pop	edi
	jc	no_size_prefix
	mov	al,ah
	add	al,70h
	stos	byte [edi]
	add	esp,4
	jmp	convert_expression
      no_size_prefix:
	pop	esi
	jmp	convert_expression
      close_expression:
	mov	al,'}'
      separator:
	stos	byte [edi]
	jmp	argument_parsed
      instruction_separator:
	stos	byte [edi]
	jmp	instruction_start
      greater:
	cmp	byte [esi],'='
	jne	separator
	inc	esi
	mov	al,'ò'
	jmp	separator
      less:
	cmp	byte [edi-1],83h
	je	separator
	cmp	byte [esi],'>'
	je	not_equal
	cmp	byte [esi],'='
	jne	separator
	inc	esi
	mov	al,'ó'
	jmp	separator
      not_equal:
	inc	esi
	mov	al,'ö'
	jmp	separator
      argument_parsed:
	cmp	[parenthesis_stack],0
	je	parse_arguments
	dec	[parenthesis_stack]
	add	esp,8
	jmp	argument_parsed
      expression_parsed:
	cmp	[parenthesis_stack],0
	je	parse_arguments
	cmp	byte [esi],')'
	jne	argument_parsed
	dec	[parenthesis_stack]
	pop	edi esi
	jmp	parse_expression
      empty_instruction:
	lods	byte [esi]
	or	al,al
	jz	line_parsed
	cmp	al,':'
	je	invalid_name
	cmp	al,3Bh
	je	skip_preprocessed_symbol
	dec	esi
	jmp	parse_arguments
      skip_preprocessed_symbol:
	lods	byte [esi]
	movzx	eax,al
	add	esi,eax
      skip_next:
	lods	byte [esi]
	or	al,al
	jz	line_parsed
	cmp	al,1Ah
	je	skip_preprocessed_symbol
	cmp	al,22h
	je	skip_preprocessed_string
	jmp	skip_next
      skip_preprocessed_string:
	lods	dword [esi]
	add	esi,eax
	jmp	skip_next
      line_parsed:
	cmp	[parenthesis_stack],0
	jne	invalid_expression
	ret
