
; flat assembler core
; Copyright (c) 1999-2003, Tomasz Grysztar.
; All rights reserved.

preprocessor:
	mov	eax,[additional_memory]
	mov	[free_additional_memory],eax
	mov	[macros_list],eax
	mov	eax,[additional_memory_end]
	mov	[labels_list],eax
	xor	eax,eax
	mov	[fix_constants_count],eax
	mov	[equ_constants_count],eax
	mov	[macro_status],al
	mov	edx,[input_file]
	mov	edi,[memory_start]
	call	preprocess_file
	jc	main_file_not_found
	cmp	[macro_status],0
	jne	unexpected_end_of_file
	mov	[source_start],edi
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
	xor	eax,eax
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
	test	[macro_status],0Fh
	jz	convert_line_data
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
	xlat	byte [ebx]
	or	al,al
	jz	convert_separator
	cmp	ah,27h
	je	convert_string
	cmp	ah,22h
	je	convert_string
	mov	byte [edi],1Ah
	scas	word [edi]
	xchg	al,ah
	stos	byte [edi]
	mov	ebx,characters
	xor	ecx,ecx
      convert_symbol:
	lods	byte [esi]
	stos	byte [edi]
	xlat	byte [ebx]
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
	je	concatenation_character
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
      concatenation_character:
	mov	byte [edi],0
      concatenate_lines:
	lods	byte [esi]
	cmp	al,20h
	je	concatenate_lines
	cmp	al,9
	je	concatenate_lines
	cmp	al,1Ah
	je	unexpected_end_of_file
	cmp	al,0Ah
	je	concatenate_lf
	cmp	al,0Dh
	je	concatenate_cr
	cmp	al,3Bh
	jne	extra_characters_on_line
      find_concatenated_line:
	lods	byte [esi]
	cmp	al,0Ah
	je	concatenate_lf
	cmp	al,0Dh
	je	concatenate_cr
	or	al,al
	jz	concatenate_ok
	cmp	al,1Ah
	jne	find_concatenated_line
	jmp	unexpected_end_of_file
      concatenate_lf:
	lods	byte [esi]
	cmp	al,0Dh
	je	concatenate_ok
	dec	esi
	jmp	concatenate_ok
      concatenate_cr:
	lods	byte [esi]
	cmp	al,0Ah
	je	concatenate_ok
	dec	esi
      concatenate_ok:
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
	add	esi,16
	lods	byte [esi]
	cmp	al,1Ah
	jne	not_fix_constant
	xor	eax,eax
	lods	byte [esi]
	mov	ebp,esi
	add	esi,eax
	lods	word [esi]
	cmp	ax,031Ah
	jne	not_fix_constant
	mov	ebx,characters
	movzx	eax,byte [esi]
	xlat	byte [ebx]
	ror	eax,8
	mov	al,[esi+1]
	xlat	byte [ebx]
	ror	eax,8
	mov	al,[esi+2]
	xlat	byte [ebx]
	ror	eax,16
	cmp	eax,'fix'
	je	define_fix_constant
      not_fix_constant:
	mov	esi,[current_line]
	add	esi,16
	cmp	word [esi],3Bh
	jne	line_start_ok
	add	esi,2
      line_start_ok:
	call	process_fix_constants
	mov	esi,[current_line]
	add	esi,16
	test	[macro_status],0F0h
	jz	concatenations_ok
	call	process_concatenations
	mov	esi,[current_line]
	add	esi,16
      concatenations_ok:
	mov	al,[macro_status]
	and	al,0Fh
	dec	al
	jz	find_macro_block
	dec	al
	jz	skip_macro_block
      preprocess_instruction:
	lods	byte [esi]
	movzx	ecx,byte [esi]
	inc	esi
	cmp	al,1Ah
	jne	not_preprocessor_symbol
	cmp	cl,4
	jb	not_preprocessor_directive
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
	je	preprocess_label
	cmp	al,1Ah
	jne	not_preprocessor_symbol
	lods	byte [esi]
	cmp	al,3
	jne	not_symbolic_constant
	mov	ebx,characters
	movzx	eax,byte [esi]
	xlat	byte [ebx]
	ror	eax,8
	mov	al,[esi+1]
	xlat	byte [ebx]
	ror	eax,8
	mov	al,[esi+2]
	xlat	byte [ebx]
	ror	eax,16
	cmp	eax,'equ'
	je	define_equ_constant
	mov	al,3
      not_symbolic_constant:
	mov	ah,1
	call	get_macro
	jc	not_preprocessor_symbol
	mov	al,3Ah
	mov	[edx-2],al
	inc	al
	xchg	al,[edx-1]
	dec	al
	mov	[edx],al
	jmp	use_macro
      preprocess_label:
	dec	esi
	sub	esi,ecx
	lea	ebp,[esi-2]
	mov	[value_type],2
	call	find_symbolic_constant
	jnc	symbolic_constant_in_label
	lea	esi,[esi+ecx+1]
	jmp	preprocess_instruction
      symbolic_constant_in_label:
	mov	ebx,[edx+12]
	mov	ecx,[edx+8]
	add	ecx,ebx
      check_for_broken_label:
	cmp	ebx,ecx
	je	label_broken
	cmp	byte [ebx],1Ah
	jne	label_broken
	movzx	eax,byte [ebx+1]
	lea	ebx,[ebx+2+eax]
	cmp	ebx,ecx
	je	label_constant_ok
	cmp	byte [ebx],':'
	jne	label_broken
	inc	ebx
	jmp	check_for_broken_label
      label_broken:
	push	line_preprocessed
	jmp	replace_symbolic_constant
      label_constant_ok:
	mov	ecx,edi
	sub	ecx,esi
	mov	edi,[edx+8]
	add	edi,ebp
	push	edi
	lea	eax,[edi+ecx]
	push	eax
	cmp	esi,edi
	je	replace_label
	jb	move_rest_of_line_up
	rep	movs byte [edi],[esi]
	jmp	replace_label
      move_rest_of_line_up:
	lea	esi,[esi+ecx-1]
	lea	edi,[edi+ecx-1]
	std
	rep	movs byte [edi],[esi]
	cld
      replace_label:
	mov	ecx,[edx+8]
	mov	edi,[esp+4]
	sub	edi,ecx
	mov	esi,[edx+12]
	rep	movs byte [edi],[esi]
	pop	edi esi
	inc	esi
	jmp	preprocess_instruction
      not_preprocessor_symbol:
	mov	esi,[current_line]
	add	esi,16
	call	process_equ_constants
      line_preprocessed:
	pop	esi ecx
	pop	[struc_name]
	ret

get_macro:
	mov	edx,esi
	mov	ebp,edi
	mov	ebx,[free_additional_memory]
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
process_fix_constants:
	cmp	[fix_constants_count],0
	je	no_symbolic_constants
	mov	[value_type],1
	jmp	process_symbolic_constants
process_equ_constants:
	cmp	[equ_constants_count],0
	je	no_symbolic_constants
	mov	[value_type],2
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
      no_symbolic_constants:
	ret
      ignore_string:
	lods	dword [esi]
	add	esi,eax
	jmp	process_symbolic_constants
      check_symbol:
	movzx	ecx,byte [esi]
	inc	esi
	call	find_symbolic_constant
	jnc	replace_symbolic_constant
	add	esi,ecx
	jmp	process_symbolic_constants
      find_symbolic_constant:
	push	edi
	mov	ebx,esi
	mov	eax,ecx
	mov	edx,[labels_list]
      scan_symbolic_constants:
	mov	esi,ebx
	mov	ecx,eax
	cmp	edx,[additional_memory_end]
	je	symbolic_constants_ok
	mov	al,[value_type]
	cmp	al,[edx+1]
	mov	eax,ecx
	jne	next_symbolic_constant
	cmp	cl,[edx]
	jne	next_symbolic_constant
	mov	edi,[edx+4]
	repe	cmps byte [esi],[edi]
	je	symbolic_constant_found
      next_symbolic_constant:
	add	edx,16
	jmp	scan_symbolic_constants
      symbolic_constants_ok:
	pop	edi
	stc
	ret
      symbolic_constant_found:
	pop	edi
	clc
	ret
      replace_symbolic_constant:
	mov	ecx,[edx+8]
	mov	edx,[edx+12]
	xchg	esi,edx
	call	move_data
	mov	esi,edx
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
	call	move_data
	ret
      move_data:
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
	call	find_symbolic_constant
	jnc	replace_symbolic_constant
	mov	al,1Ah
	mov	ah,cl
	stos	word [edi]
	rep	movs byte [edi],[esi]
	jmp	process_after_replaced
process_concatenations:
	xor	dl,dl
	mov	ebp,edi
	cmp	word [esi],3Bh
	jne	before_concatenations
	add	esi,2
      before_concatenations:
	mov	edi,esi
	lods	byte [esi]
	cmp	al,'#'
	je	concatenation
	cmp	al,1Ah
	je	symbol_before_concatenations
	cmp	al,3Bh
	je	no_more_concatenations
	cmp	al,22h
	je	string_before_concatenations
	xor	dl,dl
	or	al,al
	jnz	before_concatenations
	mov	edi,esi
	ret
      no_more_concatenations:
	mov	edi,ebp
	ret
      symbol_before_concatenations:
	mov	dl,1Ah
	mov	ebx,esi
	lods	byte [esi]
	movzx	ecx,al
	add	esi,ecx
	jmp	before_concatenations
      string_before_concatenations:
	mov	dl,22h
	mov	ebx,esi
	lods	dword [esi]
	add	esi,eax
	jmp	before_concatenations
      concatenation:
	cmp	dl,1Ah
	je	symbol_concatenation
	cmp	dl,22h
	je	string_concatenation
      no_concatenation:
	cmp	esi,edi
	je	before_concatenations
	jmp	after_concatenation
      symbol_concatenation:
	cmp	byte [esi],1Ah
	jne	no_concatenation
	inc	esi
	lods	byte [esi]
	movzx	ecx,al
	add	[ebx],al
	jc	name_too_long
	rep	movs byte [edi],[esi]
	jmp	after_concatenation
      string_concatenation:
	cmp	byte [esi],22h
	jne	no_concatenation
	inc	esi
	lods	dword [esi]
	mov	ecx,eax
	add	[ebx],eax
	rep	movs byte [edi],[esi]
      after_concatenation:
	lods	byte [esi]
	cmp	al,'#'
	je	concatenation
	stos	byte [edi]
	cmp	al,1Ah
	je	symbol_after_concatenation
	cmp	al,3Bh
	je	no_more_concatenations
	cmp	al,22h
	je	string_after_concatenation
	xor	dl,dl
	or	al,al
	jnz	after_concatenation
	ret
      symbol_after_concatenation:
	mov	dl,1Ah
	mov	ebx,edi
	lods	byte [esi]
	stos	byte [edi]
	movzx	ecx,al
	rep	movs byte [edi],[esi]
	jmp	after_concatenation
      string_after_concatenation:
	mov	dl,22h
	mov	ebx,edi
	lods	dword [esi]
	stos	dword [edi]
	mov	ecx,eax
	rep	movs byte [edi],[esi]
	jmp	after_concatenation
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
define_fix_constant:
	add	esi,3
	push	ebp esi
	call	process_fix_constants
	mov	esi,[esp+4]
	movzx	ecx,byte [esi-1]
	mov	[value_type],1
	call	find_symbolic_constant
	jc	allocate_new_fix_constant
	mov	ebx,edx
	jmp	make_fix_constant
      allocate_new_fix_constant:
	mov	edx,[labels_list]
	sub	edx,16
	cmp	edx,[free_additional_memory]
	jb	out_of_memory
	mov	[labels_list],edx
      make_fix_constant:
	pop	ebx
	mov	ecx,edi
	dec	ecx
	sub	ecx,ebx
	mov	[edx+8],ecx
	mov	[edx+12],ebx
	pop	ebx
	mov	byte [ebx-2],3Bh
	mov	al,[ebx-1]
	mov	[edx],al
	mov	[edx+4],ebx
	mov	byte [edx+1],1
	inc	[fix_constants_count]
	jmp	line_preprocessed
define_equ_constant:
	add	esi,3
	push	esi
	call	process_equ_constants
	pop	ebx
	mov	edx,[labels_list]
	sub	edx,16
	cmp	edx,[free_additional_memory]
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
	mov	byte [edx+1],2
	inc	[equ_constants_count]
	jmp	line_preprocessed
restore_equ_constant:
	lods	byte [esi]
	cmp	al,1Ah
	jne	invalid_name
	movzx	ecx,byte [esi]
	inc	esi
	mov	[value_type],2
	call	find_symbolic_constant
	jc	no_symbolic_constant
	mov	ecx,edx
	sub	ecx,[labels_list]
	add	[labels_list],16
	shr	ecx,2
	jz	constant_restored
	mov	ebx,esi
	mov	esi,edx
	xchg	edi,edx
	add	edi,16-4
	sub	esi,4
	std
	rep	movs dword [edi],[esi]
	cld
	dec	[equ_constants_count]
	mov	esi,ebx
	mov	edi,edx
	jmp	constant_restored
      no_symbolic_constant:
	add	esi,ecx
      constant_restored:
	lods	byte [esi]
	cmp	al,','
	je	restore_equ_constant
	or	al,al
	jnz	extra_characters_on_line
	jmp	line_preprocessed
define_struc:
	or	ah,1
define_macro:
	lods	byte [esi]
	cmp	al,1Ah
	jne	invalid_name
	lods	byte [esi]
	mov	ebx,[free_additional_memory]
	mov	[ebx],ax
	mov	[ebx+4],esi
	add	ebx,8
	cmp	ebx,[labels_list]
	jae	out_of_memory
	mov	[free_additional_memory],ebx
	movzx	eax,al
	add	esi,eax
	mov	al,[macro_status]
	and	al,0F0h
	or	al,1
	mov	[macro_status],al
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
	mov	ebx,[free_additional_memory]
	sub	ebx,8
	mov	eax,[ebx+4]
	mov	ebx,[current_line]
	mov	dword [eax-6],ebx
	mov	al,[macro_status]
	and	al,0F0h
	or	al,2
	mov	[macro_status],al
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
	and	[macro_status],0F0h
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
	push	[macro_constants] [macro_block] [macro_block_line] [macro_block_line_number]
	push	[counter] [counter_limit]
	push	dword [macro_status]
	or	[macro_status],10h
	or	byte [ebx+1],80h
	mov	edx,esi
	mov	eax,[ebx+4]
	movzx	esi,byte [ebx]
	add	esi,eax
	mov	eax,[eax-6]
	mov	[macro_line],eax
	push	edi
	mov	edi,[memory_end]
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
	sub	edi,16
	cmp	edi,[esp]
	jb	out_of_memory
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
	cmp	byte [esi],'<'
	jne	get_argument_value
	inc	esi
	mov	[edi+12],esi
	mov	ecx,1
      enclosed_argument:
	lods	byte [esi]
	or	al,al
	jz	invalid_macro_arguments
	cmp	al,'#'
	je	invalid_macro_arguments
	cmp	al,1Ah
	je	enclosed_symbol
	cmp	al,22h
	je	enclosed_string
	cmp	al,'>'
	je	enclosed_argument_end
	cmp	al,'<'
	jne	enclosed_argument
	inc	ecx
	jmp	enclosed_argument
     enclosed_symbol:
	movzx	eax,byte [esi]
	inc	esi
	add	esi,eax
	jmp	enclosed_argument
      enclosed_string:
	lods	dword [esi]
	add	esi,eax
	jmp	enclosed_argument
      enclosed_argument_end:
	loop	enclosed_argument
	mov	al,[esi]
	or	al,al
	jz	enclosed_argument_ok
	cmp	al,','
	jne	invalid_macro_arguments
      enclosed_argument_ok:
	mov	eax,esi
	sub	eax,[edi+12]
	dec	eax
	mov	[edi+8],eax
	jmp	argument_value_ok
      get_argument_value:
	lods	byte [esi]
	or	al,al
	jz	argument_value_end
	cmp	al,','
	je	argument_value_end
	cmp	al,'#'
	je	invalid_macro_arguments
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
      argument_value_ok:
	xchg	esi,edx
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
	mov	[macro_line],esi
	add	esi,18
	lods	byte [esi]
	or	al,al
	jz	find_macro_instructions
	cmp	al,'{'
	jne	unexpected_characters
      macro_instructions_start:
	cmp	byte [edx],0
	jne	invalid_macro_arguments
	mov	[memory_end],edi
	pop	edi
	mov	ecx,80000000h
	push	[current_line]
	mov	[macro_block],esi
	mov	eax,[macro_line]
	mov	[macro_block_line],eax
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
	mov	eax,[macro_line]
	stos	dword [edi]
	or	[macro_status],20h
	push	ebx ecx
	test	[macro_status],0Fh
	jz	process_macro
	mov	ax,3Bh
	stos	word [edi]
      process_macro:
	lods	byte [esi]
	cmp	al,'}'
	je	macro_line_processed
	or	al,al
	jz	macro_line_processed
	cmp	al,1Ah
	je	process_macro_symbol
	cmp	al,3Bh
	je	macro_internal_symbol
	and	[macro_status],not 20h
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
	test	[macro_status],20h
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
	and	[macro_status],not 20h
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
	cmp	edx,[memory_end]
	je	not_macro_constant
	sub	edx,16
	cmp	eax,[edx]
	je	try_macro_constant
	cmp	ebp,[edx]
	jne	scan_macro_constants
      try_macro_constant:
	mov	ecx,ebp
	mov	edi,[edx+4]
	repe	cmps byte [esi],[edi]
	je	macro_constant_found
	mov	esi,ebx
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
      macro_internal_symbol:
	lods	byte [esi]
	movzx	eax,al
	add	esi,eax
      more_internal_symbols:
	lods	byte [esi]
	or	al,al
	jz	macro_line_processed
	cmp	al,1Ah
	je	macro_internal_symbol
	cmp	al,3Bh
	je	macro_internal_symbol
	cmp	al,22h
	je	macro_internal_string
	jmp	more_internal_symbols
      macro_internal_string:
	lods	dword [esi]
	add	esi,eax
	jmp	more_internal_symbols
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
	mov	[macro_line],esi
	add	esi,18
	jmp	process_macro_line
      local_symbols:
	lods	byte [esi]
	cmp	al,1Ah
	jne	invalid_argument
	mov	byte [edi-1],3Bh
	xor	al,al
	stos	byte [edi]
      make_local_symbol:
	push	ecx
	movzx	ecx,byte [esi]
	inc	esi
	mov	edx,[memory_end]
	sub	edx,16
	cmp	edx,edi
	jb	out_of_memory
	mov	[memory_end],edx
	mov	eax,[counter]
	shl	eax,8
	mov	al,cl
	mov	[edx],eax
	mov	[edx+4],esi
	mov	[edx+12],edi
	movzx	eax,[_counter]
	add	eax,ecx
	inc	eax
	cmp	eax,100h
	jae	name_too_long
	lea	ebp,[edi+2+eax]
	cmp	ebp,[memory_end]
	jae	out_of_memory
	mov	ah,al
	mov	al,1Ah
	stos	word [edi]
	rep	movs byte [edi],[esi]
	mov	al,'?'
	stos	byte [edi]
	push	esi
	mov	esi,_counter+1
	movzx	ecx,[_counter]
	rep	movs byte [edi],[esi]
	pop	esi
	mov	eax,edi
	sub	eax,[edx+12]
	mov	[edx+8],eax
	pop	ecx
	xor	al,al
	stos	byte [edi]
	lods	byte [esi]
	cmp	al,'}'
	je	macro_block_processed
	or	al,al
	jz	process_next_line
	cmp	al,','
	jne	extra_characters_on_line
	dec	edi
	lods	byte [esi]
	cmp	al,1Ah
	je	make_local_symbol
	jmp	invalid_argument
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
	mov	eax,[macro_line]
	mov	[macro_block_line],eax
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
	mov	eax,[macro_block_line]
	mov	[macro_line],eax
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
	mov	[memory_end],eax
	pop	eax
	and	al,0F0h
	and	[macro_status],0Fh
	or	[macro_status],al
	pop	[counter_limit] [counter]
	pop	[macro_block_line_number] [macro_block_line] [macro_block] [macro_constants]
	mov	esi,[struc_name]
	or	esi,esi
	jz	line_preprocessed
	movzx	ecx,byte [esi-1]
	and	[value_type],0F0h
	or	[value_type],1
	call	find_symbolic_constant
	jc	line_preprocessed
	mov	ecx,[edx+8]
	mov	ebx,[struc_name]
	movzx	eax,byte [ebx-1]
	lea	esi,[ebx+eax]
	add	eax,2
	sub	ecx,eax
	jz	replace_struc_name
	ja	move_struc_up
	mov	eax,ecx
	neg	eax
	mov	ecx,edi
	sub	ecx,esi
	mov	edi,esi
	sub	edi,eax
	call	move_data
	jmp	replace_struc_name
      move_struc_up:
	mov	eax,ecx
	mov	ecx,edi
	sub	ecx,esi
	mov	esi,edi
	add	edi,eax
	mov	ebx,edi
	dec	edi
	dec	esi
	std
	rep	movs byte [edi],[esi]
	cld
	mov	edi,ebx
      replace_struc_name:
	push	edi
	mov	edi,[struc_name]
	sub	edi,2
	mov	ecx,[edx+8]
	mov	esi,[edx+12]
	call	move_data
	pop	edi
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
	cmp	byte [eax+ecx],'F'
	jbe	digit_increased
	stc
	ret
      letter_digit:
	mov	byte [eax+ecx],'A'
      digit_increased:
	clc
	ret

symbol_characters db 26
 db 9,0Ah,0Dh,1Ah,20h,'+-/*:=|&~()[]<>{},;\#'

preprocessor_directives:
 db 7,'include'
 dw include_file-preprocessor
 db 5,'macro'
 dw define_macro-preprocessor
 db 5,'purge'
 dw purge_macro-preprocessor
 db 7,'restore'
 dw restore_equ_constant-preprocessor
 db 5,'struc'
 dw define_struc-preprocessor
 db 0

macro_directives:
 db 6,'common'
 dw common_block-preprocessor
 db 7,'forward'
 dw forward_block-preprocessor
 db 5,'local'
 dw local_symbols-preprocessor
 db 7,'reverse'
 dw reverse_block-preprocessor
 db 0
