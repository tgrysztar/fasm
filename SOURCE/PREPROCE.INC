
; flat assembler  version 1.07
; Copyright (c) 1999-2000, Tomasz Grysztar
; All rights reserved.

preprocessor:
	mov	eax,[memory_start]
	mov	[source_start],eax
	mov	eax,[additional_memory]
	mov	[files_list],eax
	mov	eax,[additional_memory_end]
	mov	[names_list],eax
	mov	[file_number],1
	mov	[macro_defined],0
	push	[memory_end]
	mov	esi,[input_file]
	mov	edi,[memory_start]
	call	preprocess_file
	jc	main_file_not_found
	mov	[code_start],edi
	pop	[memory_end]
	ret

preprocess_file:
	push	edi
	mov	edi,[additional_memory]
	mov	edx,edi
	xor	al,al
	stos	byte [edi]
      copy_file_name:
	lods	byte [esi]
	cmp	edi,[names_list]
	jae	out_of_memory
	stos	byte [edi]
	or	al,al
	jz	file_name_end
	cmp	edx,[files_list]
	je	copy_file_name
	cmp	al,27h
	jne	copy_file_name
	lods	byte [esi]
	cmp	al,27h
	je	copy_file_name
	dec	esi
      file_name_end:
	mov	byte [edi-1],0
	inc	edx
	call	open
	jc	no_source_file
	mov	al,2
	xor	edx,edx
	call	lseek
	push	eax
	xor	al,al
	xor	edx,edx
	call	lseek
	pop	ecx
	mov	edx,[memory_end]
	mov	ebp,edx
	sub	edx,ecx
	jc	out_of_memory
	mov	esi,edx
	cmp	edx,[esp]
	jbe	out_of_memory
	mov	[memory_end],edx
	mov	eax,edi
	add	eax,4
	cmp	eax,[names_list]
	jae	out_of_memory
	mov	eax,edx
	stos	dword [edi]
	mov	[additional_memory],edi
	push	ebp
	call	read
	call	close
	pop	ebp edi
	movzx	ecx,[file_number]
	inc	[file_number]
	shl	ecx,20
      preprocess_source:
	inc	ecx
	mov	[current_line],edi
	mov	[home_line],edi
	mov	edx,[memory_end]
	xor	bl,bl
	call	convert_line
	mov	[current_line_end],edi
	call	preprocess_instruction
      next_line:
	cmp	byte [esi-1],1Ah
	je	file_end
	cmp	esi,ebp
	jb	preprocess_source
      file_end:
	clc
	ret
      no_source_file:
	add	esp,4
	stc
	ret
      convert_line:
	mov	eax,ecx
	stos	dword [edi]
	mov	al,bl
	stos	byte [edi]
	mov	ebx,edi
      copy_line:
	cmp	edi,edx
	jae	out_of_memory
	cmp	esi,ebp
	je	line_end
	lods	byte [esi]
	cmp	al,0Ah
	je	lf_character
	cmp	al,0Dh
	je	cr_character
	cmp	al,1Ah
	je	line_end
	or	al,al
	jz	line_end
	stos	byte [edi]
	jmp	copy_line
      lf_character:
	lods	byte [esi]
	cmp	al,0Dh
	je	line_end
	dec	esi
	jmp	line_end
      cr_character:
	lods	byte [esi]
	cmp	al,0Ah
	je	line_end
	dec	esi
      line_end:
	xor	al,al
	stos	byte [edi]
	push	esi
	mov	esi,ebx
      check_for_concate:
	lods	byte [esi]
	or	al,al
	jz	line_done
	cmp	al,27h
	je	check_line_string
	cmp	al,'\'
	je	concate
	jmp	check_for_concate
      check_line_string:
	mov	al,[esi-2]
	mov	ebx,characters
	xlatb
	or	al,al
	jz	check_for_concate
      skip_line_string:
	lods	byte [esi]
	or	al,al
	jz	line_done
	cmp	al,27h
	jne	skip_line_string
	lods	byte [esi]
	cmp	al,27h
	je	skip_line_string
	dec	esi
	jmp	check_for_concate
      concate:
	inc	ecx
	mov	ebx,edi
	pop	esi
	cmp	esi,ebp
	jae	unexpected_end_of_file
	jmp	copy_line
      line_done:
	pop	esi
	ret
      skip_spaces:
	lods	byte [esi]
	cmp	al,20h
	je	skip_spaces
	cmp	al,9
	je	skip_spaces
	cmp	al,'\'
	je	skip_concate
	dec	esi
	ret
      skip_concate:
	lods	byte [esi]
	cmp	al,20h
	je	skip_concate
	cmp	al,9
	je	skip_concate
	or	al,al
	jz	skip_spaces
	cmp	al,';'
	jne	unexpected_characters
      skip_concate_comment:
	lods	byte [esi]
	or	al,al
	jnz	skip_concate_comment
	jmp	skip_spaces

preprocess_instruction:
	push	ecx esi edi ebp
	mov	esi,[current_line]
	add	esi,5
	mov	[macro_struc_name],0
      find_directive:
	call	skip_spaces
	lods	byte [esi]
	or	al,al
	jz	instruction_preprocessed
	cmp	al,';'
	je	instruction_preprocessed
	dec	esi
	mov	edx,esi
      find_directive_end:
	lods	byte [esi]
	mov	ah,al
	mov	ebx,characters
	xlatb
	xchg	al,ah
	or	ah,ah
	jz	find_directive_end
	cmp	al,':'
	je	find_directive
	lea	ecx,[esi-1]
	mov	esi,edx
	sub	ecx,esi
	or	ecx,ecx
	jz	instruction_preprocessed
	mov	ebp,ecx
	cmp	[macro_struc_name],0
	jne	no_directive
	mov	edi,preprocessor_directives
	call	get_symbol
	jc	no_directive
	movzx	eax,ax
	add	eax,preprocessor
	jmp	near eax
      no_directive:
	cmp	[macro_defined],0
	je	no_macro
	mov	edi,[files_list]
	mov	eax,1
      get_macro:
	cmp	edi,[additional_memory]
	je	no_macro
	cmp	byte [edi],0
	je	.skip_file
	movzx	ecx,byte [edi]
	inc	edi
	and	cl,7Fh
	mov	ebx,edi
	add	ebx,ecx
	add	ebx,5
	test	byte [edi-1],80h
	jnz	.struc
	cmp	[macro_struc_name],0
	jne	.skip_macro
	jmp	.compare
      .struc:
	cmp	[macro_struc_name],0
	je	.skip_macro
      .compare:
	cmp	ecx,ebp
	jne	.skip_macro
	mov	edx,esi
	repe	cmps byte [esi],[edi]
	je	use_macro
	mov	esi,edx
	mov	edi,ebx
	inc	eax
	jmp	get_macro
      .skip_macro:
	mov	edi,ebx
	inc	eax
	jmp	get_macro
      .skip_file:
	inc	edi
	cmp	byte [edi],0
	jne	.skip_file
	add	edi,5
	inc	eax
	jmp	get_macro
      no_macro:
	cmp	[macro_struc_name],0
	jne	no_macro_struc
	mov	[macro_struc_name],esi
	cmp	ebp,255
	ja	name_too_long
	mov	[macro_struc_name_size],bp
	add	esi,ebp
	call	skip_spaces
	lods	byte [esi]
	or	al,al
	jz	instruction_preprocessed
	cmp	al,';'
	je	instruction_preprocessed
	dec	esi
	mov	edx,esi
	jmp	find_directive_end
      no_macro_struc:
	mov	ecx,ebp
	mov	edi,symbols
	call	get_symbol
	jc	instruction_preprocessed
	cmp	ax,17h
	jne	instruction_preprocessed
	mov	edx,[current_line]
	mov	al,'='
	cmp	byte [edx+4],20h
	jne	set_equ_line_type
	mov	al,'-'
      set_equ_line_type:
	mov	byte [edx+4],al
      instruction_preprocessed:
	pop	ebp edi esi ecx
	ret

include_file:
	call	skip_spaces
	lods	byte [esi]
	cmp	al,27h
	jne	invalid_argument
	mov	edx,[current_line]
	mov	al,':'
	cmp	byte [edx+4],20h
	jne	set_include_line_type
	mov	al,';'
      set_include_line_type:
	mov	byte [edx+4],al
	mov	eax,[additional_memory_end]
	cmp	eax,[names_list]
	jne	illegal_instruction
	mov	edx,esi
      find_include_end:
	lods	byte [esi]
	or	al,al
	jz	missing_end_quote
	cmp	al,27h
	jne	find_include_end
	lods	byte [esi]
	cmp	al,27h
	je	find_include_end
	dec	esi
	call	skip_spaces
	lods	byte [esi]
	or	al,al
	jz	preprocess_included_file
	cmp	al,';'
	je	preprocess_included_file
	jmp	extra_characters_on_line
      preprocess_included_file:
	mov	esi,edx
	mov	edi,[current_line_end]
	call	preprocess_file
	jc	file_not_found
	pop	ebp
	add	esp,4
	pop	esi ecx
	ret
define_struc:
	mov	[macro_struc],1
	jmp	macro_definition
define_macro:
	mov	[macro_struc],0
      macro_definition:
	call	skip_spaces
	lods	byte [esi]
	cmp	al,27h
	je	invalid_name
	dec	esi
	mov	edx,[current_line]
	mov	byte [edx+4],':'
	inc	[file_number]
	mov	eax,[additional_memory_end]
	cmp	eax,[names_list]
	jne	illegal_instruction
	cmp	[macro_defined],0
	je	create_macro
	mov	edx,esi
      .find_name_end:
	lods	byte [esi]
	mov	ebx,characters
	xlatb
	or	al,al
	jz	.find_name_end
	lea	ebp,[esi-1]
	mov	esi,edx
	sub	ebp,esi
	mov	edi,[files_list]
      .find_macro:
	cmp	edi,[additional_memory]
	je	create_macro
	cmp	byte [edi],0
	je	.skip_file
	movzx	ecx,byte [edi]
	inc	edi
	and	cl,7Fh
	mov	ebx,edi
	add	ebx,ecx
	add	ebx,5
	test	byte [edi-1],80h
	jnz	.struc
	cmp	[macro_struc],0
	jne	.skip_macro
	jmp	.compare
      .struc:
	and	cl,7Fh
	cmp	[macro_struc],0
	je	.skip_macro
      .compare:
	cmp	ecx,ebp
	jne	.skip_macro
	mov	edx,esi
	repe	cmps byte [esi],[edi]
	je	redefine_macro
	mov	esi,edx
	mov	edi,ebx
	jmp	.find_macro
      .skip_macro:
	mov	edi,ebx
	jmp	.find_macro
      .skip_file:
	inc	edi
	cmp	byte [edi],0
	jne	.skip_file
	add	edi,5
	jmp	.find_macro
      redefine_macro:
	mov	ebp,esi
	xchg	ebp,[edi+1]
	mov	esi,edx
	mov	al,[ebx]
	shr	al,7
	xchg	al,[macro_struc]
	and	byte [ebx],7Fh
	or	al,al
	jz	add_macro
	or	byte [ebx],80h
	jmp	add_macro
      create_macro:
	xor	ebp,ebp
      add_macro:
	mov	[macro_defined],1
	mov	edx,[additional_memory]
	mov	ebx,edx
	inc	edx
      copy_macro_name:
	lods	byte [esi]
	mov	ah,al
	push	ebx
	mov	ebx,characters
	xlatb
	xchg	al,ah
	pop	ebx
	or	ah,ah
	jnz	macro_name_end
	mov	[edx],al
	inc	edx
	jmp	copy_macro_name
      macro_name_end:
	dec	esi
	mov	edi,edx
	sub	edx,[additional_memory]
	dec	edx
	cmp	edx,80h
	jae	name_too_long
	cmp	[macro_struc],0
	je	macro_name_length_ok
	or	dl,80h
      macro_name_length_ok:
	mov	[ebx],dl
	lea	ebx,[edi+5]
	mov	[additional_memory],ebx
      macro_name_ok:
	xor	al,al
	stos	byte [edi]
	mov	eax,esi
	or	ebp,ebp
	jz	store_macro_address
	mov	eax,ebp
      store_macro_address:
	stos	dword [edi]
	mov	edi,[current_line_end]
	mov	[multi_macro_current],0
      find_macro_argument:
	call	skip_spaces
	lods	byte [esi]
	cmp	al,27h
	je	invalid_name
	dec	esi
	or	al,al
	jz	find_next_macro_argument
	cmp	al,';'
	je	find_next_macro_argument
	cmp	al,'{'
	je	find_next_macro_argument
	inc	esi
	cmp	al,'['
	jne	skip_macro_argument
	cmp	[multi_macro_current],0
	jne	invalid_name
	mov	[multi_macro_current],1
	call	skip_spaces
	cmp	byte [esi],']'
	je	invalid_macro_arguments
	jmp	find_macro_argument
      skip_macro_argument:
	mov	ebx,characters
	xlatb
	or	al,al
	jnz	macro_argument_end
	lods	byte [esi]
	jmp	skip_macro_argument
      macro_argument_end:
	dec	esi
	cmp	byte [esi-1],20h
	je	invalid_macro_arguments
	cmp	byte [esi-1],9
	je	invalid_macro_arguments
      find_next_macro_argument:
	call	skip_spaces
	lods	byte [esi]
	cmp	al,','
	je	next_macro_argument
	cmp	[multi_macro_current],0
	je	macro_arguments_end
	cmp	al,']'
	jne	invalid_macro_arguments
	call	skip_spaces
	lods	byte [esi]
      macro_arguments_end:
	or	al,al
	jz	find_macro_start
	cmp	al,';'
	je	find_macro_start
	cmp	al,'{'
	je	find_macro_end
	jmp	invalid_macro_arguments
      next_macro_argument:
	call	skip_spaces
	lods	byte [esi]
	dec	esi
	or	al,al
	jz	invalid_macro_arguments
	cmp	al,';'
	je	invalid_macro_arguments
	jmp	find_macro_argument
      find_macro_start:
	mov	ebp,esp
	call	get_macro_line
	jc	unexpected_end_of_file
	add	esi,5
	call	skip_spaces
	lods	byte [esi]
	or	al,al
	jz	find_macro_start
	cmp	al,';'
	je	find_macro_start
	cmp	al,'{'
	jne	unexpected_characters
      find_macro_end:
	call	skip_spaces
	lods	byte [esi]
	cmp	al,27h
	je	skip_string_in_macro
	mov	ah,al
	mov	ebx,characters
	xlatb
	xchg	al,ah
	or	ah,ah
	jnz	special_character_in_macro
	jmp	find_macro_end
      skip_string_in_macro:
	lods	byte [esi]
	or	al,al
	jz	missing_end_quote
	cmp	al,27h
	jne	skip_string_in_macro
	jmp	find_macro_end
      special_character_in_macro:
	cmp	al,';'
	jne	check_macro_line_end
      skip_comment_in_macro:
	lods	byte [esi]
	or	al,al
	jnz	skip_comment_in_macro
	jmp	macro_line_end
      check_macro_line_end:
	or	al,al
	jnz	check_macro_end
      macro_line_end:
	mov	ebp,esp
	call	get_macro_line
	jc	unexpected_end_of_file
	add	esi,5
	jmp	find_macro_end
      check_macro_end:
	cmp	al,'}'
	je	macro_last_line
	lods	byte [esi]
	cmp	al,27h
	je	skip_string_in_macro
	dec	esi
	jmp	find_macro_end
      macro_last_line:
	call	skip_spaces
	lods	byte [esi]
	or	al,al
	jz	macro_after_end_new_line
	cmp	al,';'
	je	macro_after_end_comment
	cmp	al,'{'
	je	find_macro_end
	jmp	extra_characters_on_line
      check_macro_after_end:
	call	skip_spaces
	lods	byte [esi]
	or	al,al
	jz	macro_after_end_new_line
	cmp	al,';'
	je	macro_after_end_comment
	cmp	al,'{'
	je	find_macro_end
      macro_definition_ok:
	pop	ebp
	add	esp,4
	pop	esi ecx
	ret
      macro_after_end_comment:
	lods	byte [esi]
	or	al,al
	jnz	macro_after_end_comment
      macro_after_end_new_line:
	mov	esi,[esp+8]
	call	skip_spaces
	lods	byte [esi]
	cmp	al,0Dh
	je	macro_after_end_new_line_get
	cmp	al,0Ah
	je	macro_after_end_new_line_get
	cmp	al,1Ah
	je	macro_after_end_new_line_get
	or	al,al
	jz	macro_after_end_new_line_get
	cmp	al,';'
	je	macro_after_end_new_line_get
	cmp	al,'{'
	jne	macro_definition_ok
      macro_after_end_new_line_get:
	mov	ebp,esp
	call	get_macro_line
	jc	macro_definition_ok
	add	esi,5
	jmp	check_macro_after_end
      get_macro_line:
	push	edi
	mov	[current_line],edi
	mov	[home_line],edi
	mov	esi,[ebp+8]
	mov	ecx,[ebp+0Ch]
	inc	ecx
	mov	edx,[memory_end]
	mov	bl,'{'
	push	ebp
	mov	ebp,[ebp]
	cmp	esi,ebp
	jae	no_next_macro_line
	call	convert_line
	cmp	byte [esi-1],1Ah
	je	no_next_macro_line
	pop	ebp
	mov	[ebp+0Ch],ecx
	mov	[ebp+8],esi
	pop	esi
	clc
	ret
      no_next_macro_line:
	pop	ebp
	pop	esi
	stc
	ret

purge_macro:
	call	skip_spaces
	lods	byte [esi]
	cmp	al,27h
	je	invalid_name
	dec	esi
	mov	edx,[current_line]
	mov	byte [edx+4],':'
	mov	eax,[additional_memory_end]
	cmp	eax,[names_list]
	jne	illegal_instruction
	mov	edx,esi
      .find_name_end:
	lods	byte [esi]
	mov	ebx,characters
	xlatb
	or	al,al
	jz	.find_name_end
	lea	ebp,[esi-1]
	push	ebp
	mov	esi,edx
	sub	ebp,esi
	mov	edi,[files_list]
      .find_macro:
	cmp	edi,[additional_memory]
	je	purge_done
	cmp	byte [edi],0
	je	.skip_file
	movzx	ecx,byte [edi]
	inc	edi
	mov	ebx,edi
	add	ebx,ecx
	add	ebx,5
	cmp	ecx,ebp
	jne	.skip_macro
	mov	edx,esi
	repe	cmps byte [esi],[edi]
	je	disable_macro
	mov	esi,edx
	mov	edi,ebx
	jmp	.find_macro
      .skip_macro:
	mov	edi,ebx
	jmp	.find_macro
      .skip_file:
	inc	edi
	cmp	byte [edi],0
	jne	.skip_file
	add	edi,5
	jmp	.find_macro
      disable_macro:
	sub	ebx,ebp
	mov	byte [ebx-6],0
	jmp	purge_done
      purge_done:
	pop	esi
	call	skip_spaces
	lods	byte [esi]
	cmp	al,','
	je	purge_macro
	or	al,al
	jz	purge_ok
	cmp	al,';'
	jne	extra_characters_on_line
      purge_ok:
	pop	ebp edi esi ecx
	ret

use_macro:
	mov	ebx,edi
	sub	ebx,ebp
	dec	ebx
	xor	dl,dl
	xchg	dl,[ebx]
	push	dx
	push	ebx
	push	[names_list]
	shl	eax,20
	push	eax
	mov	edx,[current_line]
	mov	[esp+12h],edx
	mov	al,':'
	cmp	byte [edx+4],20h
	jne	set_macro_line_type
	mov	al,';'
      set_macro_line_type:
	mov	byte [edx+4],al
	mov	ebx,esi
	mov	esi,[edi+1]
	mov	edi,[names_list]
	dec	edi
	mov	byte [edi],0
	mov	[names_list],edi
	mov	[multi_macro_current],0
	mov	[multi_macro_count],1
      find_argument:
	call	skip_spaces
	lods	byte [esi]
	cmp	al,'['
	jne	argument_found
	mov	[multi_macro_current],1
	mov	[multi_macro_arguments],esi
	jmp	find_argument
      argument_found:
	dec	esi
	or	al,al
	jz	macro_without_arguments
	cmp	al,'{'
	je	macro_without_arguments
	cmp	al,';'
	je	macro_without_arguments
	mov	edx,esi
	jmp	find_argument_end
      macro_without_arguments:
	xchg	esi,ebx
	call	skip_spaces
	mov	al,[esi]
	or	al,al
	jz	no_arguments
	cmp	al,'{'
	je	no_arguments
	cmp	al,';'
	je	no_arguments
	jmp	extra_characters_on_line
      no_arguments:
	xchg	esi,ebx
	jmp	find_macro_instructions
      find_argument_end:
	lods	byte [esi]
	cmp	al,','
	je	argument_end
	or	al,al
	jz	argument_end
	cmp	al,';'
	je	argument_end
	cmp	al,'{'
	je	argument_end
	cmp	al,']'
	je	argument_end
	cmp	al,'\'
	je	argument_end
	cmp	al,9
	je	argument_end
	cmp	al,20h
	jne	find_argument_end
      argument_end:
	dec	esi
	xchg	esi,ebx
	call	skip_spaces
	mov	ebp,esi
      find_argument_value_end:
	lods	byte [esi]
	cmp	al,27h
	je	skip_argument_string
	cmp	al,','
	je	argument_value_end
	or	al,al
	jz	argument_value_end
	cmp	al,';'
	je	argument_value_end
	cmp	al,9
	je	argument_spaces
	cmp	al,'\'
	je	argument_spaces
	cmp	al,20h
	jne	find_argument_value_end
      argument_spaces:
	push	esi
	call	skip_spaces
	lods	byte [esi]
	cmp	al,','
	je	no_argument_spaces
	or	al,al
	jz	no_argument_spaces
	cmp	al,';'
	je	no_argument_spaces
	add	esp,4
	dec	esi
	jmp	find_argument_value_end
      no_argument_spaces:
	pop	esi
	jmp	argument_value_end
      skip_argument_string:
	lods	byte [esi]
	cmp	al,27h
	jne	skip_argument_string
	jmp	find_argument_value_end
      argument_value_end:
	dec	esi
	xchg	esi,ebx
	push	ebx esi
	mov	edi,[names_list]
	sub	esi,edx
	sub	ebx,ebp
	sub	edi,esi
	sub	edi,ebx
	sub	edi,4
	cmp	edi,[additional_memory]
	jbe	out_of_memory
	mov	[names_list],edi
	mov	ecx,esi
	cmp	ecx,100h
	jae	name_too_long
	mov	esi,edx
	mov	al,cl
	stos	byte [edi]
	mov	ax,[multi_macro_current]
	stos	word [edi]
	rep	movs byte [edi],[esi]
	mov	ecx,ebx
	cmp	ecx,100h
	jae	argument_too_long
	mov	esi,ebp
	mov	al,cl
	stos	byte [edi]
	rep	movs byte [edi],[esi]
	pop	esi ebx
	call	skip_spaces
	lods	byte [esi]
	xchg	esi,ebx
	cmp	al,','
	je	next_argument
	dec	ebx
	call	skip_spaces
	lods	byte [esi]
	or	al,al
	jz	arguments_ok
	cmp	al,';'
	je	arguments_ok
	cmp	[multi_macro_current],0
	je	invalid_macro_arguments
	mov	ebx,esi
	inc	[multi_macro_count]
	inc	[multi_macro_current]
	mov	esi,[multi_macro_arguments]
	jmp	find_argument
      next_argument:
	call	skip_spaces
	lods	byte [esi]
	cmp	al,','
	je	next_argument_ok
	dec	esi
      next_argument_ok:
	xchg	esi,ebx
	jmp	find_argument
      arguments_ok:
	mov	esi,ebx
	call	skip_spaces
	cmp	[multi_macro_current],0
	je	find_macro_instructions
	lods	byte [esi]
	cmp	al,']'
	jne	invalid_macro_arguments
	mov	[multi_macro_current],1
	mov	[macro_instructions],esi
	mov	eax,[esp]
	mov	[multi_macro_line],eax
      find_macro_instructions:
	call	skip_spaces
	lods	byte [esi]
	cmp	al,'{'
	je	macro_instructions_start
	cmp	al,';'
	je	skip_macro_comment
	or	al,al
	jnz	unexpected_characters
	add	esi,5
	jmp	find_macro_instructions
      skip_macro_comment:
	lods	byte [esi]
	or	al,al
	jnz	skip_macro_comment
	add	esi,5
	jmp	find_macro_instructions
      macro_instructions_start:
	mov	edi,[current_line_end]
	mov	[current_line],edi
	mov	eax,[esp]
	stos	dword [edi]
	mov	al,20h
	stos	byte [edi]
	inc	dword [esp]
	mov	[macro_directive_allowed],1
	mov	ebx,locals_counter
	call	increase_counter
	cmp	[macro_struc_name],0
	je	macro_process
	mov	cx,[macro_struc_name_size]
	or	ch,ch
	jnz	macro_process
	push	esi
	mov	esi,[macro_struc_name]
	movzx	ecx,cl
	rep	movs byte [edi],[esi]
	mov	al,':'
	stos	byte [edi]
	mov	byte [macro_struc_name_size+1],1
	pop	esi
      macro_process:
	lods	byte [esi]
	stos	byte [edi]
	cmp	al,20h
	je	macro_process
	cmp	al,9
	je	macro_process
	dec	edi
	cmp	al,'\'
	je	macro_concate
	dec	esi
	jmp	macro_instruction
      macro_concate:
	lods	byte [esi]
	or	al,al
	jnz	macro_concate
	jmp	macro_process
      macro_instruction:
	lods	byte [esi]
	stos	byte [edi]
	mov	ah,al
	mov	ebx,characters
	xlatb
	xchg	al,ah
	or	ah,ah
	jnz	special_character
	cmp	al,27h
	je	skip_string
	dec	esi
	dec	edi
	cmp	al,'.'
	jne	no_struc_field
	cmp	byte [esi+1],'.'
	je	no_struc_field
	cmp	[macro_struc_name],0
	je	no_struc_field
	push	esi
	mov	esi,[macro_struc_name]
	movzx	ecx,byte [macro_struc_name_size]
	rep	movs byte [edi],[esi]
	pop	esi
      no_struc_field:
	push	edi
	mov	edi,[names_list]
	mov	ebx,edi
	mov	edx,esi
      check_macro_variable:
	movzx	ecx,byte [edi]
	jecxz	no_macro_variable
	inc	edi
	mov	ax,[edi]
	add	edi,2
	mov	ebp,edi
	add	ebp,ecx
	or	ax,ax
	jz	compare_macro_variable
	cmp	ax,[multi_macro_current]
	jne	next_macro_variable
      compare_macro_variable:
	repe	cmps byte [esi],[edi]
	jnz	next_macro_variable
	mov	al,[esi]
	mov	ebx,characters
	xlatb
	or	al,al
	jz	next_macro_variable
      macro_variable_ok:
	pop	edi
	mov	ebx,ebp
	movzx	ecx,byte [ebx]
	inc	ebx
	or	ecx,ecx
	jz	macro_process
      copy_macro_variable:
	mov	al,[ebx]
	inc	ebx
	stos	byte [edi]
	loop	copy_macro_variable
	jmp	macro_process
      next_macro_variable:
	mov	edi,ebp
	movzx	ecx,byte [edi]
	inc	edi
	add	edi,ecx
	mov	esi,edx
	jmp	check_macro_variable
      no_macro_variable:
	pop	edi
	mov	edx,esi
	mov	ebp,edi
      check_for_macro_directive:
	lods	byte [esi]
	stos	byte [edi]
	mov	ebx,characters
	xlatb
	or	al,al
	jz	check_for_macro_directive
	dec	esi
	mov	ecx,esi
	sub	ecx,edx
	push	esi
	cmp	[macro_directive_allowed],1
	jne	no_macro_directive
	cmp	byte [esi],':'
	je	no_macro_directive
	mov	[macro_directive_allowed],0
	push	edi ebp
	mov	esi,edx
	mov	edi,macro_directives
	call	get_symbol
	pop	ebp edi
	jc	no_macro_directive
	add	esp,4
	mov	edi,ebp
	movzx	eax,ax
	add	eax,preprocessor
	jmp	near eax
      local_symbols:
	call	skip_spaces
	lods	byte [esi]
	or	al,al
	jz	invalid_argument
	cmp	al,';'
	je	invalid_argument
	lea	edx,[esi-1]
      get_local_name:
	mov	ebx,characters
	xlatb
	or	al,al
	jnz	local_name_end
	lods	byte [esi]
	jmp	get_local_name
      local_name_end:
	dec	esi
	mov	ecx,esi
	sub	ecx,edx
	or	ecx,ecx
	jz	invalid_argument
	cmp	ecx,100h
	jae	name_too_long
	push	edi
	mov	edi,[names_list]
	sub	edi,ecx
	sub	edi,4+5
	sub	edi,ecx
	cmp	edi,[additional_memory]
	jbe	out_of_memory
	mov	[names_list],edi
	mov	al,cl
	stos	byte [edi]
	mov	ax,[multi_macro_current]
	stos	word [edi]
	push	cx
	mov	esi,edx
	rep	movs byte [edi],[esi]
	pop	cx
	mov	al,5
	add	al,cl
	jc	name_too_long
	stos	byte [edi]
	mov	esi,edx
	rep	movs byte [edi],[esi]
	mov	al,'?'
	stos	byte [edi]
	mov	eax,[locals_counter]
	stos	dword [edi]
	pop	edi
	call	skip_spaces
	lods	byte [esi]
	cmp	al,','
	je	local_symbols
	or	al,al
	jz	macro_directive_end
	cmp	al,';'
	je	macro_directive_comment
	jmp	invalid_argument
      macro_directive_comment:
	lods	byte [esi]
	or	al,al
	jnz	macro_directive_comment
      macro_directive_end:
	stos	byte [edi]
	jmp	next_macro_line
      no_macro_directive:
	pop	esi
	dec	edi
	jmp	macro_process
      skip_string:
	lods	byte [esi]
	stos	byte [edi]
	or	al,al
	jz	missing_end_quote
	cmp	al,27h
	jne	skip_string
	jmp	macro_process
      skip_comment:
	lods	byte [esi]
	stos	byte [edi]
	or	al,al
	jnz	skip_comment
	jmp	next_macro_line
      special_character:
	cmp	al,';'
	je	skip_comment
	or	al,al
	jnz	check_for_end
      next_macro_line:
	add	esi,5
	mov	[current_line_end],edi
	mov	ebp,[esp+0Ah]
	call	macro_preprocess_instruction
	mov	[current_line],edi
	mov	eax,[esp]
	stos	dword [edi]
	mov	al,20h
	stos	byte [edi]
	inc	dword [esp]
	mov	[macro_directive_allowed],1
	jmp	macro_process
      check_for_end:
	cmp	al,'}'
	jne	macro_process
	dec	edi
	xor	al,al
	stos	byte [edi]
	mov	[current_line_end],edi
	mov	ecx,[esp+1Ah]
	mov	ebp,[esp+0Eh]
	call	macro_preprocess_instruction
	mov	ax,[multi_macro_current]
	or	ax,ax
	jz	check_for_next_macro_part
	inc	ax
	cmp	ax,[multi_macro_count]
	ja	check_for_next_macro_part
	mov	[multi_macro_current],ax
	mov	[current_line],edi
	mov	eax,[esp]
	stos	dword [edi]
	mov	al,20h
	stos	byte [edi]
	mov	eax,[multi_macro_line]
	mov	[esp],eax
	mov	[macro_directive_allowed],1
	mov	esi,[macro_instructions]
	jmp	find_macro_instructions
      check_for_next_macro_part:
	call	skip_spaces
	lods	byte [esi]
	or	al,al
	jz	macro_end_new_line
	cmp	al,';'
	je	macro_end_comment
	cmp	al,'{'
	jne	macro_end
	dec	esi
	mov	[macro_instructions],esi
	mov	eax,[esp]
	mov	[multi_macro_line],eax
	mov	[multi_macro_current],1
	jmp	find_macro_instructions
      macro_end_comment:
	lods	byte [esi]
	or	al,al
	jnz	macro_end_comment
      macro_end_new_line:
	add	esi,5
	jmp	check_for_next_macro_part
      macro_end:
	dec	esi
	add	esp,4
	pop	[names_list]
	pop	esi
	pop	ax
	mov	[esi],al
	pop	ebp
	pop	[current_line]
	pop	esi ecx
	ret
      macro_preprocess_instruction:
	push	[multi_macro_current]
	push	[multi_macro_count]
	push	[multi_macro_line]
	push	[macro_instructions]
	push	[macro_struc_name]
	push	[macro_struc_name_size]
	call	preprocess_instruction
	pop	[macro_struc_name_size]
	pop	[macro_struc_name]
	pop	[macro_instructions]
	pop	[multi_macro_line]
	pop	[multi_macro_count]
	pop	[multi_macro_current]
	ret
      increase_counter:
	inc	byte [ebx+3]
	cmp	byte [ebx+3],'9'
	jbe	counter_ok
	mov	byte [ebx+3],'0'
	inc	byte [ebx+2]
	cmp	byte [ebx+2],'9'
	jbe	counter_ok
	mov	byte [ebx+2],'0'
	inc	byte [ebx+1]
	cmp	byte [ebx+1],'9'
	jbe	counter_ok
	mov	byte [ebx+1],'0'
	inc	byte [ebx]
      counter_ok:
	ret

file_number dw ?
files_list dd ?
names_list dd ?
macro_defined db ?
macro_directive_allowed db ?
macro_instructions dd ?
macro_struc db ?
macro_struc_name dd ?
macro_struc_name_size dw ?
multi_macro_current dw ?
multi_macro_count dw ?
multi_macro_arguments dd ?
multi_macro_line dd ?
locals_counter dd '0000'
current_line_end dd ?
