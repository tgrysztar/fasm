
; flat assembler source
; Copyright (c) 1999-2001, Tomasz Grysztar
; All rights reserved.

preprocessor:
	mov	eax,[memory_start]
	mov	[source_start],eax
	push	[additional_memory]
	mov	eax,[additional_memory]
	mov	[macros_list],eax
	mov	eax,[additional_memory_end]
	mov	[labels_list],eax
	mov	[display_buffer],0
	mov	[macro_status],0
	mov	edx,[input_file]
	mov	edi,[memory_start]
	call	preprocess_file
	jc	main_file_not_found
	cmp	[macro_status],0
	jne	unexpected_end_of_file
	pop	[additional_memory]
	mov	[code_start],edi
	ret

preprocess_file:
	push	[memory_end]
	push	edx
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
	dec	edx
	mov	byte [edx],1Ah
	sub	edx,ecx
	jc	out_of_memory
	mov	esi,edx
	cmp	edx,edi
	jbe	out_of_memory
	mov	[memory_end],edx
	call	read
	call	close
	pop	edx
	xor	ecx,ecx
	mov	ebx,esi
      preprocess_source:
	inc	ecx
	mov	[current_line],edi
	mov	eax,edx
	stos	dword [edi]
	mov	eax,ecx
	stos	dword [edi]
	mov	eax,esi
	sub	eax,ebx
	stos	dword [edi]
	push	ebx edx
	call	convert_line
	call	preprocess_line
	pop	edx ebx
      next_line:
	cmp	byte [esi-1],1Ah
	jne	preprocess_source
      file_end:
	pop	[memory_end]
	clc
	ret
      no_source_file:
	pop	eax eax
	stc
	ret

convert_line:
	push	ecx
	cmp	[macro_status],0
	jle	convert_line_data
	mov	ax,3Bh
	stos	word [edi]
      convert_line_data:
	cmp	edi,[memory_end]
	jae	out_of_memory
	lods	byte [esi]
	cmp	al,20h
	je	convert_line_data
	cmp	al,9
	je	convert_line_data
	dec	esi
	lods	byte [esi]
	mov	ah,al
	mov	ebx,characters
	xlatb
	or	al,al
	jz	convert_separator
	cmp	ah,27h
	je	convert_string
	cmp	ah,22h
	je	convert_string
	mov	byte [edi],1Ah
	scas	word [edi]
	stos	byte [edi]
	mov	ebx,characters
	xor	ecx,ecx
      convert_symbol:
	lods	byte [esi]
	xlatb
	stos	byte [edi]
	or	al,al
	loopnzd convert_symbol
	neg	ecx
	cmp	ecx,255
	ja	name_too_long
	dec	edi
	mov	ebx,edi
	sub	ebx,ecx
	mov	byte [ebx-1],cl
	mov	ah,[esi-1]
      convert_separator:
	xchg	al,ah
	cmp	al,20h
	jb	control_character
	je	convert_line_data
      symbol_character:
	cmp	al,3Bh
	je	ignore_comment
	cmp	al,5Ch
	je	concate_lines
	stos	byte [edi]
	jmp	convert_line_data
      control_character:
	cmp	al,1Ah
	je	line_end
	cmp	al,0Dh
	je	cr_character
	cmp	al,0Ah
	je	lf_character
	cmp	al,9
	je	convert_line_data
	or	al,al
	jnz	symbol_character
	jmp	line_end
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
	jmp	line_end
      convert_string:
	mov	al,22h
	stos	byte [edi]
	scas	dword [edi]
	mov	ebx,edi
      copy_string:
	lods	byte [esi]
	stos	byte [edi]
	cmp	al,0Ah
	je	missing_end_quote
	cmp	al,0Dh
	je	missing_end_quote
	or	al,al
	jz	missing_end_quote
	cmp	al,1Ah
	je	missing_end_quote
	cmp	al,ah
	jne	copy_string
	lods	byte [esi]
	cmp	al,ah
	je	copy_string
	dec	esi
	dec	edi
	mov	eax,edi
	sub	eax,ebx
	mov	[ebx-4],eax
	jmp	convert_line_data
      concate_lines:
	lods	byte [esi]
	cmp	al,20h
	je	concate_lines
	cmp	al,9
	je	concate_lines
	cmp	al,1Ah
	je	unexpected_end_of_file
	cmp	al,0Ah
	je	concate_lf
	cmp	al,0Dh
	je	concate_cr
	cmp	al,3Bh
	jne	extra_characters_on_line
      find_concated_line:
	lods	byte [esi]
	cmp	al,0Ah
	je	concate_lf
	cmp	al,0Dh
	je	concate_cr
	or	al,al
	jz	concate_ok
	cmp	al,1Ah
	jne	find_concated_line
	jmp	unexpected_end_of_file
      concate_lf:
	lods	byte [esi]
	cmp	al,0Dh
	je	concate_ok
	dec	esi
	jmp	concate_ok
      concate_cr:
	lods	byte [esi]
	cmp	al,0Ah
	je	concate_ok
	dec	esi
      concate_ok:
	inc	dword [esp]
	jmp	convert_line_data
      ignore_comment:
	lods	byte [esi]
	cmp	al,0Ah
	je	lf_character
	cmp	al,0Dh
	je	cr_character
	or	al,al
	jz	line_end
	cmp	al,1Ah
	jne	ignore_comment
      line_end:
	xor	al,al
	stos	byte [edi]
	pop	ecx
	ret

preprocess_line:
	push	[struc_name]
	push	ecx esi
	mov	esi,[current_line]
	add	esi,12
	mov	al,[macro_status]
	dec	al
	jz	find_macro_block
	dec	al
	jz	skip_macro_block
      preprocess_instruction:
	lods	byte [esi]
	cmp	al,':'
	je	preprocess_instruction
	movzx	ecx,byte [esi]
	inc	esi
	cmp	al,1Ah
	jne	not_preprocessor_symbol
	push	edi
	mov	edi,preprocessor_directives
	call	get_symbol
	pop	edi
	jc	not_preprocessor_directive
	mov	byte [edx-2],3Bh
	movzx	ebx,ax
	add	ebx,preprocessor
	xor	eax,eax
	jmp	near ebx
      not_preprocessor_directive:
	mov	al,cl
	xor	ah,ah
	call	get_macro
	jc	not_macro
	mov	byte [edx-2],3Bh
	mov	[struc_name],0
	jmp	use_macro
      not_macro:
	mov	[struc_name],esi
	add	esi,ecx
	lods	byte [esi]
	cmp	al,':'
	je	preprocess_instruction
	cmp	al,1Ah
	jne	not_preprocessor_symbol
	cmp	dword [esi],3+'equ' shl 8
	je	define_symbolic_constant
	lods	byte [esi]
	mov	ah,1
	call	get_macro
	jc	not_preprocessor_symbol
	mov	byte [edx-2],':'
	mov	al,3Bh
	xchg	al,[edx-1]
	dec	al
	mov	[edx],al
	jmp	use_macro
      not_preprocessor_symbol:
	mov	esi,[current_line]
	add	esi,12
	call	process_symbolic_constants
      line_preprocessed:
	pop	esi ecx
	pop	[struc_name]
	ret
get_macro:
	mov	edx,esi
	mov	ebp,edi
	mov	ebx,[additional_memory]
      check_macro:
	mov	cl,al
	cmp	ebx,[macros_list]
	je	no_macro_found
	sub	ebx,8
	cmp	ax,[ebx]
	jne	check_macro
	mov	edi,[ebx+4]
	repe	cmps byte [esi],[edi]
	je	macro_ok
	mov	esi,edx
	jmp	check_macro
      no_macro_found:
	mov	edi,ebp
	stc
	ret
      macro_ok:
	mov	edi,ebp
	clc
	ret
process_symbolic_constants:
	mov	ebp,esi
	lods	byte [esi]
	cmp	al,1Ah
	je	check_symbol
	cmp	al,22h
	je	ignore_string
	or	al,al
	jnz	process_symbolic_constants
	dec	esi
	ret
      ignore_string:
	lods	dword [esi]
	add	esi,eax
	jmp	process_symbolic_constants
      check_symbol:
	movzx	ecx,byte [esi]
	inc	esi
	call	replace_symbolic_constant
	jnc	process_after_replaced
	add	esi,ecx
	jmp	process_symbolic_constants
      replace_symbolic_constant:
	push	edi
	mov	ebx,esi
	mov	eax,ecx
	mov	edx,[labels_list]
      scan_symbolic_constants:
	mov	ecx,eax
	mov	esi,ebx
	cmp	edx,[additional_memory_end]
	je	not_symbolic_constant
	cmp	al,[edx]
	jne	next_symbolic_constant
	mov	edi,[edx+4]
	repe	cmps byte [esi],[edi]
	je	symbolic_constant_found
      next_symbolic_constant:
	add	edx,16
	jmp	scan_symbolic_constants
      not_symbolic_constant:
	pop	edi
	stc
	ret
      symbolic_constant_found:
	pop	edi
	mov	ecx,[edx+8]
	mov	edx,[edx+12]
	xchg	esi,edx
	xor	eax,eax
	shr	ecx,1
	rcl	al,1
	shr	ecx,1
	rcl	ah,1
	rep	movs dword [edi],[esi]
	mov	cl,ah
	rep	movs word [edi],[esi]
	mov	cl,al
	rep	movs byte [edi],[esi]
	mov	esi,edx
	clc
	ret
      process_after_replaced:
	lods	byte [esi]
	cmp	al,1Ah
	je	symbol_after_replaced
	stos	byte [edi]
	cmp	al,22h
	je	string_after_replaced
	or	al,al
	jnz	process_after_replaced
	mov	ecx,edi
	sub	ecx,esi
	mov	edi,ebp
	xor	eax,eax
	shr	ecx,1
	rcl	al,1
	shr	ecx,1
	rcl	ah,1
	rep	movs dword [edi],[esi]
	mov	cl,ah
	rep	movs word [edi],[esi]
	mov	cl,al
	rep	movs byte [edi],[esi]
	ret
      string_after_replaced:
	lods	dword [esi]
	stos	dword [edi]
	mov	ecx,eax
	rep	movs byte [edi],[esi]
	jmp	process_after_replaced
      symbol_after_replaced:
	movzx	ecx,byte [esi]
	inc	esi
	call	replace_symbolic_constant
	jnc	process_after_replaced
	mov	al,1Ah
	mov	ah,cl
	stos	word [edi]
	rep	movs byte [edi],[esi]
	jmp	process_after_replaced
include_file:
	lods	byte [esi]
	cmp	al,22h
	jne	invalid_argument
	lods	dword [esi]
	mov	edx,esi
	add	esi,eax
	cmp	byte [esi],0
	jne	extra_characters_on_line
	call	preprocess_file
	jc	file_not_found
	jmp	line_preprocessed
define_symbolic_constant:
	add	esi,4
	push	esi
	call	process_symbolic_constants
	pop	ebx
	mov	edx,[labels_list]
	sub	edx,16
	cmp	edx,[additional_memory]
	jb	out_of_memory
	mov	[labels_list],edx
	mov	ecx,edi
	dec	ecx
	sub	ecx,ebx
	mov	[edx+8],ecx
	mov	[edx+12],ebx
	mov	ebx,[struc_name]
	mov	byte [ebx-2],3Bh
	mov	al,[ebx-1]
	mov	[edx],al
	mov	[edx+4],ebx
	jmp	line_preprocessed
define_struc:
	or	ah,1
define_macro:
	cmp	[macro_status],0
	jne	unexpected_instruction
	lods	byte [esi]
	cmp	al,1Ah
	jne	invalid_name
	lods	byte [esi]
	mov	ebx,[additional_memory]
	mov	[ebx],ax
	mov	[ebx+4],esi
	add	ebx,8
	cmp	ebx,[labels_list]
	jae	out_of_memory
	mov	[additional_memory],ebx
	movzx	eax,al
	add	esi,eax
	mov	[macro_status],1
	xor	bl,bl
	lods	byte [esi]
	or	al,al
	jz	line_preprocessed
	cmp	al,'{'
	je	found_macro_block
	dec	esi
      skip_macro_arguments:
	lods	byte [esi]
	cmp	al,1Ah
	je	skip_macro_argument
	cmp	al,'['
	jne	invalid_macro_arguments
	xor	bl,-1
	jz	invalid_macro_arguments
	lods	byte [esi]
	cmp	al,1Ah
	jne	invalid_macro_arguments
      skip_macro_argument:
	movzx	eax,byte [esi]
	inc	esi
	add	esi,eax
	lods	byte [esi]
	cmp	al,','
	je	skip_macro_arguments
	cmp	al,']'
	jne	end_macro_arguments
	lods	byte [esi]
	not	bl
      end_macro_arguments:
	or	bl,bl
	jnz	invalid_macro_arguments
	or	al,al
	jz	line_preprocessed
	cmp	al,'{'
	je	found_macro_block
	jmp	invalid_macro_arguments
      find_macro_block:
	add	esi,2
	lods	byte [esi]
	or	al,al
	jz	line_preprocessed
	cmp	al,'{'
	jne	unexpected_characters
      found_macro_block:
	mov	[macro_status],2
      skip_macro_block:
	lods	byte [esi]
	cmp	al,1Ah
	je	skip_macro_symbol
	cmp	al,3Bh
	je	skip_macro_symbol
	cmp	al,22h
	je	skip_macro_string
	or	al,al
	jz	line_preprocessed
	cmp	al,'}'
	jne	skip_macro_block
	lods	byte [esi]
	or	al,al
	jnz	extra_characters_on_line
	mov	[macro_status],0
	jmp	line_preprocessed
      skip_macro_symbol:
	movzx	eax,byte [esi]
	inc	esi
	add	esi,eax
	jmp	skip_macro_block
      skip_macro_string:
	lods	dword [esi]
	add	esi,eax
	jmp	skip_macro_block
purge_macro:
	lods	byte [esi]
	cmp	al,1Ah
	jne	invalid_name
	lods	byte [esi]
	xor	ah,ah
	call	get_macro
	jc	macro_purged
	or	byte [ebx+1],80h
      macro_purged:
	lods	byte [esi]
	cmp	al,','
	je	purge_macro
	or	al,al
	jnz	extra_characters_on_line
	jmp	line_preprocessed
use_macro:
	push	[macro_constants] [macro_block] [macro_block_line_number]
	push	[counter] [counter_limit]
	or	[macro_status],80h
	or	byte [ebx+1],80h
	mov	edx,esi
	movzx	esi,byte [ebx]
	add	esi,[ebx+4]
	push	edi
	mov	edi,[additional_memory]
	mov	[macro_constants],edi
	mov	[counter],0
      process_macro_arguments:
	lods	byte [esi]
	or	al,al
	jz	find_macro_instructions
	cmp	al,'{'
	je	macro_instructions_start
	cmp	al,'['
	jne	get_macro_argument
	mov	ebp,esi
	inc	esi
	inc	[counter]
      get_macro_argument:
	movzx	eax,byte [esi]
	inc	esi
	mov	[edi+4],esi
	add	esi,eax
	ror	eax,8
	or	eax,[counter]
	rol	eax,8
	mov	[edi],eax
	xchg	esi,edx
	mov	[edi+12],esi
      get_argument_value:
	lods	byte [esi]
	or	al,al
	jz	argument_value_end
	cmp	al,','
	je	argument_value_end
	cmp	al,22h
	je	argument_string
	cmp	al,1Ah
	jne	get_argument_value
	movzx	eax,byte [esi]
	inc	esi
	add	esi,eax
	jmp	get_argument_value
      argument_string:
	lods	dword [esi]
	add	esi,eax
	jmp	get_argument_value
      argument_value_end:
	dec	esi
	mov	eax,esi
	sub	eax,[edi+12]
	mov	[edi+8],eax
	xchg	esi,edx
	add	edi,16
	cmp	edi,[labels_list]
	jae	out_of_memory
	lods	byte [esi]
	cmp	al,','
	je	next_argument
	cmp	al,']'
	je	next_arguments_group
	dec	esi
	jmp	arguments_end
      next_argument:
	cmp	byte [edx],','
	jne	process_macro_arguments
	inc	edx
	jmp	process_macro_arguments
      next_arguments_group:
	cmp	byte [edx],','
	jne	arguments_end
	inc	edx
	inc	[counter]
	mov	esi,ebp
	jmp	process_macro_arguments
      arguments_end:
	lods	byte [esi]
	cmp	al,'{'
	je	macro_instructions_start
      find_macro_instructions:
	add	esi,14
	lods	byte [esi]
	or	al,al
	jz	find_macro_instructions
	cmp	al,'{'
	jne	unexpected_characters
      macro_instructions_start:
	cmp	byte [edx],0
	jne	invalid_macro_arguments
	mov	[additional_memory],edi
	pop	edi
	mov	ecx,80000000h
	push	[current_line]
	mov	[macro_block],esi
	mov	[macro_block_line_number],ecx
	mov	eax,1
	xchg	eax,[counter]
	mov	[counter_limit],eax
	or	eax,eax
	jnz	process_macro_line
	mov	[counter_limit],1
      process_macro_line:
	mov	[current_line],edi
	mov	eax,[ebx+4]
	dec	eax
	stos	dword [edi]
	mov	eax,ecx
	stos	dword [edi]
	mov	eax,[esp]
	stos	dword [edi]
	or	[macro_status],40h
	push	ebx ecx
      process_macro:
	lods	byte [esi]
	cmp	al,'}'
	je	macro_line_processed
	or	al,al
	jz	macro_line_processed
	cmp	al,1Ah
	je	process_macro_symbol
	and	[macro_status],not 40h
	stos	byte [edi]
	cmp	al,22h
	jne	process_macro
      copy_macro_string:
	mov	ecx,[esi]
	add	ecx,4
	rep	movs byte [edi],[esi]
	jmp	process_macro
      process_macro_symbol:
	push	esi edi
	test	[macro_status],40h
	jz	not_macro_directive
	movzx	ecx,byte [esi]
	inc	esi
	mov	edi,macro_directives
	call	get_symbol
	jnc	process_macro_directive
	dec	esi
	jmp	not_macro_directive
      process_macro_directive:
	movzx	edx,ax
	add	edx,preprocessor
	pop	edi eax
	mov	byte [edi],0
	inc	edi
	pop	ecx ebx
	jmp	near edx
      not_macro_directive:
	and	[macro_status],not 40h
	mov	eax,[counter]
	or	eax,eax
	jnz	check_for_macro_constant
	inc	eax
      check_for_macro_constant:
	shl	eax,8
	mov	al,[esi]
	inc	esi
	movzx	ebp,al
	mov	edx,[macro_constants]
	mov	ebx,esi
      scan_macro_constants:
	cmp	edx,[additional_memory]
	je	not_macro_constant
	cmp	eax,[edx]
	je	try_macro_constant
	cmp	ebp,[edx]
	jne	next_macro_constant
      try_macro_constant:
	mov	ecx,ebp
	mov	edi,[edx+4]
	repe	cmps byte [esi],[edi]
	je	macro_constant_found
	mov	esi,ebx
      next_macro_constant:
	add	edx,16
	jmp	scan_macro_constants
      macro_constant_found:
	cmp	[counter],0
	jne	replace_macro_constant
	mov	eax,[edx]
	shr	eax,8
	or	eax,eax
	jz	replace_macro_constant
	cmp	eax,[counter_limit]
	je	replace_macro_constant
	pop	edi
	mov	ecx,[edx+8]
	mov	esi,[edx+12]
	rep	movs byte [edi],[esi]
	mov	byte [edi],','
	inc	edi
	mov	esi,ebx
	inc	eax
	shl	eax,8
	mov	al,[esi-1]
	push	edi
	jmp	scan_macro_constants
      replace_macro_constant:
	pop	edi eax
	mov	ecx,[edx+8]
	mov	edx,[edx+12]
	xchg	esi,edx
	rep	movs byte [edi],[esi]
	mov	esi,edx
	jmp	process_macro
      not_macro_constant:
	pop	edi esi
	mov	al,1Ah
	stos	byte [edi]
	mov	al,[esi]
	inc	esi
	stos	byte [edi]
	cmp	byte [esi],'.'
	jne	copy_macro_symbol
	mov	ebx,[struc_name]
	or	ebx,ebx
	jz	copy_macro_symbol
	xchg	esi,ebx
	movzx	ecx,byte [esi-1]
	add	[edi-1],cl
	jc	name_too_long
	rep	movs byte [edi],[esi]
	xchg	esi,ebx
      copy_macro_symbol:
	movzx	ecx,al
	rep	movs byte [edi],[esi]
	jmp	process_macro
      macro_line_processed:
	mov	byte [edi],0
	inc	edi
	push	eax
	call	preprocess_line
	pop	eax
	pop	ecx ebx
	cmp	al,'}'
	je	macro_block_processed
      process_next_line:
	inc	ecx
	add	esi,14
	jmp	process_macro_line
      local_symbols:
	lods	byte [esi]
	cmp	al,1Ah
	jne	invalid_argument
	push	edi ecx
	movzx	ecx,byte [esi]
	inc	esi
	mov	edx,[additional_memory]
	mov	eax,[counter]
	shl	eax,8
	mov	al,cl
	mov	[edx],eax
	mov	[edx+4],esi
	movzx	eax,[_counter]
	mov	edi,[memory_end]
	sub	edi,eax
	sub	edi,ecx
	sub	edi,3
	mov	[memory_end],edi
	mov	[edx+12],edi
	add	al,cl
	jc	name_too_long
	inc	al
	jz	name_too_long
	mov	byte [edi],1Ah
	inc	edi
	mov	[edi],al
	inc	edi
	add	eax,2
	mov	[edx+8],eax
	add	edx,16
	cmp	edx,[labels_list]
	jae	out_of_memory
	mov	[additional_memory],edx
	rep	movs byte [edi],[esi]
	mov	al,'?'
	stos	byte [edi]
	movzx	ecx,byte [_counter]
	push	esi
	mov	esi,_counter+1
	rep	movs byte [edi],[esi]
	pop	esi
	pop	ecx edi
	cmp	edi,[memory_end]
	jae	out_of_memory
	lods	byte [esi]
	cmp	al,','
	je	local_symbols
	cmp	al,'}'
	je	macro_block_processed
	or	al,al
	jnz	extra_characters_on_line
	jmp	process_next_line
      common_block:
	call	close_macro_block
	jc	process_macro_line
	mov	[counter],0
	jmp	new_macro_block
      forward_block:
	call	close_macro_block
	jc	process_macro_line
	mov	[counter],1
	jmp	new_macro_block
      reverse_block:
	call	close_macro_block
	jc	process_macro_line
	mov	eax,[counter_limit]
	or	eax,80000000h
	mov	[counter],eax
      new_macro_block:
	mov	[macro_block],esi
	mov	[macro_block_line_number],ecx
	jmp	process_macro_line
      close_macro_block:
	push	ecx
	mov	eax,_counter
	call	increase_counter
	pop	ecx
	cmp	[counter],0
	je	block_closed
	jl	reverse_counter
	mov	eax,[counter]
	cmp	eax,[counter_limit]
	je	block_closed
	inc	[counter]
	jmp	continue_block
      reverse_counter:
	mov	eax,[counter]
	dec	eax
	cmp	eax,80000000h
	je	block_closed
	mov	[counter],eax
      continue_block:
	mov	esi,[macro_block]
	mov	ecx,[macro_block_line_number]
	stc
	ret
      block_closed:
	clc
	ret
      macro_block_processed:
	call	close_macro_block
	jc	process_macro_line
	and	byte [ebx+1],not 80h
	pop	[current_line]
	mov	eax,[macro_constants]
	mov	[additional_memory],eax
	mov	[macro_status],0
	pop	[counter_limit] [counter]
	pop	[macro_block_line_number] [macro_block] [macro_constants]
	jmp	line_preprocessed

increase_counter:
	movzx	ecx,byte [eax]
      counter_loop:
	call	increase_digit
	jnc	counter_ok
	mov	byte [eax+ecx],'0'
	loop	counter_loop
      counter_ok:
	ret
      increase_digit:
	inc	byte [eax+ecx]
	cmp	byte [eax+ecx],':'
	jb	digit_increased
	je	letter_digit
	cmp	byte [eax+ecx],'f'
	jbe	digit_increased
	stc
	ret
      letter_digit:
	mov	byte [eax+ecx],'a'
      digit_increased:
	clc
	ret
