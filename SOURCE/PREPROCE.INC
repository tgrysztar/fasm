
preprocessor:
	mov	eax,[additional_memory_end]
	mov	[symbols_list],eax
	mov	[file_number],1
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
	xor	al,al
	stosb
	mov	edx,edi
      copy_file_name:
	lodsb
	cmp	edi,[additional_memory_end]
	jae	out_of_memory
	stosb
	cmp	al,27h
	je	file_name_end
	or	al,al
	jnz	copy_file_name
      file_name_end:
	mov	byte [edi-1],0
	mov	ax,3D00h
	int	21h
	jc	no_source_file
	mov	ax,4202h
	xor	edx,edx
	int	21h
	push	eax
	mov	ax,4200h
	xor	edx,edx
	int	21h
	pop	ecx
	mov	edx,[memory_end]
	mov	ebp,edx
	sub	edx,ecx
	mov	esi,edx
	mov	[memory_end],edx
	mov	eax,edi
	add	eax,4
	cmp	eax,[additional_memory_end]
	jae	out_of_memory
	mov	eax,edx
	stosd
	mov	[additional_memory],edi
	mov	ah,3Fh
	int	21h
	mov	ah,3Eh
	int	21h
	pop	edi
	movzx	ecx,[file_number]
	inc	[file_number]
	shl	ecx,24
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
	cmp	esi,ebp
	jb	preprocess_source
	clc
	ret
      no_source_file:
	add	esp,4
	stc
	ret
      convert_line:
	mov	eax,ecx
	stosd
	mov	al,bl
	stosb
      copy_line:
	cmp	edi,edx
	jae	out_of_memory
	cmp	esi,ebp
	je	line_end
	lodsb
	cmp	al,0Ah
	je	lf_character
	cmp	al,0Dh
	je	cr_character
	stosb
	jmp	copy_line
      lf_character:
	cmp	al,0Dh
	je	line_end
	dec	esi
	jmp	line_end
      cr_character:
	cmp	al,0Ah
	je	line_end
	dec	esi
      line_end:
	add	esi,2
	xor	al,al
	stosb
	ret

preprocess_instruction:
	push	ecx esi edi ebp
	mov	esi,[current_line]
	add	esi,5
      find_directive:
	lodsb
	cmp	al,20h
	je	find_directive
	or	al,al
	jz	no_directive
	cmp	al,';'
	je	no_directive
	dec	esi
	mov	edx,esi
      find_directive_end:
	lodsb
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	jne	find_directive_end
	cmp	al,':'
	je	find_directive
	lea	ecx,[esi-1]
	mov	esi,edx
	sub	ecx,esi
	jecxz	no_directive
	mov	edi,preprocessor_directives
	call	get_instruction
	jc	no_directive
	jmp	ebx
      no_directive:
	mov	edi,[files_list]
	mov	eax,1
      get_macro:
	cmp	edi,[additional_memory]
	je	instruction_preprocessed
	cmp	byte [edi],0
	je	skip_file
	movzx	ecx,byte [edi]
	inc	edi
	mov	ebx,edi
	add	ebx,ecx
	add	ebx,5
	cmp	ecx,ebp
	jne	skip_macro
	mov	edx,esi
	repe	cmpsb
	je	use_macro
	mov	esi,edx
	mov	edi,ebx
	jmp	get_macro
      skip_macro:
	mov	edi,ebx
	inc	eax
	jmp	get_macro
      skip_file:
	inc	edi
	cmp	byte [edi],0
	jne	skip_file
	add	edi,5
	inc	eax
	jmp	get_macro
      instruction_preprocessed:
	pop	ebp edi esi ecx
	ret
include_file:
	lodsb
	cmp	al,20h
	je	include_file
	cmp	al,27h
	jne	invalid_argument
	mov	edx,[current_line]
	mov	byte [edx+4],':'
	mov	eax,[additional_memory_end]
	cmp	eax,[symbols_list]
	jne	illegal_instruction
	mov	edx,esi
      find_include_end:
	lodsb
	cmp	al,27h
	jne	find_include_end
	lodsb
	cmp	al,27h
	je	find_include_end
	dec	esi
      check_include_line:
	lodsb
	or	al,al
	jz	preprocess_included_file
	cmp	al,';'
	je	preprocess_included_file
	cmp	al,20h
	je	check_include_line
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
define_macro:
	lodsb
	cmp	al,20h
	je	define_macro
	dec	esi
	mov	edx,[current_line]
	mov	byte [edx+4],':'
	mov	eax,[additional_memory_end]
	cmp	eax,[symbols_list]
	jne	illegal_instruction
	mov	edx,[additional_memory]
	mov	ebx,edx
	inc	edx
      copy_macro_name:
	lodsb
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	je	macro_name_end
	mov	[edx],al
	inc	edx
	jmp	copy_macro_name
      macro_name_end:
	dec	esi
	mov	edi,edx
	sub	edx,[additional_memory]
	dec	edx
	cmp	edx,100h
	jae	name_too_long
	mov	[ebx],dl
	xor	al,al
	stosb
	mov	eax,esi
	stosd
	mov	[additional_memory],edi
	inc	[file_number]
	mov	edi,[current_line_end]
      find_macro_argument:
	lodsb
	cmp	al,20h
	je	find_macro_argument
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
	repne	scasb
	mov	edi,ebx
	je	macro_argument_end
	lodsb
	jmp	skip_macro_argument
      macro_argument_end:
	dec	esi
	cmp	byte [esi-1],20h
	je	invalid_macro_arguments
      find_next_macro_argument:
	lodsb
	cmp	al,20h
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
	lodsb
	cmp	al,20h
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
	lodsb
	cmp	al,20h
	je	check_macro_start
	or	al,al
	jz	find_macro_start
	cmp	al,';'
	je	find_macro_start
	cmp	al,'{'
	jne	unexpected_characters
      find_macro_end:
	lodsb
	mov	ebx,edi
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	mov	edi,ebx
	je	special_character_in_macro
	cmp	al,27h
	je	skip_string_in_macro
	jmp	find_macro_end
      skip_string_in_macro:
	lodsb
	or	al,al
	jz	missing_end_quote
	cmp	al,27h
	jne	skip_string_in_macro
	lodsb
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
	jne	find_macro_end
      check_macro_after_end:
	lodsb
	cmp	al,20h
	je	check_macro_after_end
	or	al,al
	jz	macro_definition_ok
	cmp	al,';'
	je	macro_definition_ok
	jmp	extra_characters_on_line
      macro_definition_ok:
	pop	ebp
	add	esp,4
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
	pop	ebp
	mov	[ebp+8],esi
	pop	esi
	ret
use_macro:
	push	[symbols_list]
	shl	eax,24
	push	eax
	mov	edx,[current_line]
	mov	[esp+Ch],edx
	mov	al,':'
	cmp	byte [edx+4],20h
	jne	set_macro_line_type
	mov	al,';'
      set_macro_line_type:
	mov	byte [edx+4],al
	mov	ebx,esi
	mov	esi,[edi+1]
	mov	edi,[symbols_list]
	dec	edi
	mov	byte [edi],0
	mov	[symbols_list],edi
      find_argument:
	lodsb
	cmp	al,20h
	je	find_argument
	dec	esi
	or	al,al
	jz	find_macro_instructions
	cmp	al,'{'
	je	find_macro_instructions
	cmp	al,';'
	je	find_macro_instructions
	mov	edx,esi
      find_argument_end:
	lodsb
	cmp	al,','
	je	argument_end
	or	al,al
	jz	argument_end
	cmp	al,';'
	je	argument_end
	cmp	al,20h
	jne	find_argument_end
      argument_end:
	dec	esi
	xchg	esi,ebx
      find_argument_value:
	lodsb
	cmp	al,20h
	je	find_argument_value
	dec	esi
	or	al,al
	jz	invalid_macro_arguments
	cmp	al,'{'
	je	invalid_macro_arguments
	cmp	al,';'
	je	invalid_macro_arguments
	mov	ebp,esi
      find_argument_value_end:
	lodsb
	cmp	al,','
	je	argument_value_end
	or	al,al
	jz	argument_value_end
	cmp	al,';'
	jne	find_argument_value_end
      argument_value_end:
	dec	esi
	xchg	esi,ebx
	push	ebx esi
	mov	edi,[symbols_list]
	sub	esi,edx
	sub	ebx,ebp
	sub	edi,esi
	sub	edi,ebx
	sub	edi,2
	cmp	edi,[additional_memory]
	jbe	out_of_memory
	mov	[symbols_list],edi
	mov	ecx,esi
	cmp	ecx,100h
	jae	name_too_long
	mov	esi,edx
	mov	al,cl
	stosb
	rep	movsb
	mov	ecx,ebx
	or	ecx,ecx
	jz	invalid_macro_arguments
	cmp	ecx,100h
	jae	argument_too_long
	mov	esi,ebp
	mov	al,cl
	stosb
	rep	movsb
	pop	esi ebx
      check_for_next_argument:
	lodsb
	cmp	al,20h
	je	check_for_next_argument
	xchg	esi,ebx
	cmp	al,','
	je	next_argument
	dec	ebx
	lodsb
	cmp	al,0
	je	arguments_ok
	cmp	al,';'
	je	arguments_ok
	jmp	invalid_macro_arguments
      next_argument:
	lodsb
	cmp	al,20h
	je	next_argument
	cmp	al,','
	jne	invalid_macro_arguments
	xchg	esi,ebx
	jmp	find_argument
      arguments_ok:
	mov	esi,ebx
      find_macro_instructions:
	lodsb
	cmp	al,20h
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
	lodsb
	or	al,al
	jnz	skip_macro_comment
	add	esi,5
	jmp	find_macro_instructions
      macro_instructions_start:
	mov	edi,[current_line_end]
	mov	[current_line],edi
	mov	eax,[esp]
	stosd
	mov	al,20h
	stosb
	inc	dword [esp]
	mov	[macro_directive_allowed],1
      macro_process:
	lodsb
	stosb
	mov	ebx,edi
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	mov	edi,ebx
	je	special_character
	cmp	al,27h
	je	skip_string
	dec	esi
	dec	edi
	push	edi
	mov	edi,[symbols_list]
	mov	ebx,edi
	mov	edx,esi
      check_macro_variable:
	movzx	ecx,byte [edi]
	jecxz	no_macro_variable
	inc	edi
	mov	ebp,edi
	add	ebp,ecx
	repe	cmpsb
	jnz	next_macro_variable
	mov	al,[esi]
	mov	ebx,edi
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	mov	edi,ebx
	jne	next_macro_variable
	pop	edi
	mov	ebx,ebp
	movzx	ecx,byte [ebx]
	inc	ebx
      copy_macro_variable:
	mov	al,[ebx]
	inc	ebx
	stosb
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
	lodsb
	stosb
	mov	ebx,edi
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
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
	call	get_instruction
	pop	ebp edi
	jc	no_macro_directive
	add	esp,4
	mov	edi,ebp
	jmp	ebx
      local_symbols:
	lodsb
	cmp	al,20h
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
	repne	scasb
	mov	edi,ebx
	je	local_name_end
	lodsb
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
	mov	edi,[symbols_list]
	sub	edi,2
	sub	edi,ecx
	sub	edi,5
	cmp	edi,[additional_memory]
	jbe	out_of_memory
	mov	[symbols_list],edi
	mov	al,cl
	stosb
	mov	esi,edx
	rep	movsb
	mov	al,5
	stosb
	mov	al,'?'
	stosb
	mov	eax,[locals_counter]
	stosd
	pop	edi
	inc	byte [locals_counter+3]
	cmp	byte [locals_counter+3],'9'
	jbe	find_next_local_name
	mov	byte [locals_counter+3],'0'
	inc	byte [locals_counter+2]
	cmp	byte [locals_counter+2],'9'
	jbe	find_next_local_name
	mov	byte [locals_counter+2],'0'
	inc	byte [locals_counter+1]
	cmp	byte [locals_counter+1],'9'
	jbe	find_next_local_name
	mov	byte [locals_counter+1],'0'
	inc	byte [locals_counter]
      find_next_local_name:
	lodsb
	cmp	al,20h
	je	find_next_local_name
	cmp	al,','
	je	local_symbols
	or	al,al
	jz	macro_directive_end
	cmp	al,';'
	je	macro_directive_comment
	jmp	invalid_argument
      macro_directive_comment:
	lodsb
	or	al,al
	jnz	macro_directive_comment
      macro_directive_end:
	stosb
	jmp	next_macro_line
      no_macro_directive:
	pop	esi
	lodsb
	jmp	special_character
      skip_string:
	lodsb
	stosb
	or	al,al
	jz	missing_end_quote
	cmp	al,27h
	jne	skip_string
	lodsb
	stosb
	cmp	al,27h
	je	skip_string
	dec	esi
	dec	edi
	jmp	macro_process
      skip_comment:
	lodsb
	stosb
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
	mov	ebp,[esp+4]
	call	preprocess_instruction
	mov	[current_line],edi
	mov	eax,[esp]
	stosd
	mov	al,20h
	stosb
	inc	dword [esp]
	mov	[macro_directive_allowed],1
	jmp	macro_process
      check_for_end:
	cmp	al,'}'
	jne	macro_process
	dec	edi
	xor	al,al
	stosb
	mov	[current_line_end],edi
	mov	ecx,[esp+14h]
	mov	ebp,[esp+8]
	call	preprocess_instruction
	add	esp,4
	pop	[symbols_list]
	pop	ebp
	pop	[current_line]
	pop	esi ecx
	ret
HF_directive:
	mov	edx,[current_line]
	mov	byte [edx+4],':'
	mov	[program_format],48h
	pop	ebp edi esi ecx
	ret

file_number db 0
files_list dd 0
macro_directive_allowed db 0
locals_counter dd '0000'
