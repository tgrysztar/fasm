
; flat assembler  version 1.02
; Copyright (c) 1999-2000, Tomasz Grysztar
; All rights reserved.

parser:
	mov	eax,[memory_end]
	mov	[last_label_id],eax
	dec	eax
	mov	byte [eax],0
	mov	[labels_list],eax
	mov	[current_locals_prefix],0
	xor	al,al
	mov	edi,[conventional_memory_end]
	dec	edi
	mov	[edi],al
	mov	[names_list],edi
	mov	esi,[source_start]
	mov	edi,[code_start]
     parser_loop:
	mov	[current_line],esi
	add	esi,4
	lods	byte [esi]
	mov	[line_type],al
	cmp	edi,[labels_list]
	jae	out_of_memory
	call	parse_line
	cmp	esi,[code_start]
	jb	parser_loop
	xor	al,al
	stos	byte [edi]
	mov	eax,[code_start]
	mov	[source_start],eax
	mov	[code_start],edi
	ret

parse_line:
	mov	al,80h
	mov	ah,[line_type]
	cmp	ah,20h
	je	line_start
	cmp	ah,';'
	je	line_start
	cmp	ah,'-'
	je	line_start
	mov	eax,[current_line]
	mov	[home_line],eax
	mov	al,81h
      line_start:
	stos	byte [edi]
	mov	eax,[current_line]
	stos	dword [edi]
	push	edi
	mov	edi,[conventional_memory]
	mov	[symbol_counter],0
      convert_line_data:
	lods	byte [esi]
	stos	byte [edi]
	mov	ebx,edi
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scas byte [edi]
	mov	edi,ebx
	je	convert_separator
	or	al,al
	jz	convert_separator
	cmp	al,27h
	je	copy_string
	mov	al,1Ah
	xchg	al,[edi-1]
	inc	edi
	stos	byte [edi]
	mov	edx,1
      convert_symbol:
	lods	byte [esi]
	stos	byte [edi]
	mov	ebx,edi
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scas byte [edi]
	mov	edi,ebx
	je	symbol_end
	or	al,al
	jz	symbol_end
	inc	edx
	jmp	convert_symbol
      copy_string:
	lods	byte [esi]
	stos	byte [edi]
	or	al,al
	jz	missing_end_quote
	cmp	al,27h
	jne	copy_string
	lods	byte [esi]
	cmp	al,27h
	je	copy_string
	dec	esi
	mov	byte [edi-1],1Ah
	jmp	convert_line_data
      symbol_end:
	cmp	edx,255
	ja	name_too_long
	mov	ebx,edi
	sub	ebx,edx
	mov	byte [ebx-2],dl
	cmp	[symbol_counter],0
	jne	count_symbol
	cmp	al,':'
	je	preprocess_symbol
      count_symbol:
	inc	[symbol_counter]
      preprocess_symbol:
	cmp	[line_type],'='
	je	check_symbol_count
	cmp	[line_type],'-'
	jne	check_name
      check_symbol_count:
	cmp	[symbol_counter],0
	je	check_name
	cmp	[symbol_counter],2
	jbe	convert_separator
      check_name:
	push	ax esi
	mov	ebx,[names_list]
	lea	esi,[edi-1]
	sub	esi,edx
      compare_names:
	mov	eax,esi
	mov	cx,dx
	cmp	byte [ebx],0
	je	symbol_converted
	cmp	[ebx],cl
	jne	next_name
	lea	edi,[ebx+1]
	repe	cmps byte [esi],[edi]
	jne	next_name
	mov	dl,[esi]
	lea	esi,[edi+4]
	mov	ecx,[edi]
	lea	edi,[eax-2]
	mov	ebx,ecx
	rep	movs byte [edi],[esi]
	mov	ecx,ebx
	mov	esi,eax
	mov	al,dl
	stos	byte [edi]
	pop	esi ax
	jmp	convert_separator
      next_name:
	mov	esi,eax
	movzx	eax,byte [ebx]
	lea	ebx,[ebx+1+eax]
	mov	eax,[ebx]
	lea	ebx,[ebx+4+eax]
	jmp	compare_names
      symbol_converted:
	add	esi,ecx
	lea	edi,[esi+1]
	pop	esi ax
      convert_separator:
	cmp	al,20h
	je	convert_space
	cmp	al,9
	je	convert_space
	cmp	al,3Bh
	je	ignore_comment
	or	al,al
	jnz	convert_line_data
	jmp	line_converted
      convert_space:
	dec	edi
	jmp	convert_line_data
      ignore_comment:
	lods	byte [esi]
	or	al,al
	jnz	ignore_comment
	mov	byte [edi-1],0
      line_converted:
	pop	edi
	push	esi
	mov	esi,[conventional_memory]
	mov	[parenthesis_stack],0
	cmp	[line_type],'{'
	je	skip_line_data
      instruction_start:
	push	edi
	cmp	byte [esi],1Ah
	jne	empty_instruction
	inc	esi
	lods	byte [esi]
	movzx	ecx,al
	cmp	byte [esi+ecx],':'
	je	simple_label
	cmp	[line_type],':'
	je	skip_rest_of_line
	cmp	[line_type],';'
	je	skip_rest_of_line
	push	esi ecx
	add	esi,ecx
	cmp	byte [esi],1Ah
	je	get_following_instruction
	cmp	byte [esi],'='
	je	constant_label
	jmp	get_main_instruction
      get_following_instruction:
	inc	esi
	lods	byte [esi]
	movzx	ecx,al
	call	get_instruction
	jc	get_following_symbol
	cmp	ah,1
	je	data_label
	cmp	ah,2
	je	data_label
	jmp	get_main_instruction
      get_following_symbol:
	mov	edi,symbols
	call	get_symbol
	jc	get_main_instruction
	cmp	ax,17h
	je	equ_definition
      get_main_instruction:
	pop	ecx esi
	call	get_instruction
	pop	edi
	jnc	parse_instruction
	push	edi
	mov	edi,symbols
	call	get_symbol
	jc	illegal_instruction
	pop	edi
	stos	word [edi]
	jmp	parse_arguments
      constant_label:
	pop	ecx esi
	call	check_label_name
	pop	edi
	call	identify_label
	mov	byte [edi],3
	inc	edi
	stos	dword [edi]
	xor	al,al
	stos	byte [edi]
	inc	esi
	jmp	parse_arguments
      equ_definition:
	mov	ebx,esi
      find_equ_end:
	lods	byte [esi]
	or	al,al
	jnz	find_equ_end
	dec	esi
	mov	ecx,esi
	sub	ecx,ebx
	pop	eax edx
	push	esi
	mov	esi,ebx
	mov	edi,[names_list]
	sub	edi,ecx
	lea	ebx,[edi-4]
	mov	[ebx],ecx
	rep	movs byte [edi],[esi]
	mov	esi,edx
	sub	ebx,eax
	mov	edi,ebx
	dec	ebx
	mov	[ebx],al
	mov	[names_list],ebx
	sub	ebx,100h
	cmp	ebx,[conventional_memory]
	jbe	out_of_memory
	mov	ecx,eax
	cmp	byte [esi],27h
	je	invalid_name
	rep	movs byte [edi],[esi]
	pop	esi edi
	jmp	instruction_start
      data_label:
	mov	ebp,esi
	pop	ecx esi
	pop	edi
	push	ax bx
	push	ebp edi
	call	check_label_name
	pop	edi
	call	identify_label
	pop	esi
	mov	byte [edi],2
	inc	edi
	stos	dword [edi]
	pop	bx ax
	stos	byte [edi]
	jmp	parse_instruction
      simple_label:
	call	check_label_name
	pop	edi
	call	identify_label
	mov	byte [edi],2
	inc	edi
	stos	dword [edi]
	inc	esi
	xor	al,al
	stos	byte [edi]
	jmp	instruction_start
      check_label_name:
	mov	al,[esi]
	cmp	al,27h
	je	invalid_name
	cmp	ecx,1
	jne	check_for_reserved_word
	cmp	al,'$'
	je	reserved_word_used_as_symbol
      check_for_reserved_word:
	push	ecx esi
	sub	esi,2
	mov	ebp,value
	call	get_number
	jnc	reserved_word_used_as_symbol
	pop	esi ecx
	call	get_instruction
	jnc	reserved_word_used_as_symbol
	mov	edi,symbols
	call	get_symbol
	jnc	reserved_word_used_as_symbol
	ret
      identify_label:
	cmp	byte [esi],'.'
	je	local_label_name
	call	get_label_id
	mov	ebx,[label_name]
	mov	[current_locals_prefix],ebx
	ret
      local_label_name:
	call	get_label_id
	ret
      parse_instruction:
	mov	dx,ax
	mov	al,1
	stos	byte [edi]
	mov	ax,bx
	stos	word [edi]
	mov	ax,dx
	stos	word [edi]
	mov	[instruction_type],ax
	cmp	bx,label_directive-assembler
	je	parse_label_instruction
	cmp	bx,segment_directive-assembler
	jne	parse_arguments
      parse_label_instruction:
	push	edi
	lods	byte [esi]
	cmp	al,1Ah
	jne	invalid_argument
	lods	byte [esi]
	movzx	ecx,al
	call	check_label_name
	pop	edi
	mov	al,4
	stos	byte [edi]
	call	get_label_id
	stos	dword [edi]
      parse_arguments:
	lods	byte [esi]
	cmp	al,','
	je	separator
	cmp	al,':'
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
	cmp	al,27h
	je	string_argument
	cmp	al,1Ah
	jne	no_string
	push	edi
	mov	edx,esi
	inc	esi
	lods	byte [esi]
	movzx	ecx,al
	mov	edi,symbols
	call	get_symbol
	jc	not_symbol_argument
	pop	edi
	stos	word [edi]
	jmp	argument_parsed
      not_symbol_argument:
	cmp	ecx,1
	jne	check_argument
	cmp	byte [esi],'?'
	jne	check_argument
	pop	edi
	movs	byte [edi],[esi]
	jmp	argument_parsed
      check_argument:
	push	esi ecx
	sub	esi,2
	mov	edi,single_operand_operators
	call	get_operator
	pop	ecx esi
	or	al,al
	jnz	expression_argument
	call	get_instruction
	jc	expression_argument
	pop	edi
	jmp	parse_instruction
      expression_argument:
	pop	edi
	sub	esi,2
      string_argument:
	cmp	[instruction_type],101h
	jne	no_string
	mov	al,[esi]
	cmp	al,27h
	jne	no_string
	inc	esi
	stos	byte [edi]
	mov	ebx,edi
	add	edi,4
      convert_string:
	lods	byte [esi]
	cmp	al,1Ah
	je	string_end
	or	al,al
	jz	missing_end_quote
	stos	byte [edi]
	jmp	convert_string
      string_end:
	xor	al,al
	stos	byte [edi]
	mov	ecx,edi
	sub	ecx,ebx
	sub	ecx,5
	mov	[ebx],ecx
	jmp	argument_parsed
      no_string:
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
	call	convert_expression
	mov	al,')'
	stos	byte [edi]
	jmp	expression_parsed
      address_argument:
	mov	al,'['
	stos	byte [edi]
	cmp	byte [esi+4],':'
	jne	convert_address
	cmp	word [esi],021Ah
	jne	convert_address
	add	esi,2
	mov	ecx,2
	push	edi
	mov	edi,symbols
	call	get_symbol
	pop	edi
	jc	invalid_address
	cmp	al,10h
	jne	invalid_address
	mov	al,ah
	and	ah,11110000b
	cmp	ah,60h
	jne	invalid_address
	stos	byte [edi]
	inc	esi
      convert_address:
	cmp	byte [esi],1Ah
	jne	convert_address_value
	push	esi
	lods	word [esi]
	movzx	ecx,ah
	push	edi
	mov	edi,symbols
	call	get_symbol
	pop	edi
	jc	no_size_prefix
	cmp	al,11h
	jne	no_size_prefix
	mov	al,ah
	add	al,70h
	stos	byte [edi]
	add	sp,4
	jmp	convert_address_value
      no_size_prefix:
	pop	esi
      convert_address_value:
	call	convert_expression
	lods	byte [esi]
	cmp	al,']'
	jne	invalid_address
	stos	byte [edi]
	jmp	argument_parsed
      close_expression:
	mov	al,'}'
      separator:
	stos	byte [edi]
	jmp	argument_parsed
      greater:
	cmp	byte [esi],'='
	jne	separator
	inc	esi
	mov	al,'ò'
	jmp	separator
      less:
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
	add	sp,8
	jmp	argument_parsed
      expression_parsed:
	cmp	[parenthesis_stack],0
	je	parse_arguments
	cmp	byte [esi],')'
	jne	argument_parsed
	dec	[parenthesis_stack]
	pop	edi esi
	jmp	parse_expression
      skip_rest_of_line:
	pop	edi
	jmp	skip_line_data
      empty_instruction:
	pop	edi
	lods	byte [esi]
	or	al,al
	jz	line_parsed
	dec	esi
	jmp	parse_arguments
      skip_line_data:
	lods	byte [esi]
	or	al,al
	jnz	skip_line_data
      line_parsed:
	cmp	[parenthesis_stack],0
	jne	invalid_expression
	pop	esi
	ret

line_type db ?
symbol_counter db ?
instruction_type dw ?
current_locals_prefix dd ?
parenthesis_stack db ?
