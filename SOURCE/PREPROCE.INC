
; flat assembler  version 1.0
; Copyright (c) 1999-2000, Tomasz Grysztar
; All rights reserved.

preprocessor:
	mov	eax,[memory_start]
	mov	[source_start],eax
	mov	eax,[conventional_memory]
	mov	[files_list],eax
	mov	eax,[conventional_memory_end]
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
	mov	edi,[conventional_memory]
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
	mov	[conventional_memory],edi
	call	read
	call	close
	pop	edi
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
	add	sp,4
	stc
	ret
      convert_line:
	mov	eax,ecx
	stos	dword [edi]
	mov	al,bl
	stos	byte [edi]
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
	ret

preprocess_instruction:
	push	ecx esi edi ebp
	mov	esi,[current_line]
	add	esi,5
      find_directive:
	lods	byte [esi]
	cmp	al,20h
	je	find_directive
	cmp	al,9
	je	find_directive
	or	al,al
	jz	instruction_preprocessed
	cmp	al,';'
	je	instruction_preprocessed
	dec	esi
	mov	edx,esi
      find_directive_end:
	lods	byte [esi]
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scas byte [edi]
	jne	find_directive_end
	cmp	al,':'
	je	find_directive
	lea	ecx,[esi-1]
	mov	esi,edx
	sub	ecx,esi
	or	ecx,ecx
	jz	instruction_preprocessed
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
	cmp	edi,[conventional_memory]
	je	no_macro
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
	je	use_macro
	mov	esi,edx
	mov	edi,ebx
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
	add	esi,ebp
      find_equ_directive:
	lods	byte [esi]
	cmp	al,20h
	je	find_equ_directive
	cmp	al,9
	je	find_equ_directive
	or	al,al
	jz	instruction_preprocessed
	cmp	al,';'
	je	instruction_preprocessed
	dec	esi
	mov	edx,esi
      find_equ_symbol_end:
	lods	byte [esi]
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scas byte [edi]
	jne	find_equ_symbol_end
	lea	ecx,[esi-1]
	mov	esi,edx
	sub	ecx,esi
	jecxz	instruction_preprocessed
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
	lods	byte [esi]
	cmp	al,20h
	je	include_file
	cmp	al,9
	je	include_file
	cmp	al,27h
	jne	invalid_argument
	mov	edx,[current_line]
	mov	al,':'
	cmp	byte [edx+4],20h
	jne	set_include_line_type
	mov	al,';'
      set_include_line_type:
	mov	byte [edx+4],al
	mov	eax,[conventional_memory_end]
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
      check_include_line:
	lods	byte [esi]
	or	al,al
	jz	preprocess_included_file
	cmp	al,';'
	je	preprocess_included_file
	cmp	al,20h
	je	check_include_line
	cmp	al,9
	je	check_include_line
	jmp	extra_characters_on_line
      preprocess_included_file:
	mov	esi,edx
	mov	edi,[current_line_end]
	call	preprocess_file
	jc	file_not_found
	pop	ebp
	add	sp,4
	pop	esi ecx
	ret
define_macro:
	lods	byte [esi]
	cmp	al,20h
	je	define_macro
	cmp	al,9
	je	define_macro
	cmp	al,27h
	je	invalid_name
	dec	esi
	mov	edx,[current_line]
	mov	byte [edx+4],':'
	mov	eax,[conventional_memory_end]
	cmp	eax,[names_list]
	jne	illegal_instruction
	cmp	[macro_defined],0
	je	create_macro
	mov	edx,esi
      .find_name_end:
	lods	byte [esi]
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scas byte [edi]
	jne	.find_name_end
	lea	ebp,[esi-1]
	mov	esi,edx
	sub	ebp,esi
	mov	edi,[files_list]
      .find_macro:
	cmp	edi,[conventional_memory]
	je	create_macro
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
	push	esi edi
	sub	ebx,ebp
	sub	ebx,6
	mov	esi,[conventional_memory]
	mov	edi,esi
	add	edi,ebp
	add	edi,6
	cmp	edi,[names_list]
	jae	out_of_memory
	mov	[conventional_memory],edi
	std
	mov	ecx,esi
	sub	ecx,ebx
	dec	edi
	dec	esi
	rep	movs byte [edi],[esi]
	cld
	pop	edi esi
	jmp	macro_name_ok
      create_macro:
	mov	[macro_defined],1
	mov	edx,[conventional_memory]
	mov	ebx,edx
	inc	edx
      copy_macro_name:
	lods	byte [esi]
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scas byte [edi]
	je	macro_name_end
	mov	[edx],al
	inc	edx
	jmp	copy_macro_name
      macro_name_end:
	dec	esi
	mov	edi,edx
	sub	edx,[conventional_memory]
	dec	edx
	cmp	edx,100h
	jae	name_too_long
	mov	[ebx],dl
	lea	ebx,[edi+5]
	mov	[conventional_memory],ebx
	inc	[file_number]
      macro_name_ok:
	xor	al,al
	stos	byte [edi]
	mov	eax,esi
	stos	dword [edi]
	mov	edi,[current_line_end]
      find_macro_argument:
	lods	byte [esi]
	cmp	al,20h
	je	find_macro_argument
	cmp	al,9
	je	find_macro_argument
	cmp	al,27h
	je	invalid_name
	or	al,al
	jz	find_macro_start
	cmp	al,';'
	je	find_macro_start
	cmp	al,'{'
	je	find_macro_end
      skip_macro_argument:
	mov	ebx,edi
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scas byte [edi]
	mov	edi,ebx
	je	macro_argument_end
	lods	byte [esi]
	jmp	skip_macro_argument
      macro_argument_end:
	dec	esi
	cmp	byte [esi-1],20h
	je	invalid_macro_arguments
	cmp	byte [esi-1],9
	je	invalid_macro_arguments
      find_next_macro_argument:
	lods	byte [esi]
	cmp	al,20h
	je	find_next_macro_argument
	cmp	al,9
	je	find_next_macro_argument
	or	al,al
	jz	find_macro_start
	cmp	al,';'
	je	find_macro_start
	cmp	al,'{'
	je	find_macro_end
	cmp	al,','
	je	next_macro_argument
	jmp	invalid_macro_arguments
      next_macro_argument:
	lods	byte [esi]
	cmp	al,20h
	je	next_macro_argument
	cmp	al,9
	je	next_macro_argument
	dec	esi
	or	al,al
	jz	invalid_macro_arguments
	cmp	al,';'
	je	invalid_macro_arguments
	jmp	find_macro_argument
      find_macro_start:
	mov	ebp,esp
	call	get_macro_line
	add	esi,5
      check_macro_start:
	lods	byte [esi]
	cmp	al,20h
	je	check_macro_start
	cmp	al,9
	je	check_macro_start
	or	al,al
	jz	find_macro_start
	cmp	al,';'
	je	find_macro_start
	cmp	al,'{'
	jne	unexpected_characters
      find_macro_end:
	lods	byte [esi]
	mov	ebx,edi
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scas byte [edi]
	mov	edi,ebx
	je	special_character_in_macro
	jmp	find_macro_end
      skip_string_in_macro:
	lods	byte [esi]
	or	al,al
	jz	missing_end_quote
	cmp	al,27h
	jne	skip_string_in_macro
	lods	byte [esi]
	cmp	al,27h
	je	skip_string_in_macro
	dec	esi
	jmp	find_macro_end
      special_character_in_macro:
	or	al,al
	jnz	check_macro_end
	mov	ebp,esp
	call	get_macro_line
	add	esi,5
	jmp	find_macro_end
      check_macro_end:
	cmp	al,'}'
	je	check_macro_after_end
	lods	byte [esi]
	cmp	al,27h
	je	skip_string_in_macro
	dec	esi
	jmp	find_macro_end
      check_macro_after_end:
	lods	byte [esi]
	cmp	al,20h
	je	check_macro_after_end
	cmp	al,9
	je	check_macro_after_end
	or	al,al
	jz	macro_definition_ok
	cmp	al,';'
	je	macro_definition_ok
	jmp	extra_characters_on_line
      macro_definition_ok:
	pop	ebp
	add	sp,4
	pop	esi ecx
	ret
      get_macro_line:
	push	edi
	mov	[current_line],edi
	mov	[home_line],edi
	mov	esi,[ebp+8]
	mov	ecx,[ebp+Ch]
	inc	ecx
	mov	[ebp+Ch],ecx
	mov	edx,[memory_end]
	mov	bl,'{'
	push	ebp
	mov	ebp,[ebp]
	cmp	esi,ebp
	jae	unexpected_end_of_file
	call	convert_line
	cmp	byte [esi-1],1Ah
	je	unexpected_end_of_file
	pop	ebp
	mov	[ebp+8],esi
	pop	esi
	ret
purge_macro:
	lods	byte [esi]
	cmp	al,20h
	je	purge_macro
	cmp	al,9
	je	purge_macro
	cmp	al,27h
	je	invalid_name
	dec	esi
	mov	edx,[current_line]
	mov	byte [edx+4],':'
	mov	eax,[conventional_memory_end]
	cmp	eax,[names_list]
	jne	illegal_instruction
	mov	edx,esi
      .find_name_end:
	lods	byte [esi]
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scas byte [edi]
	jne	.find_name_end
	lea	ebp,[esi-1]
	push	ebp
	mov	esi,edx
	sub	ebp,esi
	mov	edi,[files_list]
      .find_macro:
	cmp	edi,[conventional_memory]
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
      purge_next:
	lods	byte [esi]
	cmp	al,20h
	je	purge_next
	cmp	al,9
	je	purge_next
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
      find_argument:
	lods	byte [esi]
	cmp	al,20h
	je	find_argument
	cmp	al,9
	je	find_argument
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
      .check:
	lods	byte [esi]
	cmp	al,20h
	je	.check
	cmp	al,9
	je	.check
	dec	esi
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
	cmp	al,9
	je	argument_end
	cmp	al,20h
	jne	find_argument_end
      argument_end:
	dec	esi
	xchg	esi,ebx
      find_argument_value:
	lods	byte [esi]
	cmp	al,20h
	je	find_argument_value
	cmp	al,9
	je	find_argument_value
	dec	esi
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
	cmp	al,20h
	jne	find_argument_value_end
      argument_spaces:
	push	esi
      check_argument_spaces:
	lods	byte [esi]
	cmp	al,20h
	je	check_argument_spaces
	cmp	al,9
	je	check_argument_spaces
	cmp	al,','
	je	no_argument_spaces
	or	al,al
	jz	no_argument_spaces
	cmp	al,';'
	je	no_argument_spaces
	add	sp,4
	dec	esi
	jmp	find_argument_value_end
      no_argument_spaces:
	pop	esi
	jmp	argument_value_end
      skip_argument_string:
	lods	byte [esi]
	cmp	al,27h
	jne	skip_argument_string
	lods	byte [esi]
	cmp	al,27h
	je	skip_argument_string
	dec	esi
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
	sub	edi,2
	cmp	edi,[conventional_memory]
	jbe	out_of_memory
	mov	[names_list],edi
	mov	ecx,esi
	cmp	ecx,100h
	jae	name_too_long
	mov	esi,edx
	mov	al,cl
	stos	byte [edi]
	rep	movs byte [edi],[esi]
	mov	ecx,ebx
	cmp	ecx,100h
	jae	argument_too_long
	mov	esi,ebp
	mov	al,cl
	stos	byte [edi]
	rep	movs byte [edi],[esi]
	pop	esi ebx
      check_for_next_argument:
	lods	byte [esi]
	cmp	al,20h
	je	check_for_next_argument
	cmp	al,9
	je	check_for_next_argument
	xchg	esi,ebx
	cmp	al,','
	je	next_argument
	dec	ebx
      check_arguments:
	lods	byte [esi]
	or	al,al
	jz	arguments_ok
	cmp	al,';'
	je	arguments_ok
	cmp	al,20h
	je	check_arguments
	cmp	al,9
	je	check_arguments
	jmp	invalid_macro_arguments
      next_argument:
	lods	byte [esi]
	cmp	al,20h
	je	next_argument
	cmp	al,9
	je	next_argument
	cmp	al,','
	je	next_argument_ok
	dec	esi
      next_argument_ok:
	xchg	esi,ebx
	jmp	find_argument
      arguments_ok:
	mov	esi,ebx
      find_macro_instructions:
	lods	byte [esi]
	cmp	al,20h
	je	find_macro_instructions
	cmp	al,9
	je	find_macro_instructions
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
	inc	byte [locals_counter+3]
	cmp	byte [locals_counter+3],'9'
	jbe	macro_process
	mov	byte [locals_counter+3],'0'
	inc	byte [locals_counter+2]
	cmp	byte [locals_counter+2],'9'
	jbe	macro_process
	mov	byte [locals_counter+2],'0'
	inc	byte [locals_counter+1]
	cmp	byte [locals_counter+1],'9'
	jbe	macro_process
	mov	byte [locals_counter+1],'0'
	inc	byte [locals_counter]
      macro_process:
	lods	byte [esi]
	stos	byte [edi]
	mov	ebx,edi
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scas byte [edi]
	mov	edi,ebx
	je	special_character
	cmp	al,27h
	je	skip_string
	dec	esi
	dec	edi
	push	edi
	mov	edi,[names_list]
	mov	ebx,edi
	mov	edx,esi
      check_macro_variable:
	movzx	ecx,byte [edi]
	jecxz	no_macro_variable
	inc	edi
	mov	ebp,edi
	add	ebp,ecx
	repe	cmps byte [esi],[edi]
	jnz	next_macro_variable
	mov	al,[esi]
	mov	ebx,edi
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scas byte [edi]
	mov	edi,ebx
	jne	next_macro_variable
      macro_variable_ok:
	pop	edi
	mov	ebx,ebp
	movzx	ecx,byte [ebx]
	inc	ebx
	jecxz	macro_process
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
	mov	ebx,edi
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scas byte [edi]
	mov	edi,ebx
	jne	check_for_macro_directive
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
	add	sp,4
	mov	edi,ebp
	movzx	eax,ax
	add	eax,preprocessor
	jmp	near eax
      local_symbols:
	lods	byte [esi]
	cmp	al,20h
	je	local_symbols
	cmp	al,9
	je	local_symbols
	or	al,al
	jz	invalid_argument
	cmp	al,';'
	je	invalid_argument
	lea	edx,[esi-1]
      get_local_name:
	mov	ebx,edi
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scas byte [edi]
	mov	edi,ebx
	je	local_name_end
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
	sub	edi,2+5
	sub	edi,ecx
	cmp	edi,[conventional_memory]
	jbe	out_of_memory
	mov	[names_list],edi
	mov	al,cl
	stos	byte [edi]
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
      find_next_local_name:
	lods	byte [esi]
	cmp	al,20h
	je	find_next_local_name
	cmp	al,9
	je	find_next_local_name
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
	lods	byte [esi]
	jmp	special_character
      skip_string:
	lods	byte [esi]
	stos	byte [edi]
	or	al,al
	jz	missing_end_quote
	cmp	al,27h
	jne	skip_string
	lods	byte [esi]
	stos	byte [edi]
	cmp	al,27h
	je	skip_string
	dec	esi
	dec	edi
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
	call	preprocess_instruction
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
	call	preprocess_instruction
	add	sp,4
	pop	[names_list]
	pop	esi
	pop	ax
	mov	[esi],al
	pop	ebp
	pop	[current_line]
	pop	esi ecx
	ret

file_number dw ?
files_list dd ?
names_list dd ?
macro_defined db ?
macro_directive_allowed db ?
locals_counter dd '0000'
current_line_end dd ?
