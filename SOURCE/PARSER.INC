
; flat assembler core
; Copyright (c) 1999-2003, Tomasz Grysztar.
; All rights reserved.

parser:
	mov	eax,[memory_end]
	mov	[labels_list],eax
	mov	eax,[additional_memory]
	mov	[free_additional_memory],eax
	xor	eax,eax
	mov	[current_locals_prefix],eax
	mov	[anonymous_reverse],eax
	mov	[anonymous_forward],eax
	mov	esi,[memory_start]
	mov	edi,[source_start]
     parser_loop:
	mov	[current_line],esi
	cmp	edi,[labels_list]
	jae	out_of_memory
	mov	al,0Fh
	stos	byte [edi]
	mov	eax,esi
	stos	dword [edi]
	add	esi,16
	call	parse_line
	cmp	esi,[source_start]
	jb	parser_loop
	xor	al,al
	stos	byte [edi]
	cmp	[anonymous_forward],0
	jne	unexpected_end_of_file
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
	or	ebx,ebx
	jz	anonymous_label_name
	dec	ebx
	mov	[current_locals_prefix],ebx
      label_identified:
	ret
      anonymous_label_name:
	cmp	byte [esi-1],'@'
	je	anonymous_label_name_ok
	mov	eax,0Fh
      anonymous_label_name_ok:
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
      parse_label_directive:
	cmp	byte [esi],1Ah
	jne	argument_parsed
	push	edi
	inc	esi
	movzx	ecx,byte [esi]
	lods	byte [esi]
	pop	edi
	mov	al,2
	stos	byte [edi]
	call	identify_label
	stos	dword [edi]
	xor	al,al
	stos	byte [edi]
	jmp	argument_parsed
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
	cmp	bx,extrn_directive-assembler
	je	parse_extrn_directive
	cmp	bx,public_directive-assembler
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
	cmp	al,'{'
	je	unallowed_character
	cmp	al,'}'
	je	unallowed_character
	cmp	al,'#'
	je	unallowed_character
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
	cmp	al,89h
	je	parse_label_operator
	jmp	argument_parsed
      parse_public_directive:
	cmp	byte [esi],1Ah
	jne	parse_arguments
	inc	esi
	push	esi
	movzx	ecx,byte [esi]
	lods	byte [esi]
	mov	al,2
	stos	byte [edi]
	call	get_label_id
	stos	dword [edi]
	mov	ax,8600h
	stos	word [edi]
	pop	ebx
	push	ebx esi edi
	mov	edi,directive_operators
	call	get_operator
	pop	edi edx ebx
	cmp	al,86h
	je	argument_parsed
	mov	esi,edx
	xchg	esi,ebx
	movzx	ecx,byte [esi]
	inc	esi
	mov	ax,'('
	stos	word [edi]
	mov	eax,ecx
	stos	dword [edi]
	rep	movs byte [edi],[esi]
	xor	al,al
	stos	byte [edi]
	xchg	esi,ebx
	jmp	argument_parsed
      parse_extrn_directive:
	cmp	byte [esi],22h
	je	parse_quoted_extrn
	cmp	byte [esi],1Ah
	jne	parse_arguments
	push	esi
	movzx	ecx,byte [esi+1]
	add	esi,2
	mov	ax,'('
	stos	word [edi]
	mov	eax,ecx
	stos	dword [edi]
	rep	movs byte [edi],[esi]
	mov	ax,8600h
	stos	word [edi]
	pop	esi
      parse_label_operator:
	cmp	byte [esi],1Ah
	jne	argument_parsed
	inc	esi
	movzx	ecx,byte [esi]
	lods	byte [esi]
	mov	al,2
	stos	byte [edi]
	call	get_label_id
	stos	dword [edi]
	xor	al,al
	stos	byte [edi]
	jmp	argument_parsed
      parse_quoted_extrn:
	inc	esi
	mov	ax,'('
	stos	word [edi]
	lods	dword [esi]
	mov	ecx,eax
	stos	dword [edi]
	rep	movs byte [edi],[esi]
	xor	al,al
	stos	byte [edi]
	push	esi edi
	mov	edi,directive_operators
	call	get_operator
	mov	edx,esi
	pop	edi esi
	cmp	al,86h
	jne	argument_parsed
	stos	byte [edi]
	mov	esi,edx
	jmp	parse_label_operator
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
	jmp	expression_parsed
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
      expression:
	call	convert_expression
	mov	al,')'
	stos	byte [edi]
	jmp	expression_parsed
      forced_expression:
	mov	al,'('
	stos	byte [edi]
	call	convert_expression
	mov	al,')'
	stos	byte [edi]
	jmp	argument_parsed
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
      unallowed_character:
	mov	al,0FFh
	jmp	separator
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
	mov	al,0F2h
	jmp	separator
      less:
	cmp	byte [edi-1],0F6h
	je	separator
	cmp	byte [esi],'>'
	je	not_equal
	cmp	byte [esi],'='
	jne	separator
	inc	esi
	mov	al,0F3h
	jmp	separator
      not_equal:
	inc	esi
	mov	al,0F1h
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
	cmp	al,3Bh
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

get_operator:
	cmp	byte [esi],1Ah
	jne	get_simple_operator
	mov	edx,esi
	push	ebp
	inc	esi
	lods	byte [esi]
	movzx	ebp,al
	push	edi
	mov	ecx,ebp
	call	lower_case
	pop	edi
      check_operator:
	mov	esi,converted
	movzx	ecx,byte [edi]
	jecxz	no_operator
	inc	edi
	mov	ebx,edi
	add	ebx,ecx
	cmp	ecx,ebp
	jne	next_operator
	repe	cmps byte [esi],[edi]
	je	operator_found
      next_operator:
	mov	edi,ebx
	inc	edi
	jmp	check_operator
      no_operator:
	mov	esi,edx
	mov	ecx,ebp
	pop	ebp
      no_simple_operator:
	xor	al,al
	ret
      operator_found:
	lea	esi,[edx+2+ebp]
	mov	ecx,ebp
	pop	ebp
	mov	al,[edi]
	ret
      get_simple_operator:
	mov	al,[esi]
	cmp	al,22h
	je	no_simple_operator
      simple_operator:
	cmp	byte [edi],1
	jb	no_simple_operator
	ja	simple_next_operator
	cmp	al,[edi+1]
	je	simple_operator_found
      simple_next_operator:
	movzx	ecx,byte [edi]
	lea	edi,[edi+1+ecx+1]
	jmp	simple_operator
      simple_operator_found:
	inc	esi
	mov	al,[edi+2]
	ret


get_symbol:
	mov	edx,esi
	mov	ebp,ecx
	push	edi
	call	lower_case
	pop	edi
      scan_symbols:
	mov	esi,converted
	movzx	eax,byte [edi]
	or	al,al
	jz	no_symbol
	mov	ecx,ebp
	inc	edi
	mov	ebx,edi
	add	ebx,eax
	mov	ah,[esi]
	cmp	ah,[edi]
	jb	no_symbol
	ja	next_symbol
	cmp	cl,al
	jne	next_symbol
	repe	cmps byte [esi],[edi]
	jb	no_symbol
	je	symbol_ok
      next_symbol:
	mov	edi,ebx
	add	edi,2
	jmp	scan_symbols
      no_symbol:
	mov	esi,edx
	mov	ecx,ebp
	stc
	ret
      symbol_ok:
	lea	esi,[edx+ebp]
	mov	ax,[ebx]
	clc
	ret

get_instruction:
	mov	edx,esi
	mov	ebp,ecx
	call	lower_case
	mov	ecx,ebp
	cmp	cl,11
	ja	no_instruction
	sub	cl,2
	jc	no_instruction
	movzx	edi,word [instructions+ecx*2]
	add	edi,instructions
      scan_instructions:
	mov	esi,converted
	mov	al,[edi]
	or	al,al
	jz	no_instruction
	mov	ecx,ebp
	mov	ebx,edi
	add	ebx,ecx
	repe	cmps byte [esi],[edi]
	jb	no_instruction
	je	instruction_ok
      next_instruction:
	mov	edi,ebx
	add	edi,3
	jmp	scan_instructions
      no_instruction:
	mov	esi,edx
	mov	ecx,ebp
	stc
	ret
      lower_case:
	mov	edi,converted
	mov	ebx,characters
      convert_case:
	lods	byte [esi]
	xlat	byte [ebx]
	stos	byte [edi]
	loop	convert_case
      case_ok:
	ret
      instruction_ok:
	lea	esi,[edx+ebp]
	mov	al,[ebx]
	mov	bx,[ebx+1]
	clc
	ret

get_label_id:
	cmp	ecx,100h
	jae	name_too_long
	cmp	byte [esi],'@'
	je	anonymous_label
	cmp	byte [esi],'.'
	jne	standard_label
	mov	al,[esi+1]
	cmp	al,30h
	jb	local_name
	cmp	al,39h
	jbe	invalid_name
      local_name:
	cmp	al,'.'
	je	standard_label
	cmp	[current_locals_prefix],0
	je	standard_label
	push	edi
	push	ecx esi
	mov	edi,[free_additional_memory]
	xor	ax,ax
	stos	word [edi]
	mov	esi,[current_locals_prefix]
	mov	ebx,edi
	lods	byte [esi]
	movzx	ecx,al
	lea	ebp,[edi+ecx]
	cmp	ebp,[additional_memory_end]
	jae	out_of_memory
	rep	movs byte [edi],[esi]
	pop	esi ecx
	add	al,cl
	jc	name_too_long
	lea	ebp,[edi+ecx]
	cmp	ebp,[additional_memory_end]
	jae	out_of_memory
	rep	movs byte [edi],[esi]
	mov	[free_additional_memory],edi
	pop	edi
	push	esi
	movzx	ecx,al
	mov	byte [ebx-1],al
	mov	esi,ebx
	call	get_label_id
	pop	esi
	ret
      anonymous_label:
	cmp	ecx,2
	jne	standard_label
	mov	al,[esi+1]
	mov	ebx,characters
	xlat	byte [ebx]
	cmp	al,'@'
	je	new_anonymous
	cmp	al,'b'
	je	anonymous_back
	cmp	al,'r'
	je	anonymous_back
	cmp	al,'f'
	jne	standard_label
	add	esi,2
	mov	eax,[anonymous_forward]
	or	eax,eax
	jnz	anonymous_ok
	mov	eax,[labels_list]
	sub	eax,8
	mov	[labels_list],eax
	cmp	eax,edi
	jbe	out_of_memory
	mov	dword [eax],0
	mov	[anonymous_forward],eax
      anonymous_ok:
	xor	ebx,ebx
	jmp	make_label_id
      anonymous_back:
	add	esi,2
	mov	eax,[anonymous_reverse]
	or	eax,eax
	jz	invalid_value
	jmp	anonymous_ok
      new_anonymous:
	add	esi,2
	mov	eax,[anonymous_forward]
	or	eax,eax
	jnz	new_anonymous_ok
	mov	eax,[labels_list]
	sub	eax,8
	mov	[labels_list],eax
	cmp	eax,edi
	jbe	out_of_memory
	mov	dword [eax],0
      new_anonymous_ok:
	mov	[anonymous_reverse],eax
	mov	[anonymous_forward],0
	jmp	anonymous_ok
      standard_label:
	cmp	ecx,1
	jne	find_label
	lods	byte [esi]
	cmp	al,'$'
	je	get_current_offset_id
	cmp	al,'%'
	je	get_counter_id
	dec	esi
	jmp	find_label
      get_current_offset_id:
	xor	eax,eax
	ret
      get_counter_id:
	mov	eax,1
	ret
      find_label:
	xor	ebx,ebx
	xor	ebp,ebp
      hash_label:
	rol	ebp,12
	mov	eax,ebp
	shr	ebp,8
	and	eax,1111b
	or	ebp,eax
	movzx	eax,byte [esi+ebx]
	add	ebp,eax
	inc	bl
	cmp	bl,cl
	jb	hash_label
	and	ebp,0FFFFFFh
	shl	ebx,24
	or	ebp,ebx
	mov	[label_hash],ebp
	push	edi
	push	esi
	mov	ebx,esi
	mov	edx,ecx
	mov	eax,[labels_list]
	cmp	eax,[memory_end]
	jne	scan_hashes
      add_label:
	mov	ecx,edx
	pop	esi
	cmp	byte [esi-2],0
	je	label_name_ok
	mov	al,[esi]
	cmp	al,30h
	jb	name_first_char_ok
	cmp	al,39h
	jbe	invalid_name
      name_first_char_ok:
	cmp	ecx,1
	jne	check_for_reserved_word
	cmp	al,'$'
	je	reserved_word
      check_for_reserved_word:
	call	get_instruction
	jnc	reserved_word
	mov	edi,data_directives
	call	get_symbol
	jnc	reserved_word
	mov	edi,symbols
	call	get_symbol
	jnc	reserved_word
	mov	edi,formatter_symbols
	call	get_symbol
	jnc	reserved_word
	sub	esi,2
	mov	edi,operators
	call	get_operator
	or	al,al
	jnz	reserved_word
	mov	edi,single_operand_operators
	call	get_operator
	or	al,al
	jnz	reserved_word
	mov	edi,directive_operators
	call	get_operator
	or	al,al
	jnz	reserved_word
	inc	esi
	movzx	ecx,byte [esi]
	inc	esi
      label_name_ok:
	mov	eax,[labels_list]
	sub	eax,8
	mov	[labels_list],eax
	mov	[eax+4],esi
	mov	ebx,esi
	add	esi,ecx
	mov	edx,[label_hash]
	mov	[eax],edx
	pop	edi
	cmp	eax,edi
	jbe	out_of_memory
	jmp	make_label_id
      reserved_word:
	mov	eax,0Fh
	pop	edi
	ret
	align	16
      scan_hashes:
	cmp	ebp,[eax]
	je	check_label
      next_label:
	add	eax,8
	cmp	eax,[memory_end]
	jne	scan_hashes
	jmp	add_label
      check_label:
	mov	esi,ebx
	mov	ecx,edx
	mov	edi,[eax+4]
	repe	cmps byte [esi],[edi]
	jne	next_label
      label_found:
	add	esp,4
	pop	edi
	mov	ebx,[eax+4]
      make_label_id:
	sub	eax,[memory_end]
	imul	eax,3
	add	eax,[memory_end]
	ret

operators:
 db 1,'+',80h
 db 1,'-',81h
 db 1,'*',90h
 db 1,'/',91h
 db 3,'mod',0A0h
 db 3,'and',0B0h
 db 2,'or',0B1h
 db 3,'xor',0B2h
 db 3,'shl',0C0h
 db 3,'shr',0C1h
 db 0

single_operand_operators:
 db 1,'+',0
 db 1,'-',0D1h
 db 3,'not',0D0h
 db 3,'rva',0E0h
 db 0

directive_operators:
 db 2,'as',86h
 db 2,'at',80h
 db 7,'defined',88h
 db 2,'eq',0F0h
 db 6,'eqtype',0F7h
 db 4,'from',82h
 db 2,'in',0F6h
 db 2,'on',84h
 db 3,'ptr',85h
 db 4,'used',89h
 db 0
