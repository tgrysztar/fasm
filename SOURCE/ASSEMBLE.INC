
assembler:
	mov	eax,[memory_end]
	dec	eax
	mov	byte [eax],0
	mov	[symbols_list],eax
	mov	[current_pass],0
	mov	[next_pass_needed],0
      assembler_loop:
	mov	esi,[source_start]
	mov	edi,[code_start]
	mov	[org_start],edi
	mov	al,[default_code_type]
	mov	[code_type],al
	mov	[entry_code_type],al
	mov	[error_line],0
	mov	[current_section],0
	mov	[current_line],0
	mov	[home_line],0
	mov	eax,[relocations_memory]
	mov	[relocations_end],eax
	mov	eax,[exports_memory]
	mov	[exports_end],eax
	mov	eax,[import_relocations_memory]
	mov	[import_relocations_end],eax
      assemble_line:
	mov	eax,[current_line]
	mov	[current_line],esi
	cmp	byte [esi+4],20h
	je	line_start
	cmp	byte [esi+4],';'
	je	line_start
	mov	[home_line],esi
      line_start:
	add	esi,4
	cmp	esi,[code_start]
	jae	pass_done
	lodsb
	mov	[line_type],al
	mov	eax,[symbols_list]
	sub	eax,16
	cmp	edi,eax
	jae	out_of_memory
	call	assemble_instruction
	jmp	assemble_line
      pass_done:
	cmp	[virtual_mode],0
	je	pass_ok
	mov	[virtual_mode],0
	mov	eax,[stored_org]
	mov	[org_start],eax
	mov	edi,[stored_offset]
      pass_ok:
	cmp	[next_pass_needed],0
	je	assemble_done
	inc	[current_pass]
	cmp	[current_pass],100
	jae	code_cannot_be_generated
	mov	[next_pass_needed],0
	jmp	assembler_loop
      assemble_done:
	mov	eax,[error_line]
	or	eax,eax
	jz	assemble_ok
	mov	[current_line],eax
	mov	eax,[error_home_line]
	mov	[home_line],eax
	jmp	[error]
      assemble_ok:
	ret

assemble_instruction:
	cmp	[line_type],'{'
	je	find_end_of_line
      find_instruction:
	lodsb
	cmp	al,20h
	je	find_instruction
	or	al,al
	jz	line_ok
	cmp	al,';'
	je	find_end_of_line
	cmp	al,':'
	je	empty_label
	dec	esi
	push	edi
	mov	edx,esi
      find_instruction_end:
	lodsb
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	jne	find_instruction_end
	lea	ecx,[esi-1]
	mov	[instruction_end],ecx
	mov	esi,edx
	sub	ecx,esi
	or	ecx,ecx
	jz	empty_instruction
	mov	edi,instructions
	call	get_instruction
	pop	edi
	jc	unknown_instruction
	cmp	byte [esi],':'
	je	reserved_word_used_as_symbol
	cmp	[line_type],':'
	je	find_end_of_line
	cmp	[line_type],';'
	je	find_end_of_line
	jmp	ebx
      unknown_instruction:
	mov	ebx,esi
	mov	esi,[instruction_end]
	mov	ecx,esi
	sub	ecx,ebx
	cmp	byte [esi],':'
	je	simple_label
	cmp	[line_type],':'
	je	find_end_of_line
	cmp	[line_type],';'
	je	find_end_of_line
	lodsb
	cmp	al,'='
	je	constant
	cmp	al,20h
	je	data_label
	jmp	illegal_instruction
      empty_instruction:
	pop	edi
	or	al,al
	jz	line_assembled
	cmp	al,';'
	je	line_assembled
	jmp	illegal_instruction
      simple_label:
	inc	esi
	mov	eax,edi
	sub	eax,[org_start]
	xor	edx,edx
	call	create_symbol
	mov	byte [ebx+8],0
	mov	byte [ebx+10],1
	jmp	find_instruction
      empty_label:
	inc	[current_section]
	jmp	find_instruction
      constant:
	push	ebx ecx
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	cmp	al,1
	je	constant_byte
	cmp	al,2
	je	constant_word
	cmp	al,4
	je	constant_dword
	cmp	al,8
	je	constant_qword
	or	al,al
	jnz	invalid_operand_size
	cmp	[program_relocatable],0
	je	constant_dword
      constant_qword:
	call	get_qword_value
	pop	ecx ebx
	call	create_symbol
	mov	byte [ebx+8],0
	jmp	line_assembled
      constant_dword:
	call	get_dword_value
	mov	[temp_byte],ch
	xor	edx,edx
	pop	ecx ebx
	call	create_symbol
	mov	byte [ebx+8],0
	mov	ch,[temp_byte]
	mov	byte [ebx+10],ch
	jmp	line_assembled
      constant_word:
	call	get_word_value
	xor	edx,edx
	movzx	eax,ax
	pop	ecx ebx
	call	create_symbol
	mov	byte [ebx+8],0
	jmp	line_assembled
      constant_byte:
	call	get_byte_value
	xor	edx,edx
	movzx	eax,al
	pop	ecx ebx
	call	create_symbol
	mov	byte [ebx+8],0
	jmp	line_assembled
      data_label:
	lodsb
	cmp	al,20h
	je	data_label
	cmp	al,'='
	je	constant
	dec	esi
	push	ebx ecx edi
	mov	edx,esi
      find_data_instruction_end:
	lodsb
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	jne	find_data_instruction_end
	lea	ecx,[esi-1]
	mov	esi,edx
	sub	ecx,esi
	or	ecx,ecx
	jz	illegal_instruction
	mov	edi,data_instructions
	call	get_instruction
	jc	illegal_instruction
	mov	ebp,ebx
	pop	edi ecx ebx
	push	eax ebp
	mov	eax,edi
	sub	eax,[org_start]
	xor	edx,edx
	call	create_symbol
	pop	ebp eax
	mov	byte [ebx+8],al
	mov	byte [ebx+10],1
	jmp	ebp
      line_assembled:
	lodsb
	cmp	al,20h
	je	line_assembled
	cmp	al,';'
	je	find_end_of_line
	or	al,al
	jz	line_ok
	jmp	extra_characters_on_line
      find_end_of_line:
	lodsb
	or	al,al
	jnz	find_end_of_line
      line_ok:
	ret

org_directive:
	cmp	[program_format],0
	jnz	instruction_not_allowed
      set_org:
	call	get_dword_value
	mov	ecx,edi
	sub	ecx,eax
	mov	[org_start],ecx
	jmp	line_assembled
virtual_directive:
	push	edi
	mov	edi,special_operators
	call	get_operator
	pop	edi
	cmp	al,1
	jne	invalid_argument
	cmp	[virtual_mode],0
	jne	set_org
	mov	[virtual_mode],-1
	mov	eax,[org_start]
	mov	[stored_org],eax
	mov	[stored_offset],edi
	jmp	set_org
      virtual_end:
	cmp	[virtual_mode],1
	je	instruction_not_allowed
	mov	[virtual_mode],0
	mov	eax,[stored_org]
	mov	[org_start],eax
	mov	edi,[stored_offset]
	jmp	line_assembled
end_directive:
	push	edi
	mov	edi,end_operators
	call	get_operator
	pop	edi
	cmp	al,1
	je	virtual_end
	jmp	invalid_argument
code_directive:
	cmp	[program_format],0
	jnz	instruction_not_allowed
	push	edi
	mov	edi,code_types
	call	get_operator
	pop	edi
	or	al,al
	jz	invalid_value
	mov	[code_type],al
	mov	ebx,edi
	sub	ebx,[org_start]
	jne	line_assembled
	cmp	ebx,[entry_point]
	jne	line_assembled
	mov	[entry_code_type],al
	jmp	line_assembled
entry_directive:
	cmp	[program_format],0
	jz	instruction_not_allowed
	call	get_dword_value
	mov	[entry_point],eax
	mov	al,[code_type]
	mov	[entry_code_type],al
	jmp	line_assembled
export_directive:
	cmp	[program_relocatable],0
	je	instruction_not_allowed
      find_export_start:
	lodsb
	cmp	al,20h
	je	find_export_start
	dec	esi
	push	edi esi
      find_export_end:
	lodsb
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	jne	find_export_end
	dec	esi
	mov	ecx,esi
	mov	esi,[esp]
	sub	ecx,esi
	push	ecx
	call	get_symbol
	pop	edx ebx
	push	esi
	mov	esi,ebx
	or	cl,cl
	jz	invalid_value
	cmp	cl,3
	je	export_symbol_defined
	mov	[next_pass_needed],1
	jmp	export_symbol_ok
      export_symbol_defined:
	cmp	ch,1
	jne	invalid_value
      export_symbol_ok:
	mov	edi,[exports_end]
	mov	ecx,edx
	lea	edx,[edi+ecx+1+4]
	cmp	edx,[exports_memory_end]
	jae	out_of_memory
	rep	movsb
	mov	byte [edi],0
	inc	edi
	stosd
	mov	[exports_end],edi
	pop	esi edi
	jmp	line_assembled
import_directive:
	cmp	[program_format],0
	je	instruction_not_allowed
	cmp	[current_pass],0
	ja	skip_line
	push	edi
      find_import_start:
	lodsb
	cmp	al,20h
	je	find_import_start
	dec	esi
	push	esi
      find_import_end:
	lodsb
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	jne	find_import_end
	dec	esi
	mov	ecx,esi
	sub	ecx,[esp]
	push	ecx
	mov	edi,size_operators
	call	get_operator
	mov	[operand_size],al
	mov	edi,special_operators
	call	get_operator
	cmp	al,2
	jne	invalid_argument
      find_DLL_name:
	lodsb
	cmp	al,20h
	je	find_DLL_name
	cmp	al,27h
	jne	invalid_argument
	mov	edi,[imports_end]
      get_DLL_name:
	lodsb
	stosb
	or	al,al
	jz	missing_end_quote
	cmp	al,27h
	jne	get_DLL_name
	lodsb
	cmp	al,27h
	je	get_DLL_name
	dec	esi
	mov	byte [edi-1],0
	pop	ecx ebx
	push	esi
	mov	edx,ecx
	mov	esi,ebx
	rep	movsb
	xor	al,al
	stosb
	mov	[imports_end],edi
	mov	ecx,edx
	mov	esi,ebx
	mov	eax,[import_number]
	xor	edx,edx
	call	create_symbol
	mov	al,[operand_size]
	mov	byte [ebx+8],al
	mov	byte [ebx+10],2
	inc	[import_number]
	pop	esi edi
	jmp	line_assembled
      skip_line:
	lodsb
	or	al,al
	jnz	skip_line
	dec	esi
	jmp	line_assembled
label_directive:
	lodsb
	cmp	al,20h
	je	label_directive
	lea	ebx,[esi-1]
	push	edi
      find_name_end:
	lodsb
	mov	edi,special_characters+1
	movzx	ecx,[special_characters]
	repne	scasb
	jne	find_name_end
	dec	esi
	mov	ecx,esi
	sub	ecx,ebx
	push	ebx ecx
	mov	edi,size_operators
	call	get_operator
	mov	byte [value],al
	mov	edi,special_operators
	call	get_operator
	cmp	al,1
	je	label_at
	or	al,al
	jnz	extra_characters_on_line
	pop	ecx ebx
	pop	edi
	mov	eax,edi
	sub	eax,[org_start]
	xor	edx,edx
	jmp	create_label
      label_at:
	mov	edi,[esp+8]
	call	get_dword_value
	pop	ecx ebx
	pop	edi
	xor	edx,edx
      create_label:
	call	create_symbol
	mov	al,byte [value]
	mov	byte [ebx+8],al
	cmp	[program_relocatable],0
	je	line_assembled
	cmp	al,[value_type]
	mov	byte [ebx+10],al
	or	al,al
	jnz	line_assembled
	mov	byte [ebx+10],1
	jmp	line_assembled
times_directive:
	call	get_dword_value
	or	ch,ch
	jnz	invalid_use_of_symbol
	cmp	eax,0
	jz	find_end_of_line
	jl	invalid_value
	mov	ecx,eax
	push	esi
      repeat_instruction:
	mov	esi,[esp]
	push	ecx
	mov	al,[line_type]
	push	eax
	push	[current_line]
	call	assemble_instruction
	pop	[current_line]
	pop	eax
	mov	[line_type],al
	mov	eax,esi
	pop	ecx
	loop	repeat_instruction
	add	esp,4
	jmp	line_ok

data_bytes:
	lodsb
	cmp	al,20h
	je	data_bytes
	cmp	al,27h
	je	data_string
	cmp	al,'?'
	jne	get_byte
	mov	eax,edi
	mov	byte [edi],0
	inc	edi
	call	undefined_data
	jmp	byte_ok
      get_byte:
	dec	esi
	call	get_byte_value
	stosb
      byte_ok:
	cmp	edi,[symbols_list]
	jae	out_of_memory
      find_next_data_byte:
	lodsb
	cmp	al,20h
	je	find_next_data_byte
	cmp	al,','
	je	data_bytes
	dec	esi
	jmp	line_assembled
      data_string:
	call	get_string
	jmp	find_next_data_byte
      get_string:
	lodsb
	or	al,al
	jz	missing_end_quote
	cmp	al,27h
	je	string_end
      store_string_byte:
	stosb
	cmp	edi,[symbols_list]
	jae	out_of_memory
	jmp	get_string
      string_end:
	lodsb
	cmp	al,27h
	je	store_string_byte
	dec	esi
	ret
      undefined_data:
	cmp	eax,[undefined_data_end]
	je	mark_undefined_data
	mov	[undefined_data_start],eax
      mark_undefined_data:
	mov	[undefined_data_end],edi
	ret
data_words:
	lodsb
	cmp	al,20h
	je	data_words
	cmp	al,'?'
	jne	get_word
	mov	eax,edi
	mov	word [edi],0
	add	edi,2
	call	undefined_data
	jmp	word_ok
      get_word:
	dec	esi
	call	get_word_value
	stosw
      word_ok:
	cmp	edi,[symbols_list]
	jae	out_of_memory
      find_next_data_word:
	lodsb
	cmp	al,20h
	je	find_next_data_word
	cmp	al,','
	je	data_words
	dec	esi
	jmp	line_assembled
data_dwords:
	lodsb
	cmp	al,20h
	je	data_dwords
	cmp	al,'?'
	jne	get_dword
	mov	eax,edi
	mov	dword [edi],0
	add	edi,4
	call	undefined_data
	jmp	dword_ok
      get_dword:
	dec	esi
	call	get_dword_value
	or	ch,ch
	jz	store_data_dword
	cmp	ch,2
	je	data_imported_value
	call	mark_relocation
	jmp	store_data_dword
      data_imported_value:
	call	mark_import_relocation
	call	mark_relocation
      store_data_dword:
	stosd
      dword_ok:
	cmp	edi,[symbols_list]
	jae	out_of_memory
      find_next_data_dword:
	lodsb
	cmp	al,20h
	je	find_next_data_dword
	cmp	al,','
	je	data_dwords
	dec	esi
	jmp	line_assembled
data_pwords:
	lodsb
	cmp	al,20h
	je	data_pwords
	cmp	al,'?'
	jne	get_pword
	mov	eax,edi
	mov	dword [edi],0
	mov	word [edi+4],0
	add	edi,6
	call	undefined_data
	jmp	pword_ok
      get_pword:
	dec	esi
	call	get_pword_value
	stosd
	mov	eax,edx
	stosw
      pword_ok:
	cmp	edi,[symbols_list]
	jae	out_of_memory
      find_next_data_pword:
	lodsb
	cmp	al,20h
	je	find_next_data_pword
	cmp	al,','
	je	data_pwords
	dec	esi
	jmp	line_assembled
data_qwords:
	lodsb
	cmp	al,20h
	je	data_qwords
	cmp	al,'?'
	jne	get_qword
	mov	eax,edi
	mov	dword [edi],0
	mov	dword [edi+4],0
	add	edi,8
	call	undefined_data
	jmp	qword_ok
      get_qword:
	dec	esi
	call	get_qword_value
	stosd
	mov	eax,edx
	stosd
      qword_ok:
	cmp	edi,[symbols_list]
	jae	out_of_memory
      find_next_data_qword:
	lodsb
	cmp	al,20h
	je	find_next_data_qword
	cmp	al,','
	je	data_qwords
	dec	esi
	jmp	line_assembled
data_file:
	lodsb
	cmp	al,20h
	je	data_file
	cmp	al,27h
	jne	invalid_operand
	mov	edx,edi
	call	get_string
	xor	al,al
	stosb
	mov	edi,edx
	mov	ax,3D00h
	int	21h
	jc	file_not_found
	mov	ax,4202h
	xor	edx,edx
	int	21h
	mov	ecx,eax
	cmp	[next_pass_needed],0
	jne	data_reserve
	mov	ax,4200h
	int	21h
	mov	edx,edi
	add	edi,ecx
	jc	out_of_memory
	cmp	edi,[symbols_list]
	jae	out_of_memory
	mov	ah,3Fh
	int	21h
	mov	ah,3Eh
	int	21h
	jmp	line_assembled
      data_reserve:
	mov	ah,3Eh
	int	21h
	add	edi,ecx
	jc	out_of_memory
	cmp	edi,[symbols_list]
	jae	out_of_memory
	jmp	line_assembled
reserve_bytes:
	push	edi
	call	get_dword_value
	or	ch,ch
	jnz	invalid_use_of_symbol
	mov	ecx,eax
	mov	edx,ecx
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[symbols_list]
	jae	out_of_memory
	xor	al,al
	rep	stosb
      reserved_data:
	pop	eax
	call	undefined_data
	jmp	line_assembled
reserve_words:
	push	edi
	call	get_dword_value
	or	ch,ch
	jnz	invalid_use_of_symbol
	mov	ecx,eax
	mov	edx,ecx
	shl	edx,1
	jc	out_of_memory
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[symbols_list]
	jae	out_of_memory
	xor	eax,eax
	rep	stosw
	jmp	reserved_data
reserve_dwords:
	push	edi
	call	get_dword_value
	or	ch,ch
	jnz	invalid_use_of_symbol
	mov	ecx,eax
	mov	edx,ecx
	shl	edx,1
	jc	out_of_memory
	shl	edx,1
	jc	out_of_memory
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[symbols_list]
	jae	out_of_memory
	xor	eax,eax
	rep	stosd
	jmp	reserved_data
reserve_pwords:
	push	edi
	call	get_dword_value
	or	ch,ch
	jnz	invalid_use_of_symbol
	mov	ecx,eax
	shl	ecx,1
	jc	out_of_memory
	add	ecx,eax
	mov	edx,ecx
	shl	edx,1
	jc	out_of_memory
	add	edx,edi
	jc	out_of_memory
	cmp	edx,[symbols_list]
	jae	out_of_memory
	xor	eax,eax
	rep	stosw
	jmp	reserved_data
reserve_qwords:
	push	edi
	call	get_dword_value
	or	ch,ch
	jnz	invalid_use_of_symbol
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
	cmp	edx,[symbols_list]
	jae	out_of_memory
	xor	eax,eax
	rep	stosd
	jmp	reserved_data

simple_instruction:
	stosb
	jmp	line_assembled
simple_instruction_16bit:
	cmp	[code_type],32
	je	size_prefix
	stosb
	jmp	line_assembled
      size_prefix:
	mov	ah,al
	mov	al,66h
	stosw
	jmp	line_assembled
simple_instruction_32bit:
	cmp	[code_type],16
	je	size_prefix
	stosb
	jmp	line_assembled
simple_extended_instruction:
	mov	ah,al
	mov	al,0Fh
	stosw
	jmp	line_assembled
prefix_instruction:
	stosb
	jmp	find_instruction
basic_instruction:
	mov	[base_code],al
	call	get_memory_address
	jc	basic_reg
	push	eax ebx ecx
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	basic_mem_imm
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	basic_mem_reg_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      basic_mem_reg_size_ok:
	pop	ecx ebx edx
	cmp	ah,1
	je	basic_mem_reg_8bit
	cmp	ah,2
	je	basic_mem_reg_16bit
	cmp	ah,4
	je	basic_mem_reg_32bit
	jmp	invalid_operand_size
      basic_mem_reg_8bit:
	call	store_instruction
	jmp	line_assembled
      basic_mem_reg_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      basic_mem_reg_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      basic_mem_imm:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	basic_mem_imm_size_ok
	xchg	al,[operand_size]
	or	al,al
	jz	basic_mem_imm_size_ok
	cmp	al,[operand_size]
	jne	operand_sizes_do_not_match
      basic_mem_imm_size_ok:
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
	pop	ecx ebx edx
	mov	[base_code],80h
	call	store_instruction
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      basic_mem_imm_16bit:
	call	get_word_value
	mov	word [value],ax
	mov	al,[base_code]
	shr	al,3
	mov	[postbyte_register],al
	call	operand_16bit_prefix
	pop	ecx ebx edx
	cmp	word [value],80h
	jb	basic_mem_simm_8bit
	cmp	word [value],-80h
	jae	basic_mem_simm_8bit
	mov	[base_code],81h
	call	store_instruction
	mov	ax,word [value]
	stosw
	jmp	line_assembled
      basic_mem_simm_8bit:
	mov	[base_code],83h
	call	store_instruction
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      basic_mem_imm_32bit:
	push	[import_value]
	call	get_dword_value
	mov	ebp,[import_value]
	pop	[import_value]
	mov	dword [value],eax
	mov	al,[base_code]
	shr	al,3
	mov	[postbyte_register],al
	call	operand_32bit_prefix
	pop	ecx ebx edx
	cmp	[value_type],0
	jne	basic_mem_imm_relocation
	cmp	dword [value],80h
	jb	basic_mem_simm_8bit
	cmp	dword [value],-80h
	jae	basic_mem_simm_8bit
	mov	[base_code],81h
	call	store_instruction
	mov	eax,dword [value]
	stosd
	jmp	line_assembled
      basic_mem_imm_relocation:
	mov	[base_code],81h
	call	store_instruction
	mov	eax,dword [value]
	cmp	[value_type],2
	je	basic_mem_imported
	call	mark_relocation
	stosd
	jmp	line_assembled
      basic_mem_imported:
	mov	[import_value],ebp
	call	mark_import_relocation
	call	mark_relocation
	stosd
	jmp	line_assembled
      basic_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	[postbyte_register],al
	call	find_next_operand
	call	get_memory_address
	jc	basic_reg_reg
	mov	edx,eax
	mov	al,[postbyte_register]
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	basic_reg_mem_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      basic_reg_mem_size_ok:
	cmp	ah,1
	je	basic_reg_mem_8bit
	cmp	ah,2
	je	basic_reg_mem_16bit
	cmp	ah,4
	je	basic_reg_mem_32bit
	jmp	invalid_operand_size
      basic_reg_mem_8bit:
	add	[base_code],2
	call	store_instruction
	jmp	line_assembled
      basic_reg_mem_16bit:
	call	operand_16bit_prefix
	add	[base_code],3
	call	store_instruction
	jmp	line_assembled
      basic_reg_mem_32bit:
	call	operand_32bit_prefix
	add	[base_code],3
	call	store_instruction
	jmp	line_assembled
      basic_reg_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	basic_reg_imm
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	mov	bh,bl
	shr	bh,4
	and	bl,111b
	mov	[postbyte_register],bl
	cmp	ah,bh
	jne	operand_sizes_do_not_match
	shl	al,3
	or	bl,al
	or	bl,11000000b
	cmp	ah,1
	je	basic_reg_reg_8bit
	cmp	ah,2
	je	basic_reg_reg_16bit
	cmp	ah,4
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
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
      basic_reg_imm:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	mov	ah,[postbyte_register]
	shr	ah,4
	or	al,al
	jz	basic_reg_imm_size_ok
	xchg	al,ah
	cmp	al,ah
	jne	operand_sizes_do_not_match
      basic_reg_imm_size_ok:
	mov	al,ah
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
	stosw
	mov	al,dl
	stosb
	jmp	line_assembled
      basic_al_imm:
	mov	al,[base_code]
	add	al,4
	stosb
	mov	al,dl
	stosb
	jmp	line_assembled
      basic_reg_imm_16bit:
	call	get_word_value
	mov	edx,eax
	call	operand_16bit_prefix
	mov	ah,[base_code]
	or	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	ah,bl
	cmp	dx,80h
	jb	basic_reg_simm_8bit
	cmp	dx,-80h
	jae	basic_reg_simm_8bit
	or	bl,bl
	jz	basic_ax_imm
	mov	al,81h
	stosw
	mov	eax,edx
	stosw
	jmp	line_assembled
      basic_reg_simm_8bit:
	mov	al,83h
	stosw
	mov	eax,edx
	stosb
	jmp	line_assembled
      basic_ax_imm:
	mov	al,[base_code]
	add	al,5
	stosb
	mov	ax,dx
	stosw
	jmp	line_assembled
      basic_reg_imm_32bit:
	call	get_dword_value
	mov	edx,eax
	call	operand_32bit_prefix
	mov	ah,[base_code]
	or	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	ah,bl
	or	ch,ch
	jnz	basic_reg_imm_relocation
	cmp	edx,80h
	jb	basic_reg_simm_8bit
	cmp	edx,-80h
	jae	basic_reg_simm_8bit
	or	bl,bl
	jz	basic_eax_imm
	mov	al,81h
	stosw
	mov	eax,edx
	stosd
	jmp	line_assembled
      basic_reg_imm_relocation:
	or	bl,bl
	jz	basic_eax_imm_relocation
	mov	al,81h
	stosw
	mov	eax,edx
	cmp	ch,2
	je	basic_reg_imported
	call	mark_relocation
	stosd
	jmp	line_assembled
      basic_reg_imported:
	call	mark_import_relocation
	call	mark_relocation
	stosd
	jmp	line_assembled
      basic_eax_imm:
	mov	al,[base_code]
	add	al,5
	stosb
	mov	eax,edx
	stosd
	jmp	line_assembled
      basic_eax_imm_relocation:
	mov	al,[base_code]
	add	al,5
	stosb
	mov	eax,edx
	cmp	ch,2
	je	basic_eax_imported
	call	mark_relocation
	stosd
	jmp	line_assembled
      basic_eax_imported:
	call	mark_import_relocation
	call	mark_relocation
	stosd
	jmp	line_assembled
single_operand_instruction:
	mov	[base_code],0F6h
	mov	[postbyte_register],al
	call	get_memory_address
	jc	single_operand_reg
	mov	edx,eax
	mov	al,[operand_size]
	cmp	al,1
	je	single_operand_mem_8bit
	cmp	al,2
	je	single_operand_mem_16bit
	cmp	al,4
	je	single_operand_mem_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      single_operand_mem_8bit:
	call	store_instruction
	jmp	line_assembled
      single_operand_mem_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      single_operand_mem_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      single_operand_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	cmp	ah,1
	je	single_operand_reg_8bit
	cmp	ah,2
	je	single_operand_reg_16bit
	cmp	ah,4
	je	single_operand_reg_32bit
	jmp	invalid_operand_size
      single_operand_reg_8bit:
	mov	ah,bl
	mov	al,0F6h
	stosw
	jmp	line_assembled
      single_operand_reg_16bit:
	call	operand_16bit_prefix
	mov	ah,bl
	mov	al,0F7h
	stosw
	jmp	line_assembled
      single_operand_reg_32bit:
	call	operand_32bit_prefix
	mov	ah,bl
	mov	al,0F7h
	stosw
	jmp	line_assembled
imul_instruction:
	mov	[base_code],0F6h
	mov	[postbyte_register],5
	call	get_memory_address
	jc	imul_reg
	mov	edx,eax
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
	jmp	line_assembled
      imul_mem_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      imul_mem_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      imul_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
      imul_reg_check_operand:
	lodsb
	cmp	al,20h
	je	imul_reg_check_operand
	cmp	al,','
	je	imul_reg_mem
	dec	esi
	mov	al,ah
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
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
	stosw
	jmp	line_assembled
      imul_reg_16bit:
	call	operand_16bit_prefix
	mov	ah,bl
	mov	al,0F7h
	stosw
	jmp	line_assembled
      imul_reg_32bit:
	call	operand_32bit_prefix
	mov	ah,bl
	mov	al,0F7h
	stosw
	jmp	line_assembled
      imul_reg_mem:
	mov	[postbyte_register],ah
	call	get_memory_address
	jc	imul_reg_reg
	push	eax ebx ecx
	mov	al,[postbyte_register]
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	xchg	ah,[operand_size]
	or	ah,ah
	jz	imul_reg_mem_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      imul_reg_mem_size_ok:
	lodsb
	cmp	al,20h
	je	imul_reg_mem_size_ok
	cmp	al,','
	je	imul_reg_mem_imm
	dec	esi
	cmp	[operand_size],2
	je	imul_reg_mem_16bit
	cmp	[operand_size],4
	je	imul_reg_mem_32bit
	jmp	invalid_operand_size
      imul_reg_mem_16bit:
	call	operand_16bit_prefix
	jmp	imul_reg_mem_store
      imul_reg_mem_32bit:
	call	operand_32bit_prefix
      imul_reg_mem_store:
	pop	ecx ebx edx
	mov	[base_code],0Fh
	mov	[extended_code],AFh
	call	store_instruction
	jmp	line_assembled
      imul_reg_mem_imm:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	cmp	al,1
	je	imul_reg_mem_imm_8bit
	cmp	al,2
	je	imul_reg_mem_imm_16bit
	cmp	al,4
	je	imul_reg_mem_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[operand_size],2
	je	imul_reg_mem_imm_16bit
	cmp	[operand_size],4
	je	imul_reg_mem_imm_32bit
	jmp	invalid_operand_size
      imul_reg_mem_imm_8bit:
	call	get_byte_value
	mov	byte [value],al
	pop	ecx ebx edx
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
	stosb
	jmp	line_assembled
      imul_reg_mem_32bit_imm_8bit:
	call	operand_32bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      imul_reg_mem_imm_16bit:
	call	get_word_value
	mov	word [value],ax
	pop	ecx ebx edx
	mov	[base_code],69h
	cmp	[operand_size],2
	jne	invalid_operand_size
	call	operand_16bit_prefix
	call	store_instruction
	mov	ax,word [value]
	stosw
	jmp	line_assembled
      imul_reg_mem_imm_32bit:
	call	get_dword_value
	or	ch,ch
	jnz	invalid_use_of_symbol
	mov	dword [value],eax
	pop	ecx ebx edx
	mov	[base_code],69h
	cmp	[operand_size],4
	jne	invalid_operand_size
	call	operand_32bit_prefix
	call	store_instruction
	mov	eax,dword [value]
	stosd
	jmp	line_assembled
      imul_reg_imm:
	mov	al,[postbyte_register]
	mov	ah,al
	shr	ah,4
	and	al,111b
	push	eax eax
	jmp	imul_reg_reg_imm
      imul_reg_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	imul_reg_imm
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	mov	bh,bl
	shr	bh,4
	and	bl,111b
	cmp	ah,bh
	jne	operand_sizes_do_not_match
	push	eax ebx
      imul_reg_reg_check_operand:
	lodsb
	cmp	al,20h
	je	imul_reg_reg_check_operand
	cmp	al,','
	je	imul_reg_reg_imm
	dec	esi
	pop	ebx edx
	cmp	bh,2
	je	imul_reg_reg_16bit
	cmp	bh,4
	je	imul_reg_reg_32bit
	jmp	invalid_operand_size
      imul_reg_reg_16bit:
	call	operand_16bit_prefix
	jmp	imul_reg_reg_store
      imul_reg_reg_32bit:
	call	operand_32bit_prefix
      imul_reg_reg_store:
	mov	ax,AF0Fh
	stosw
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stosb
	jmp	line_assembled
      imul_reg_reg_imm:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	cmp	al,1
	je	imul_reg_reg_imm_8bit
	cmp	al,2
	je	imul_reg_reg_imm_16bit
	cmp	al,4
	je	imul_reg_reg_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	mov	bh,[esp+1]
	cmp	bh,2
	je	imul_reg_reg_imm_16bit
	cmp	bh,4
	je	imul_reg_reg_imm_32bit
	jmp	invalid_operand_size
      imul_reg_reg_imm_8bit:
	call	get_byte_value
	mov	byte [value],al
	pop	ebx edx
	cmp	bh,2
	je	imul_reg_reg_16bit_imm_8bit
	cmp	bh,4
	je	imul_reg_reg_32bit_imm_8bit
	jmp	invalid_operand_size
      imul_reg_reg_16bit_imm_8bit:
	call	operand_16bit_prefix
	mov	al,6Bh
	stosb
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stosb
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      imul_reg_reg_32bit_imm_8bit:
	call	operand_32bit_prefix
	mov	al,6Bh
	stosb
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stosb
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      imul_reg_reg_imm_16bit:
	call	get_word_value
	mov	word [value],ax
	pop	ebx edx
	call	operand_16bit_prefix
	mov	al,69h
	stosb
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stosb
	mov	ax,word [value]
	stosw
	jmp	line_assembled
      imul_reg_reg_imm_32bit:
	call	get_dword_value
	or	ch,ch
	jnz	invalid_use_of_symbol
	mov	dword [value],eax
	pop	ebx edx
	call	operand_32bit_prefix
	mov	al,69h
	stosb
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stosb
	mov	eax,dword [value]
	stosd
	jmp	line_assembled
aa_instruction:
	push	eax
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jnz	aa_imm8
      aa_find_operand:
	lodsb
	cmp	al,20h
	je	aa_find_operand
	dec	esi
	mov	bl,10
	or	al,al
	jz	aa_store
	cmp	al,3Bh
	je	aa_store
	mov	al,1
      aa_imm8:
	cmp	al,1
	ja	invalid_operand_size
	call	get_byte_value
	mov	bl,al
      aa_store:
	pop	eax
	mov	ah,bl
	stosw
	jmp	line_assembled
int_instruction:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	cmp	al,1
	ja	invalid_operand_size
	call	get_byte_value
	mov	ah,al
	mov	al,0CDh
	stosw
	jmp	line_assembled
push_instruction:
	call	get_memory_address
	jc	push_reg
	mov	edx,eax
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
	mov	[base_code],FFh
	mov	[postbyte_register],110b
	call	store_instruction
	jmp	push_done
      push_mem_32bit:
	call	operand_32bit_prefix
	mov	[base_code],FFh
	mov	[postbyte_register],110b
	call	store_instruction
	jmp	push_done
      push_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	push_sreg
	pop	edi
	mov	dl,al
	shr	al,4
	and	dl,111b
	add	dl,50h
	cmp	al,2
	je	push_reg_16bit
	cmp	al,4
	je	push_reg_32bit
	jmp	invalid_operand_size
      push_reg_16bit:
	call	operand_16bit_prefix
	mov	al,dl
	stosb
	jmp	push_done
      push_reg_32bit:
	call	operand_32bit_prefix
	mov	al,dl
	stosb
	jmp	push_done
      push_sreg:
	mov	edi,segment_registers
	call	get_register
	jc	push_imm
	pop	edi
	dec	al
	cmp	al,4
	jae	push_sreg_386
	shl	al,3
	add	al,6
	stosb
	jmp	push_done
      push_sreg_386:
	sub	al,4
	shl	al,3
	mov	ah,0A0h
	add	ah,al
	mov	al,0Fh
	stosw
	jmp	push_done
      push_imm:
	mov	edi,size_operators
	call	get_operator
	pop	edi
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
	or	ch,ch
	jnz	push_imm_32bit_relocation
	cmp	eax,100h
	jb	push_imm_8bit
	mov	al,68h
	stosb
	mov	eax,edx
	stosd
	jmp	push_done
      push_imm_32bit_relocation:
	mov	al,68h
	stosb
	mov	eax,edx
	cmp	ch,2
	je	push_imported
	call	mark_relocation
	stosd
	jmp	push_done
      push_imported:
	call	mark_import_relocation
	call	mark_relocation
	stosd
	jmp	push_done
      push_imm_optimized_16bit:
	call	get_word_value
	cmp	eax,100h
	jb	push_imm_8bit
	mov	edx,eax
	mov	al,68h
	stosb
	mov	eax,edx
	stosw
	jmp	push_done
      push_imm_8bit:
	mov	ah,al
	mov	al,6Ah
	stosw
	jmp	push_done
      push_imm_16bit:
	call	get_word_value
	mov	edx,eax
	call	operand_16bit_prefix
	mov	al,68h
	stosb
	mov	eax,edx
	stosw
	jmp	push_done
      push_imm_32bit:
	call	get_dword_value
	mov	edx,eax
	call	operand_32bit_prefix
	or	ch,ch
	jnz	push_imm_32bit_relocation
	mov	al,68h
	stosb
	mov	eax,edx
	stosd
      push_done:
	lodsb
	cmp	al,20h
	je	push_done
	dec	esi
	cmp	al,';'
	je	line_assembled
	or	al,al
	jz	line_assembled
	jmp	push_instruction
pop_instruction:
	call	get_memory_address
	jc	pop_reg
	mov	edx,eax
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
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	pop_sreg
	pop	edi
	mov	dl,al
	shr	al,4
	and	dl,111b
	add	dl,58h
	cmp	al,2
	je	pop_reg_16bit
	cmp	al,4
	je	pop_reg_32bit
	jmp	invalid_operand_size
      pop_reg_16bit:
	call	operand_16bit_prefix
	mov	al,dl
	stosb
	jmp	pop_done
      pop_reg_32bit:
	call	operand_32bit_prefix
	mov	al,dl
	stosb
	jmp	pop_done
      pop_sreg:
	mov	edi,segment_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	dec	al
	cmp	al,1
	je	illegal_instruction
	cmp	al,4
	jae	pop_sreg_386
	shl	al,3
	add	al,7
	stosb
	jmp	pop_done
      pop_sreg_386:
	sub	al,4
	shl	al,3
	mov	ah,A1h
	add	ah,al
	mov	al,0Fh
	stosw
	jmp	pop_done
      pop_done:
	lodsb
	cmp	al,20h
	je	pop_done
	dec	esi
	cmp	al,';'
	je	line_assembled
	or	al,al
	jz	line_assembled
	jmp	pop_instruction
inc_instruction:
	mov	[base_code],al
	call	get_memory_address
	jc	inc_reg
	mov	edx,eax
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
	mov	al,FEh
	xchg	al,[base_code]
	mov	[postbyte_register],al
	call	store_instruction
	jmp	line_assembled
      inc_mem_16bit:
	call	operand_16bit_prefix
	mov	al,FFh
	xchg	al,[base_code]
	mov	[postbyte_register],al
	call	store_instruction
	jmp	line_assembled
      inc_mem_32bit:
	call	operand_32bit_prefix
	mov	al,FFh
	xchg	al,[base_code]
	mov	[postbyte_register],al
	call	store_instruction
	jmp	line_assembled
      inc_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	dl,al
	and	dl,111b
	shr	al,4
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
	stosw
	jmp	line_assembled
      inc_reg_16bit:
	call	operand_16bit_prefix
	mov	al,dl
	stosb
	jmp	line_assembled
      inc_reg_32bit:
	call	operand_32bit_prefix
	mov	al,dl
	stosb
	jmp	line_assembled
bound_instruction:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	mov	[postbyte_register],al
	call	find_next_operand
	mov	edi,[esp]
	call	get_memory_address
	jc	invalid_operand
	pop	edi
	mov	edx,eax
	mov	al,[postbyte_register]
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	bound_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      bound_size_ok:
	cmp	ah,2
	je	bound_16bit
	cmp	ah,4
	je	bound_32bit
	jmp	invalid_operand_size
      bound_32bit:
	call	operand_32bit_prefix
	mov	[base_code],62h
	call	store_instruction
	jmp	line_assembled
      bound_16bit:
	call	operand_16bit_prefix
	mov	[base_code],62h
	call	store_instruction
	jmp	line_assembled
arpl_instruction:
	mov	[base_code],al
	call	get_memory_address
	jc	invalid_operand
	push	eax ebx ecx
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	arpl_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      arpl_size_ok:
	cmp	ah,2
	jne	invalid_operand_size
	mov	[base_code],63h
	pop	ecx ebx edx
	call	store_instruction
	jmp	line_assembled
conditional_jump:
	mov	[base_code],al
	push	edi
	mov	edi,jump_operators
	call	get_operator
	mov	[jump_type],al
	mov	edi,size_operators
	call	get_operator
	pop	edi
	cmp	[jump_type],2
	je	invalid_operand
	push	eax
	call	get_dword_value
	pop	ebx
	sub	eax,[current_offset]
	add	eax,[org_start]
	sub	eax,2
	cmp	ch,2
	je	conditional_jump_imported
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
	stosw
	mov	eax,edx
	stosd
	jmp	line_assembled
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
	stosw
	mov	eax,edx
	stosw
	cmp	eax,8000h
	jge	jump_out_of_range
	cmp	eax,-8000h
	jl	jump_out_of_range
	jmp	line_assembled
      conditional_jump_8bit:
	mov	edx,eax
	mov	ah,al
	mov	al,[base_code]
	stosw
	cmp	edx,80h
	jge	jump_out_of_range
	cmp	edx,-80h
	jl	jump_out_of_range
	jmp	line_assembled
      jump_out_of_range:
	cmp	[error_line],0
	jne	line_assembled
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	eax,[home_line]
	mov	[error_home_line],eax
	mov	[error],relative_jump_out_of_range
	jmp	line_assembled
      conditional_jump_imported:
	or	bl,bl
	jz	conditional_jump_imported_32bit
	cmp	bl,4
	jne	invalid_use_of_symbol
      conditional_jump_imported_32bit:
	sub	eax,4
	mov	edx,eax
	mov	ecx,edi
	call	operand_32bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	ah,[base_code]
	add	ah,10h
	mov	al,0Fh
	stosw
	mov	eax,edx
	call	mark_import_relocation
	stosd
	jmp	line_assembled
jcxz_instruction:
	mov	[base_code],al
	push	edi
	mov	edi,jump_operators
	call	get_operator
	mov	[jump_type],al
	mov	edi,size_operators
	call	get_operator
	mov	[operand_size],al
	pop	edi
	cmp	[jump_type],2
	je	invalid_operand
	call	get_dword_value
	cmp	ch,2
	je	invalid_use_of_symbol
	sub	eax,[current_offset]
	add	eax,[org_start]
	mov	bl,[operand_size]
	cmp	bl,1
	je	jcxz_8bit
	or	bl,bl
	jnz	invalid_operand_size
      jcxz_8bit:
	sub	eax,2
	mov	edx,eax
	mov	ecx,edi
	call	address_16bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	al,0E3h
	stosb
	mov	eax,edx
	stosb
	cmp	eax,80h
	jge	jump_out_of_range
	cmp	eax,-80h
	jl	jump_out_of_range
	jmp	line_assembled
jecxz_instruction:
	mov	[base_code],al
	push	edi
	mov	edi,jump_operators
	call	get_operator
	mov	[jump_type],al
	mov	edi,size_operators
	call	get_operator
	mov	[operand_size],al
	pop	edi
	cmp	[jump_type],2
	je	invalid_operand
	call	get_dword_value
	cmp	ch,2
	je	invalid_use_of_symbol
	sub	eax,[current_offset]
	add	eax,[org_start]
	mov	bl,[operand_size]
	cmp	bl,1
	je	jecxz_8bit
	or	bl,bl
	jnz	invalid_operand_size
      jecxz_8bit:
	sub	eax,2
	mov	edx,eax
	mov	ecx,edi
	call	address_32bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	al,0E3h
	stosb
	mov	eax,edx
	stosb
	cmp	eax,80h
	jge	jump_out_of_range
	cmp	eax,-80h
	jl	jump_out_of_range
	jmp	line_assembled
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
	push	edi
	mov	edi,jump_operators
	call	get_operator
	mov	[jump_type],al
	mov	edi,size_operators
	call	get_operator
	mov	[operand_size],al
	pop	edi
	cmp	[jump_type],2
	je	invalid_operand
	call	get_dword_value
	cmp	ch,2
	je	invalid_use_of_symbol
	sub	eax,[current_offset]
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
	stosb
	mov	eax,edx
	stosb
	cmp	eax,80h
	jge	jump_out_of_range
	cmp	eax,-80h
	jl	jump_out_of_range
	jmp	line_assembled
call_instruction:
	push	edi
	mov	edi,jump_operators
	call	get_operator
	mov	[jump_type],al
	pop	edi
	call	get_memory_address
	jc	call_reg
	mov	[base_code],FFh
	mov	edx,eax
	mov	al,[operand_size]
	or	al,al
	jz	call_mem_size_not_specified
	cmp	al,2
	je	call_mem_16bit
	cmp	al,4
	je	call_mem_32bit
	cmp	al,6
	je	call_mem_48bit
	jmp	invalid_operand_size
      call_mem_size_not_specified:
	cmp	[jump_type],2
	je	call_mem_far
	cmp	[jump_type],1
	je	call_mem_near
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      call_mem_near:
	cmp	[code_type],16
	je	call_mem_16bit
	jmp	call_mem_near_32bit
      call_mem_far:
	cmp	[code_type],16
	je	call_mem_far_32bit
      call_mem_48bit:
	cmp	[jump_type],1
	je	invalid_operand_size
	call	operand_32bit_prefix
	mov	[postbyte_register],011b
	call	store_instruction
	jmp	line_assembled
      call_mem_32bit:
	cmp	[jump_type],2
	je	call_mem_far_32bit
	cmp	[jump_type],1
	je	call_mem_near_32bit
	cmp	[code_type],16
	je	call_mem_far_32bit
      call_mem_near_32bit:
	call	operand_32bit_prefix
	mov	[postbyte_register],010b
	call	store_instruction
	jmp	line_assembled
      call_mem_far_32bit:
	call	operand_16bit_prefix
	mov	[postbyte_register],011b
	call	store_instruction
	jmp	line_assembled
      call_mem_16bit:
	cmp	[jump_type],2
	je	invalid_operand_size
	call	operand_16bit_prefix
	mov	[postbyte_register],010b
	call	store_instruction
	jmp	line_assembled
      call_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	call_imm
	mov	bl,al
	and	bl,111b
	or	bl,11000000b
	shr	al,4
	cmp	al,2
	je	call_reg_16bit
	cmp	al,4
	je	call_reg_32bit
	jmp	invalid_operand_size
      call_reg_32bit:
	cmp	[jump_type],2
	je	call_reg_far32bit
	cmp	[jump_type],1
	je	call_reg_near32bit
	cmp	[code_type],16
	je	call_reg_far32bit
      call_reg_near32bit:
	call	operand_32bit_prefix
	or	bl,010b shl 3
	mov	ah,bl
	mov	al,FFh
	stosw
	jmp	line_assembled
      call_reg_far32bit:
	call	operand_32bit_prefix
	or	bl,011b shl 3
	mov	ah,bl
	mov	al,FFh
	stosw
	jmp	line_assembled
      call_reg_16bit:
	cmp	[jump_type],2
	je	invalid_operand_size
	call	operand_16bit_prefix
	or	bl,010b shl 3
	mov	ah,bl
	mov	al,FFh
	stosw
	jmp	line_assembled
      call_imm:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	push	esi
	push	eax
	call	get_dword_value
	pop	ebx
	mov	[operand_size],bl
	pop	ebx
	cmp	ch,2
	je	call_imported
	cmp	byte [esi],':'
	je	call_far
	cmp	[jump_type],2
	je	invalid_operand
	sub	eax,[current_offset]
	add	eax,[org_start]
	dec	eax
	mov	bl,[operand_size]
	cmp	bl,2
	je	call_16bit
	cmp	bl,4
	je	call_32bit
	or	bl,bl
	jnz	invalid_operand_size
	cmp	[code_type],32
	je	call_32bit
      call_16bit:
	sub	eax,2
	mov	edx,eax
	mov	ecx,edi
	call	operand_16bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	al,0E8h
	stosb
	mov	eax,edx
	stosw
	cmp	eax,8000h
	jge	jump_out_of_range
	cmp	eax,-8000h
	jl	jump_out_of_range
	jmp	line_assembled
      call_32bit:
	sub	eax,4
	mov	edx,eax
	mov	ecx,edi
	call	operand_32bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	al,0E8h
	stosb
	mov	eax,edx
	stosd
	jmp	line_assembled
      call_far:
	cmp	[jump_type],1
	je	invalid_operand
	mov	esi,ebx
	call	get_word_value
	mov	bl,[operand_size]
	cmp	bl,2
	je	call_far_size_ok
	or	bl,bl
	jnz	invalid_operand_size
      call_far_size_ok:
	inc	esi
	push	eax
	push	edi
	mov	edi,size_operators
	call	get_operator
	mov	[operand_size],al
	pop	edi
	call	get_dword_value
	pop	edx
	mov	ebx,eax
	mov	cl,[operand_size]
	cmp	cl,2
	je	call_far_16bit
	cmp	cl,4
	je	call_far_32bit
	or	cl,cl
	jnz	invalid_operand_size
	cmp	[code_type],32
	je	call_far_32bit
      call_far_16bit:
	call	operand_16bit_prefix
	mov	al,9Ah
	stosb
	mov	eax,ebx
	stosw
	mov	eax,edx
	stosw
	jmp	line_assembled
      call_far_32bit:
	call	operand_32bit_prefix
	mov	al,9Ah
	stosb
	mov	eax,ebx
	stosd
	mov	eax,edx
	stosw
	jmp	line_assembled
      call_imported:
	cmp	byte [esi],':'
	je	invalid_use_of_symbol
	cmp	[jump_type],2
	je	invalid_operand
	cmp	[operand_size],0
	je	call_imported_32bit
	cmp	[operand_size],4
	jne	invalid_use_of_symbol
      call_imported_32bit:
	sub	eax,[current_offset]
	add	eax,[org_start]
	sub	eax,5
	mov	edx,eax
	mov	ecx,edi
	call	operand_32bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	al,0E8h
	stosb
	mov	eax,edx
	call	mark_import_relocation
	stosd
	jmp	line_assembled
jmp_instruction:
	push	edi
	mov	edi,jump_operators
	call	get_operator
	pop	edi
	mov	[jump_type],al
	call	get_memory_address
	jc	jmp_reg
	mov	[base_code],FFh
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
	mov	[postbyte_register],101b
	call	store_instruction
	jmp	line_assembled
      jmp_mem_32bit:
	cmp	[jump_type],2
	je	jmp_mem_far_32bit
	cmp	[jump_type],1
	je	jmp_mem_near_32bit
	cmp	[code_type],16
	je	jmp_mem_far_32bit
      jmp_mem_near_32bit:
	call	operand_32bit_prefix
	mov	[postbyte_register],100b
	call	store_instruction
	jmp	line_assembled
      jmp_mem_far_32bit:
	call	operand_16bit_prefix
	mov	[postbyte_register],101b
	call	store_instruction
	jmp	line_assembled
      jmp_mem_16bit:
	cmp	[jump_type],2
	je	invalid_operand_size
	call	operand_16bit_prefix
	mov	[postbyte_register],100b
	call	store_instruction
	jmp	line_assembled
      jmp_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	jmp_imm
	mov	bl,al
	and	bl,111b
	or	bl,11000000b
	shr	al,4
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
	or	bl,100b shl 3
	mov	ah,bl
	mov	al,FFh
	stosw
	jmp	line_assembled
      jmp_reg_far32bit:
	call	operand_32bit_prefix
	or	bl,101b shl 3
	mov	ah,bl
	mov	al,FFh
	stosw
	jmp	line_assembled
      jmp_reg_16bit:
	cmp	[jump_type],2
	je	invalid_operand_size
	call	operand_16bit_prefix
	or	bl,100b shl 3
	mov	ah,bl
	mov	al,FFh
	stosw
	jmp	line_assembled
      jmp_imm:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	push	esi
	push	eax
	call	get_dword_value
	pop	ebx
	mov	[operand_size],bl
	pop	ebx
	cmp	ch,2
	je	jmp_imported
	cmp	byte [esi],':'
	je	jmp_far
	cmp	[jump_type],2
	je	invalid_operand
	sub	eax,[current_offset]
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
	cmp	eax,80h
	jb	jmp_8bit
	cmp	eax,-80h
	jae	jmp_8bit
	cmp	[code_type],32
	je	jmp_32bit
      jmp_16bit:
	dec	eax
	mov	edx,eax
	mov	ecx,edi
	call	operand_16bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	al,0E9h
	stosb
	mov	eax,edx
	stosw
	cmp	eax,10000h
	jge	jump_out_of_range
	cmp	eax,-8000h
	jl	jump_out_of_range
	jmp	line_assembled
      jmp_8bit:
	mov	edx,eax
	mov	ah,al
	mov	al,0EBh
	stosw
	cmp	edx,80h
	jge	jump_out_of_range
	cmp	edx,-80h
	jl	jump_out_of_range
	jmp	line_assembled
      jmp_32bit:
	sub	eax,3
	mov	edx,eax
	mov	ecx,edi
	call	operand_32bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	al,0E9h
	stosb
	mov	eax,edx
	stosd
	jmp	line_assembled
      jmp_far:
	cmp	[jump_type],1
	je	invalid_operand
	mov	esi,ebx
	call	get_word_value
	mov	bl,[operand_size]
	cmp	bl,2
	je	jmp_far_size_ok
	or	bl,bl
	jnz	invalid_operand_size
      jmp_far_size_ok:
	inc	esi
	push	eax
	push	edi
	mov	edi,size_operators
	call	get_operator
	mov	[operand_size],al
	pop	edi
	call	get_dword_value
	pop	edx
	mov	ebx,eax
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
	mov	al,0EAh
	stosb
	mov	eax,ebx
	stosw
	mov	eax,edx
	stosw
	jmp	line_assembled
      jmp_far_32bit:
	call	operand_32bit_prefix
	mov	al,0EAh
	stosb
	mov	eax,ebx
	stosd
	mov	eax,edx
	stosw
	jmp	line_assembled
      jmp_imported:
	cmp	byte [esi],':'
	je	invalid_use_of_symbol
	cmp	[jump_type],2
	je	invalid_operand
	cmp	[operand_size],0
	je	jmp_imported_32bit
	cmp	[operand_size],4
	jne	invalid_use_of_symbol
      jmp_imported_32bit:
	sub	eax,[current_offset]
	add	eax,[org_start]
	sub	eax,5
	mov	edx,eax
	mov	ecx,edi
	call	operand_32bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	al,0E9h
	stosb
	mov	eax,edx
	call	mark_import_relocation
	stosd
	jmp	line_assembled
ret_instruction:
	lodsb
	cmp	al,20h
	je	ret_instruction
	dec	esi
	or	al,al
	jz	simple_ret
	cmp	al,';'
	je	simple_ret
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	ret_imm
	cmp	al,2
	je	ret_imm
	jmp	invalid_operand_size
      ret_imm:
	call	get_word_value
	mov	edx,eax
	mov	al,0C2h
	stosb
	mov	eax,edx
	stosw
	jmp	line_assembled
      simple_ret:
	mov	al,0C3h
	stosb
	jmp	line_assembled
retf_instruction:
	lodsb
	cmp	al,20h
	je	retf_instruction
	dec	esi
	or	al,al
	jz	simple_retf
	cmp	al,';'
	je	simple_retf
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	retf_imm
	cmp	al,2
	je	retf_imm
	jmp	invalid_operand_size
      retf_imm:
	call	get_word_value
	mov	edx,eax
	mov	al,0CAh
	stosb
	mov	eax,edx
	stosw
	jmp	line_assembled
      simple_retf:
	mov	al,0CBh
	stosb
	jmp	line_assembled
test_instruction:
	mov	[base_code],84h
	call	get_memory_address
	jc	test_reg
	push	eax ebx ecx
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	test_mem_imm
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	test_mem_reg_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      test_mem_reg_size_ok:
	pop	ecx ebx edx
	cmp	ah,1
	je	test_mem_reg_8bit
	cmp	ah,2
	je	test_mem_reg_16bit
	cmp	ah,4
	je	test_mem_reg_32bit
	jmp	invalid_operand_size
      test_mem_reg_8bit:
	call	store_instruction
	jmp	line_assembled
      test_mem_reg_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      test_mem_reg_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      test_mem_imm:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	test_mem_imm_size_ok
	xchg	al,[operand_size]
	or	al,al
	jz	test_mem_imm_size_ok
	cmp	al,[operand_size]
	jne	operand_sizes_do_not_match
      test_mem_imm_size_ok:
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
	pop	ecx ebx edx
	call	store_instruction
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      test_mem_imm_16bit:
	call	get_word_value
	mov	word [value],ax
	mov	[postbyte_register],0
	mov	[base_code],0F7h
	call	operand_16bit_prefix
	pop	ecx ebx edx
	call	store_instruction
	mov	ax,word [value]
	stosw
	jmp	line_assembled
      test_mem_imm_32bit:
	push	[import_value]
	call	get_dword_value
	mov	ebp,[import_value]
	pop	[import_value]
	mov	dword [value],eax
	mov	[postbyte_register],0
	mov	[base_code],0F7h
	call	operand_32bit_prefix
	pop	ecx ebx edx
	call	store_instruction
	mov	eax,dword [value]
	cmp	[value_type],2
	je	test_mem_imported
	cmp	[value_type],0
	jnz	test_mem_imm_32bit_relocation
	stosd
	jmp	line_assembled
      test_mem_imm_32bit_relocation:
	call	mark_relocation
	stosd
	jmp	line_assembled
      test_mem_imported:
	mov	[import_value],ebp
	call	mark_import_relocation
	call	mark_relocation
	stosd
	jmp	line_assembled
      test_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	[postbyte_register],al
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	test_reg_imm
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	mov	bh,bl
	shr	bh,4
	and	bl,111b
	mov	[postbyte_register],bl
	cmp	ah,bh
	jne	operand_sizes_do_not_match
	shl	al,3
	or	bl,al
	or	bl,11000000b
	cmp	ah,1
	je	test_reg_reg_8bit
	cmp	ah,2
	je	test_reg_reg_16bit
	cmp	ah,4
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
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
      test_reg_imm:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	mov	ah,[postbyte_register]
	shr	ah,4
	or	al,al
	jz	test_reg_imm_size_ok
	xchg	al,ah
	cmp	al,ah
	jne	operand_sizes_do_not_match
      test_reg_imm_size_ok:
	mov	al,ah
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
	stosw
	mov	al,dl
	stosb
	jmp	line_assembled
      test_al_imm:
	mov	al,0A8h
	stosb
	mov	al,dl
	stosb
	jmp	line_assembled
      test_reg_imm_16bit:
	call	get_word_value
	mov	edx,eax
	call	operand_16bit_prefix
	mov	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	bl,bl
	jz	test_ax_imm
	or	ah,bl
	mov	al,0F7h
	stosw
	mov	eax,edx
	stosw
	jmp	line_assembled
      test_ax_imm:
	mov	al,0A9h
	stosb
	mov	ax,dx
	stosw
	jmp	line_assembled
      test_reg_imm_32bit:
	call	get_dword_value
	mov	edx,eax
	call	operand_32bit_prefix
	mov	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	ch,ch
	jnz	test_reg_imm_32bit_relocation
	or	bl,bl
	jz	test_eax_imm
	or	ah,bl
	mov	al,0F7h
	stosw
	mov	eax,edx
	stosd
	jmp	line_assembled
      test_reg_imm_32bit_relocation:
	or	bl,bl
	jz	test_eax_imm_relocation
	or	ah,bl
	mov	al,0F7h
	stosw
	mov	eax,edx
	cmp	ch,2
	je	test_reg_imported
	call	mark_relocation
	stosd
	jmp	line_assembled
      test_reg_imported:
	call	mark_import_relocation
	call	mark_relocation
	stosd
	jmp	line_assembled
      test_eax_imm:
	mov	al,0A9h
	stosb
	mov	eax,edx
	stosd
	jmp	line_assembled
      test_eax_imm_relocation:
	mov	al,0A9h
	stosb
	mov	eax,edx
	cmp	ch,2
	je	test_eax_imported
	call	mark_relocation
	stosd
	jmp	line_assembled
      test_eax_imported:
	call	mark_import_relocation
	call	mark_relocation
	stosd
	jmp	line_assembled
xchg_instruction:
	mov	[base_code],86h
	call	get_memory_address
	jc	xchg_reg
	push	eax ebx ecx
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	xchg_mem_reg_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      xchg_mem_reg_size_ok:
	pop	ecx ebx edx
	cmp	ah,1
	je	xchg_mem_reg_8bit
	cmp	ah,2
	je	xchg_mem_reg_16bit
	cmp	ah,4
	je	xchg_mem_reg_32bit
	jmp	invalid_operand_size
      xchg_mem_reg_8bit:
	call	store_instruction
	jmp	line_assembled
      xchg_mem_reg_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      xchg_mem_reg_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      xchg_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	[postbyte_register],al
	call	find_next_operand
	call	get_memory_address
	jc	xchg_reg_reg
	mov	edx,eax
	mov	al,[postbyte_register]
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	xchg_reg_mem_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      xchg_reg_mem_size_ok:
	cmp	ah,1
	je	xchg_reg_mem_8bit
	cmp	ah,2
	je	xchg_reg_mem_16bit
	cmp	ah,4
	je	xchg_reg_mem_32bit
	jmp	invalid_operand_size
      xchg_reg_mem_8bit:
	call	store_instruction
	jmp	line_assembled
      xchg_reg_mem_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      xchg_reg_mem_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      xchg_reg_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	mov	bh,bl
	shr	bh,4
	and	bl,111b
	mov	[postbyte_register],bl
	cmp	ah,bh
	jne	operand_sizes_do_not_match
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	bh,al
	cmp	ah,1
	je	xchg_reg_reg_8bit
	cmp	ah,2
	je	xchg_reg_reg_16bit
	cmp	ah,4
	je	xchg_reg_reg_32bit
	jmp	invalid_operand_size
      xchg_reg_reg_32bit:
	call	operand_32bit_prefix
	or	bh,bh
	jz	xchg_ax_reg
	xchg	bh,[postbyte_register]
	or	bh,bh
	jz	xchg_ax_reg
	inc	[base_code]
	jmp	basic_reg_reg_8bit
      xchg_reg_reg_16bit:
	call	operand_16bit_prefix
	or	bh,bh
	jz	xchg_ax_reg
	xchg	bh,[postbyte_register]
	or	bh,bh
	jz	xchg_ax_reg
	inc	[base_code]
      xchg_reg_reg_8bit:
	mov	al,[base_code]
	mov	ah,bl
	stosw
	jmp	line_assembled
      xchg_ax_reg:
	mov	al,90h
	add	al,[postbyte_register]
	stosb
	jmp	line_assembled
mov_instruction:
	mov	[base_code],88h
	call	get_memory_address
	jc	mov_reg
	push	eax ebx ecx
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	mov_mem_sreg
	pop	edi
	pop	ecx ebx edx
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	mov_mem_reg_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      mov_mem_reg_size_ok:
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
	jmp	line_assembled
      mov_mem_al:
	or	ch,ch
	jnz	mov_mem_al_relocation
	cmp	edx,8000h
	jb	mov_mem_address16_al
	cmp	edx,-8000h
	jae	mov_mem_address16_al
	call	address_32bit_prefix
	call	store_segment_prefix
	mov	al,0A2h
	stosb
	mov	eax,edx
	stosd
	jmp	line_assembled
      mov_mem_al_relocation:
	call	address_32bit_prefix
	call	store_segment_prefix
	mov	al,0A2h
	stosb
	mov	eax,edx
	cmp	ch,2
	je	mov_mem_al_imported
	call	mark_relocation
	stosd
	jmp	line_assembled
      mov_mem_al_imported:
	call	mark_import_relocation
	call	mark_relocation
	stosd
	jmp	line_assembled
      mov_mem_address16_al:
	call	address_16bit_prefix
	call	store_segment_prefix
	mov	al,0A2h
	stosb
	mov	eax,edx
	stosw
	jmp	line_assembled
      mov_mem_reg_16bit:
	or	al,bl
	or	al,bh
	jz	mov_mem_ax
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      mov_mem_ax:
	call	operand_16bit_prefix
	or	ch,ch
	jnz	mov_mem_ax_relocation
	cmp	edx,8000h
	jb	mov_mem_address16_ax
	cmp	edx,-8000h
	jae	mov_mem_address16_ax
	call	address_32bit_prefix
	call	store_segment_prefix
	mov	al,0A3h
	stosb
	mov	eax,edx
	stosd
	jmp	line_assembled
      mov_mem_ax_relocation:
	call	address_32bit_prefix
	call	store_segment_prefix
	mov	al,0A3h
	stosb
	mov	eax,edx
	cmp	ch,2
	je	mov_mem_ax_imported
	call	mark_relocation
	stosd
	jmp	line_assembled
      mov_mem_ax_imported:
	call	mark_import_relocation
	call	mark_relocation
	stosd
	jmp	line_assembled
      mov_mem_address16_ax:
	call	address_16bit_prefix
	call	store_segment_prefix
	mov	al,0A3h
	stosb
	mov	eax,edx
	stosw
	jmp	line_assembled
      mov_mem_reg_32bit:
	or	al,bl
	or	al,bh
	jz	mov_mem_eax
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	line_assembled
      mov_mem_eax:
	call	operand_32bit_prefix
	or	ch,ch
	jnz	mov_mem_ax_relocation
	cmp	edx,8000h
	jb	mov_mem_address16_eax
	cmp	edx,-8000h
	jae	mov_mem_address16_eax
	call	address_32bit_prefix
	call	store_segment_prefix
	mov	al,0A3h
	stosb
	mov	eax,edx
	stosd
	jmp	line_assembled
      mov_mem_address16_eax:
	call	address_16bit_prefix
	call	store_segment_prefix
	mov	al,0A3h
	stosb
	mov	eax,edx
	stosw
	jmp	line_assembled
      mov_mem_sreg:
	mov	edi,segment_registers
	call	get_register
	pop	edi
	jc	mov_mem_imm
	pop	ecx ebx edx
	dec	al
	mov	[postbyte_register],al
	mov	ah,[operand_size]
	or	ah,ah
	jz	mov_mem_sreg_size_ok
	cmp	ah,2
	jne	invalid_operand_size
      mov_mem_sreg_size_ok:
	call	operand_16bit_prefix
	mov	[base_code],8Ch
	call	store_instruction
	jmp	line_assembled
      mov_mem_imm:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	mov_mem_imm_size_ok
	xchg	al,[operand_size]
	or	al,al
	jz	mov_mem_imm_size_ok
	cmp	al,[operand_size]
	jne	operand_sizes_do_not_match
      mov_mem_imm_size_ok:
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
	pop	ecx ebx edx
	call	store_instruction
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      mov_mem_imm_16bit:
	call	get_word_value
	mov	word [value],ax
	mov	[postbyte_register],0
	mov	[base_code],0C7h
	call	operand_16bit_prefix
	pop	ecx ebx edx
	call	store_instruction
	mov	ax,word [value]
	stosw
	jmp	line_assembled
      mov_mem_imm_32bit:
	push	[import_value]
	call	get_dword_value
	mov	ebp,[import_value]
	pop	[import_value]
	mov	dword [value],eax
	mov	[postbyte_register],0
	mov	[base_code],0C7h
	call	operand_32bit_prefix
	pop	ecx ebx edx
	call	store_instruction
	mov	eax,dword [value]
	cmp	[value_type],0
	je	mov_mem_imm_32bit_store
	cmp	ch,2
	je	mov_mem_imported
	call	mark_relocation
      mov_mem_imm_32bit_store:
	stosd
	jmp	line_assembled
      mov_mem_imported:
	mov	[import_value],ebp
	call	mark_import_relocation
	call	mark_relocation
	stosd
	jmp	line_assembled
      mov_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	mov_sreg
	mov	[postbyte_register],al
	call	find_next_operand
	call	get_memory_address
	jc	mov_reg_reg
	mov	edx,eax
	mov	al,[postbyte_register]
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	mov_reg_mem_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      mov_reg_mem_size_ok:
	cmp	ah,1
	je	mov_reg_mem_8bit
	cmp	ah,2
	je	mov_reg_mem_16bit
	cmp	ah,4
	je	mov_reg_mem_32bit
	jmp	invalid_operand_size
      mov_reg_mem_8bit:
	add	[base_code],2
	call	store_instruction
	jmp	line_assembled
      mov_reg_mem_16bit:
	call	operand_16bit_prefix
	add	[base_code],3
	call	store_instruction
	jmp	line_assembled
      mov_reg_mem_32bit:
	call	operand_32bit_prefix
	add	[base_code],3
	call	store_instruction
	jmp	line_assembled
      mov_reg_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	mov_reg_sreg
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	mov	bh,bl
	shr	bh,4
	and	bl,111b
	mov	[postbyte_register],bl
	cmp	ah,bh
	jne	operand_sizes_do_not_match
	shl	al,3
	or	bl,al
	or	bl,11000000b
	cmp	ah,1
	je	mov_reg_reg_8bit
	cmp	ah,2
	je	mov_reg_reg_16bit
	cmp	ah,4
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
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
      mov_reg_sreg:
	push	edi
	mov	edi,segment_registers
	call	get_register
	pop	edi
	jc	mov_reg_creg
	dec	al
	mov	bl,[postbyte_register]
	mov	bh,bl
	shr	bh,4
	and	bl,111b
	shl	al,3
	or	bl,al
	or	bl,11000000b
	cmp	bh,2
	jne	invalid_operand_size
	mov	al,8Ch
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
      mov_reg_creg:
	push	edi
	mov	edi,control_registers
	call	get_register
	pop	edi
	jc	mov_reg_dreg
	mov	bl,[postbyte_register]
	mov	bh,bl
	shr	bh,4
	and	bl,111b
	shl	al,3
	or	bl,al
	or	bl,11000000b
	cmp	bh,4
	jne	invalid_operand_size
	mov	ax,200Fh
	stosw
	mov	al,bl
	stosb
	jmp	line_assembled
      mov_reg_dreg:
	push	edi
	mov	edi,debug_registers
	call	get_register
	pop	edi
	jc	mov_reg_imm
	mov	bl,[postbyte_register]
	mov	bh,bl
	shr	bh,4
	and	bl,111b
	shl	al,3
	or	bl,al
	or	bl,11000000b
	cmp	bh,4
	jne	invalid_operand_size
	mov	ax,210Fh
	stosw
	mov	al,bl
	stosb
	jmp	line_assembled
      mov_reg_imm:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	mov	ah,[postbyte_register]
	shr	ah,4
	or	al,al
	jz	mov_reg_imm_size_ok
	xchg	al,ah
	cmp	al,ah
	jne	operand_sizes_do_not_match
      mov_reg_imm_size_ok:
	mov	al,ah
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
	stosw
	jmp	line_assembled
      mov_reg_imm_16bit:
	call	get_word_value
	mov	edx,eax
	call	operand_16bit_prefix
	mov	al,[postbyte_register]
	and	al,111b
	add	al,0B8h
	stosb
	mov	eax,edx
	stosw
	jmp	line_assembled
      mov_reg_imm_32bit:
	call	get_dword_value
	mov	edx,eax
	call	operand_32bit_prefix
	mov	al,[postbyte_register]
	and	al,111b
	add	al,0B8h
	stosb
	mov	eax,edx
	or	ch,ch
	jz	mov_reg_imm_32bit_store
	cmp	ch,2
	je	mov_reg_imported
	call	mark_relocation
      mov_reg_imm_32bit_store:
	stosd
	jmp	line_assembled
      mov_reg_imported:
	call	mark_import_relocation
	call	mark_relocation
	stosd
	jmp	line_assembled
      mov_sreg:
	push	edi
	mov	edi,segment_registers
	call	get_register
	jc	mov_creg
	pop	edi
	dec	al
	cmp	al,1
	je	illegal_instruction
	mov	[postbyte_register],al
	call	find_next_operand
	call	get_memory_address
	jc	mov_sreg_reg
	mov	edx,eax
	mov	ah,[operand_size]
	or	ah,ah
	jz	mov_sreg_mem_size_ok
	cmp	ah,2
	jne	invalid_operand_size
      mov_sreg_mem_size_ok:
	mov	[base_code],8Eh
	call	store_instruction
	jmp	line_assembled
      mov_sreg_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
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
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
      mov_creg:
	mov	edi,control_registers
	call	get_register
	jc	mov_dreg
	mov	[postbyte_register],al
	call	find_next_operand
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
	or	ah,ah
	jz	mov_creg_reg_size_ok
	cmp	ah,4
	jne	invalid_operand_size
      mov_creg_reg_size_ok:
	mov	bl,11000000b
	or	bl,al
	mov	al,[postbyte_register]
	shl	al,3
	or	bl,al
	mov	ax,220Fh
	stosw
	mov	al,bl
	stosb
	jmp	line_assembled
      mov_dreg:
	mov	edi,debug_registers
	call	get_register
	jc	invalid_operand
	mov	[postbyte_register],al
	call	find_next_operand
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
	or	ah,ah
	jz	mov_dreg_reg_size_ok
	cmp	ah,4
	jne	invalid_operand_size
      mov_dreg_reg_size_ok:
	mov	bl,11000000b
	or	bl,al
	mov	al,[postbyte_register]
	shl	al,3
	or	bl,al
	mov	ax,230Fh
	stosw
	mov	al,bl
	stosb
	jmp	line_assembled
lea_instruction:
	mov	[base_code],8Dh
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	[postbyte_register],al
	call	find_next_operand
	call	get_memory_address
	jc	invalid_operand
	mov	edx,eax
	mov	al,[postbyte_register]
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	ah,2
	je	lea_16bit
	cmp	ah,4
	je	lea_32bit
	jmp	invalid_operand_size
      lea_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	line_assembled
      lea_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	line_assembled
ls_instruction:
	or	al,al
	jz	les_instruction
	cmp	al,3
	jz	lds_instruction
	add	al,B0h
	mov	[extended_code],al
	mov	[base_code],0Fh
	jmp	ls_code_ok
      les_instruction:
	mov	[base_code],C4h
	jmp	ls_code_ok
      lds_instruction:
	mov	[base_code],C5h
      ls_code_ok:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	[postbyte_register],al
	call	find_next_operand
	call	get_memory_address
	jc	invalid_operand
	mov	edx,eax
	mov	al,[postbyte_register]
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	ah,2
	je	ls_16bit
	cmp	ah,4
	je	ls_32bit
	jmp	invalid_operand_size
      ls_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	cmp	[operand_size],0
	je	line_assembled
	cmp	[operand_size],4
	jne	invalid_operand_size
	jmp	line_assembled
      ls_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	cmp	[operand_size],0
	je	line_assembled
	cmp	[operand_size],6
	jne	invalid_operand_size
	jmp	line_assembled
enter_instruction:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	cmp	al,2
	je	enter_imm16_size_ok
	or	al,al
	jnz	invalid_operand_size
      enter_imm16_size_ok:
	call	get_word_value
	push	eax
	call	find_next_operand
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	cmp	al,1
	je	enter_imm8_size_ok
	or	al,al
	jnz	invalid_operand_size
      enter_imm8_size_ok:
	call	get_byte_value
	mov	dl,al
	pop	ebx
	mov	al,0C8h
	stosb
	mov	eax,ebx
	stosw
	mov	al,dl
	stosb
	jmp	line_assembled
sh_instruction:
	mov	[postbyte_register],al
	call	get_memory_address
	jc	sh_reg
	push	eax ebx ecx
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	sh_mem_imm
	pop	edi
	cmp	al,11h
	jne	invalid_operand
	mov	ah,[operand_size]
	pop	ecx ebx edx
	cmp	ah,1
	je	sh_mem_cl_8bit
	cmp	ah,2
	je	sh_mem_cl_16bit
	cmp	ah,4
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
	jmp	line_assembled
      sh_mem_cl_16bit:
	mov	[base_code],0D3h
	call	operand_16bit_prefix
	call	store_instruction
	jmp	line_assembled
      sh_mem_cl_32bit:
	mov	[base_code],0D3h
	call	operand_32bit_prefix
	call	store_instruction
	jmp	line_assembled
      sh_mem_imm:
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	sh_mem_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      sh_mem_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	mov	ah,[operand_size]
	pop	ecx ebx edx
	cmp	ah,1
	je	sh_mem_imm_8bit
	cmp	ah,2
	je	sh_mem_imm_16bit
	cmp	ah,4
	je	sh_mem_imm_32bit
	or	ah,ah
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
	stosb
	jmp	line_assembled
      sh_mem_1_8bit:
	mov	[base_code],0D0h
	call	store_instruction
	jmp	line_assembled
      sh_mem_imm_16bit:
	cmp	byte [value],1
	je	sh_mem_1_16bit
	mov	[base_code],0C1h
	call	operand_16bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      sh_mem_1_16bit:
	mov	[base_code],0D1h
	call	operand_16bit_prefix
	call	store_instruction
	jmp	line_assembled
      sh_mem_imm_32bit:
	cmp	byte [value],1
	je	sh_mem_1_32bit
	mov	[base_code],0C1h
	call	operand_32bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      sh_mem_1_32bit:
	mov	[base_code],0D1h
	call	operand_32bit_prefix
	call	store_instruction
	jmp	line_assembled
      sh_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	mov	[operand_size],al
	call	find_next_operand
	mov	edi,general_registers
	call	get_register
	jc	sh_reg_imm
	pop	edi
	cmp	al,11h
	jne	invalid_operand
	mov	al,[operand_size]
	mov	ah,al
	and	al,111b
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	shr	ah,4
	cmp	ah,1
	je	sh_reg_cl_8bit
	cmp	ah,2
	je	sh_reg_cl_16bit
	cmp	ah,4
	je	sh_reg_cl_32bit
	jmp	invalid_operand_size
      sh_reg_cl_8bit:
	mov	al,0D2h
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
      sh_reg_cl_16bit:
	call	operand_16bit_prefix
	mov	al,0D3h
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
      sh_reg_cl_32bit:
	call	operand_32bit_prefix
	mov	al,0D3h
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
      sh_reg_imm:
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	sh_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      sh_reg_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	mov	al,[operand_size]
	mov	ah,al
	and	al,111b
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	shr	ah,4
	cmp	ah,1
	je	sh_reg_imm_8bit
	cmp	ah,2
	je	sh_reg_imm_16bit
	cmp	ah,4
	je	sh_reg_imm_32bit
	jmp	invalid_operand_size
      sh_reg_imm_8bit:
	cmp	byte [value],1
	je	sh_reg_1_8bit
	mov	al,0C0h
	stosb
	mov	al,bl
	mov	ah,byte [value]
	stosw
	jmp	line_assembled
      sh_reg_1_8bit:
	mov	al,0D0h
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
      sh_reg_imm_16bit:
	cmp	byte [value],1
	je	sh_reg_1_16bit
	call	operand_16bit_prefix
	mov	al,0C1h
	stosb
	mov	al,bl
	mov	ah,byte [value]
	stosw
	jmp	line_assembled
      sh_reg_1_16bit:
	call	operand_16bit_prefix
	mov	al,0D1h
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
      sh_reg_imm_32bit:
	cmp	byte [value],1
	je	sh_reg_1_32bit
	call	operand_32bit_prefix
	mov	al,0C1h
	stosb
	mov	al,bl
	mov	ah,byte [value]
	stosw
	jmp	line_assembled
      sh_reg_1_32bit:
	call	operand_32bit_prefix
	mov	al,0D1h
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
in_instruction:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	ah,al
	shr	ah,4
	mov	[operand_size],ah
	and	al,111b
	jnz	invalid_operand
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	in_imm
	cmp	al,22h
	jne	invalid_operand
	pop	edi
	mov	al,[operand_size]
	cmp	al,1
	je	in_al_dx
	cmp	al,2
	je	in_ax_dx
	cmp	al,4
	je	in_eax_dx
	jmp	invalid_operand_size
      in_al_dx:
	mov	al,0ECh
	stosb
	jmp	line_assembled
      in_ax_dx:
	call	operand_16bit_prefix
	mov	al,0EDh
	stosb
	jmp	line_assembled
      in_eax_dx:
	call	operand_32bit_prefix
	mov	al,0EDh
	stosb
	jmp	line_assembled
      in_imm:
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	in_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      in_imm_size_ok:
	call	get_byte_value
	mov	dl,al
	mov	al,[operand_size]
	cmp	al,1
	je	in_al_imm
	cmp	al,2
	je	in_ax_imm
	cmp	al,4
	je	in_eax_imm
	jmp	invalid_operand_size
      in_al_imm:
	mov	al,0E4h
	stosb
	mov	al,dl
	stosb
	jmp	line_assembled
      in_ax_imm:
	call	operand_16bit_prefix
	mov	al,0E5h
	stosb
	mov	al,dl
	stosb
	jmp	line_assembled
      in_eax_imm:
	call	operand_32bit_prefix
	mov	al,0E5h
	stosb
	mov	al,dl
	stosb
	jmp	line_assembled
out_instruction:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	out_imm
	cmp	al,22h
	jne	invalid_operand
	call	find_next_operand
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	ah,al
	shr	ah,4
	and	al,111b
	jnz	invalid_operand
	cmp	ah,1
	je	out_dx_al
	cmp	ah,2
	je	out_dx_ax
	cmp	ah,4
	je	out_dx_eax
	jmp	invalid_operand_size
      out_dx_al:
	mov	al,0EEh
	stosb
	jmp	line_assembled
      out_dx_ax:
	call	operand_16bit_prefix
	mov	al,0EFh
	stosb
	jmp	line_assembled
      out_dx_eax:
	call	operand_32bit_prefix
	mov	al,0EFh
	stosb
	jmp	line_assembled
      out_imm:
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	out_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      out_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	ah,al
	shr	ah,4
	and	al,111b
	jnz	invalid_operand
	cmp	ah,1
	je	out_imm_al
	cmp	ah,2
	je	out_imm_ax
	cmp	ah,4
	je	out_imm_eax
	jmp	invalid_operand_size
      out_imm_al:
	mov	al,0E6h
	stosb
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      out_imm_ax:
	call	operand_16bit_prefix
	mov	al,0E7h
	stosb
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      out_imm_eax:
	call	operand_32bit_prefix
	mov	al,E7h
	stosb
	mov	al,byte [value]
	stosb
	jmp	line_assembled
movx_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	ah,2
	je	movx_reg_16bit
	cmp	ah,4
	je	movx_reg_32bit
	jmp	invalid_operand_size
      movx_reg_16bit:
	call	find_next_operand
	call	get_memory_address
	jc	movzx_reg_16bit_reg
	mov	edx,eax
	mov	al,[operand_size]
	cmp	al,1
	je	movx_reg_16bit_mem_8bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      movx_reg_16bit_mem_8bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	line_assembled
      movzx_reg_16bit_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	cmp	ah,1
	je	movx_reg_16bit_reg_8bit
	jmp	invalid_operand_size
      movx_reg_16bit_reg_8bit:
	call	operand_16bit_prefix
	mov	al,0Fh
	stosb
	mov	al,[extended_code]
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
      movx_reg_32bit:
	call	find_next_operand
	call	get_memory_address
	jc	movzx_reg_32bit_reg
	mov	edx,eax
	mov	al,[operand_size]
	cmp	al,1
	je	movx_reg_32bit_mem_8bit
	cmp	al,2
	je	movx_reg_32bit_mem_16bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      movx_reg_32bit_mem_8bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	line_assembled
      movx_reg_32bit_mem_16bit:
	inc	[extended_code]
	call	operand_32bit_prefix
	call	store_instruction
	jmp	line_assembled
      movzx_reg_32bit_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	cmp	ah,1
	je	movx_reg_32bit_reg_8bit
	cmp	ah,2
	je	movx_reg_32bit_reg_16bit
	jmp	invalid_operand_size
      movx_reg_32bit_reg_8bit:
	call	operand_32bit_prefix
	mov	al,0Fh
	stosb
	mov	al,[extended_code]
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
      movx_reg_32bit_reg_16bit:
	call	operand_32bit_prefix
	mov	al,0Fh
	stosb
	mov	al,[extended_code]
	inc	al
	stosb
	mov	al,bl
	stosb
	jmp	line_assembled
stos_instruction:
	mov	[base_code],al
	call	get_memory_address
	jc	invalid_operand
	or	eax,eax
	jnz	invalid_address
	or	bl,bl
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
lods_instruction:
	call	get_memory_address
	jc	invalid_operand
	or	eax,eax
	jnz	invalid_address
	or	bl,bl
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
	call	store_segment_prefix
	mov	al,ACh
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
xlat_instruction:
	call	get_memory_address
	jc	xlatb_instruction
	or	eax,eax
	jnz	invalid_address
	or	bl,bl
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
	mov	al,D7h
	cmp	[operand_size],1
	jbe	simple_instruction
	jmp	invalid_operand_size
      xlatb_instruction:
	mov	al,D7h
	jmp	simple_instruction
ins_instruction:
	call	get_memory_address
	jc	invalid_operand
	or	eax,eax
	jnz	invalid_address
	or	bl,bl
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
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
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
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	cmp	al,22h
	jne	invalid_operand
	call	find_next_operand
	call	get_memory_address
	jc	invalid_operand
	or	eax,eax
	jnz	invalid_address
	or	bl,bl
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
	call	store_segment_prefix
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
	call	get_memory_address
	jc	invalid_operand
	or	eax,eax
	jnz	invalid_address
	or	bl,bl
	jnz	invalid_address
	cmp	[segment_register],1
	ja	invalid_address
	mov	bl,[operand_size]
	push	ebx
	call	find_next_operand
	call	get_memory_address
	pop	edx
	or	eax,eax
	jnz	invalid_address
	or	bl,bl
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
	mov	bl,dl
	or	bl,[operand_size]
	jz	operand_size_not_specified
	or	dl,dl
	jz	movs_size_ok
	cmp	[operand_size],0
	je	movs_size_ok
	cmp	dl,[operand_size]
	jne	operand_sizes_do_not_match
      movs_size_ok:
	cmp	ah,2
	je	movs_16bit
	cmp	ah,4
	jne	invalid_address
	call	address_32bit_prefix
	jmp	movs_store
      movs_16bit:
	call	address_16bit_prefix
      movs_store:
	call	store_segment_prefix
	mov	al,A4h
	cmp	bl,1
	je	simple_instruction
	inc	al
	cmp	bl,2
	je	simple_instruction_16bit
	cmp	bl,4
	je	simple_instruction_32bit
	jmp	invalid_operand_size
cmps_instruction:
	call	get_memory_address
	jc	invalid_operand
	or	eax,eax
	jnz	invalid_address
	or	bl,bl
	jnz	invalid_address
	mov	al,[segment_register]
	mov	bl,[operand_size]
	push	eax ebx
	call	find_next_operand
	call	get_memory_address
	or	eax,eax
	jnz	invalid_address
	cmp	[segment_register],1
	ja	invalid_address
	pop	edx eax
	mov	[segment_register],al
	or	bl,bl
	jnz	invalid_address
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
	mov	bl,dl
	or	bl,[operand_size]
	jz	operand_size_not_specified
	or	dl,dl
	jz	cmps_size_ok
	cmp	[operand_size],0
	je	cmps_size_ok
	cmp	dl,[operand_size]
	jne	operand_sizes_do_not_match
      cmps_size_ok:
	cmp	ah,2
	je	cmps_16bit
	cmp	ah,4
	jne	invalid_address
	call	address_32bit_prefix
	jmp	cmps_store
      cmps_16bit:
	call	address_16bit_prefix
      cmps_store:
	call	store_segment_prefix
	mov	al,A6h
	cmp	bl,1
	je	simple_instruction
	inc	al
	cmp	bl,2
	je	simple_instruction_16bit
	cmp	bl,4
	je	simple_instruction_32bit
	jmp	invalid_operand_size
set_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	call	get_memory_address
	jc	set_reg
	mov	edx,eax
	cmp	[operand_size],1
	ja	invalid_operand_size
	mov	[postbyte_register],0
	call	store_instruction
	jmp	line_assembled
      set_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	bl,al
	mov	bh,al
	shr	bh,4
	and	bl,111b
	cmp	bh,1
	jne	invalid_operand_size
	mov	ah,[extended_code]
	mov	al,0Fh
	stosw
	mov	al,11000000b
	or	al,bl
	stosb
	jmp	line_assembled
shd_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	call	get_memory_address
	jc	shd_reg
	push	eax ebx ecx
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	shd_mem_reg_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      shd_mem_reg_size_ok:
	mov	[operand_size],ah
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	shd_mem_reg_imm
	cmp	al,11h
	jne	invalid_operand
	pop	edi
	pop	ecx ebx edx
	cmp	[operand_size],2
	je	shd_mem_reg_cl_16bit
	cmp	[operand_size],4
	je	shd_mem_reg_cl_32bit
	jmp	invalid_operand_size
      shd_mem_reg_cl_16bit:
	call	operand_16bit_prefix
	inc	[extended_code]
	call	store_instruction
	jmp	line_assembled
      shd_mem_reg_cl_32bit:
	call	operand_32bit_prefix
	inc	[extended_code]
	call	store_instruction
	jmp	line_assembled
      shd_mem_reg_imm:
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	shd_mem_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      shd_mem_reg_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	pop	ecx ebx edx
	cmp	[operand_size],2
	je	shd_mem_reg_imm_16bit
	cmp	[operand_size],4
	je	shd_mem_reg_imm_32bit
	jmp	invalid_operand_size
      shd_mem_reg_imm_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      shd_mem_reg_imm_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      shd_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	mov	[postbyte_register],al
	call	find_next_operand
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	mov	bh,bl
	shr	bh,4
	and	bl,111b
	mov	[postbyte_register],bl
	cmp	ah,bh
	jne	operand_sizes_do_not_match
	shl	al,3
	or	bl,al
	or	bl,11000000b
	push	eax ebx
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	shd_reg_reg_imm
	cmp	al,11h
	jne	invalid_operand
	pop	edi
	pop	ebx eax
	cmp	ah,2
	je	shd_reg_reg_cl_16bit
	cmp	ah,4
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
	stosw
	mov	al,bl
	stosb
	jmp	line_assembled
      shd_reg_reg_imm:
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	shd_reg_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      shd_reg_reg_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	pop	ebx eax
	cmp	ah,2
	je	shd_reg_reg_imm_16bit
	cmp	ah,4
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
	stosw
	mov	al,bl
	stosb
	mov	al,byte [value]
	stosb
	jmp	line_assembled
bt_instruction:
	mov	[postbyte_register],al
	shl	al,3
	add	al,83h
	mov	[extended_code],al
	mov	[base_code],0Fh
	call	get_memory_address
	jc	bt_reg
	push	eax ebx ecx
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	bt_mem_imm
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	bt_mem_reg_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      bt_mem_reg_size_ok:
	pop	ecx ebx edx
	cmp	ah,2
	je	bt_mem_reg_16bit
	cmp	ah,4
	je	bt_mem_reg_32bit
	jmp	invalid_operand_size
      bt_mem_reg_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	line_assembled
      bt_mem_reg_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	line_assembled
      bt_mem_imm:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	bt_mem_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      bt_mem_imm_size_ok:
	mov	[extended_code],BAh
	call	get_byte_value
	mov	byte [value],al
	mov	al,[operand_size]
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
	pop	ecx ebx edx
	call	store_instruction
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      bt_mem_imm_32bit:
	call	operand_32bit_prefix
	pop	ecx ebx edx
	call	store_instruction
	mov	al,byte [value]
	stosb
	jmp	line_assembled
      bt_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	mov	[postbyte_register],al
	call	find_next_operand
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	bt_reg_imm
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	mov	bh,bl
	shr	bh,4
	and	bl,111b
	mov	[postbyte_register],bl
	cmp	ah,bh
	jne	operand_sizes_do_not_match
	shl	al,3
	or	bl,al
	or	bl,11000000b
	cmp	ah,2
	je	bt_reg_reg_16bit
	cmp	ah,4
	je	bt_reg_reg_32bit
	jmp	invalid_operand_size
      bt_reg_reg_16bit:
	call	operand_16bit_prefix
	mov	ah,[extended_code]
	mov	al,0Fh
	stosw
	mov	al,bl
	stosb
	jmp	line_assembled
      bt_reg_reg_32bit:
	call	operand_32bit_prefix
	mov	ah,[extended_code]
	mov	al,0Fh
	stosw
	mov	al,bl
	stosb
	jmp	line_assembled
      bt_reg_imm:
	push	edi
	mov	edi,size_operators
	call	get_operator
	pop	edi
	or	al,al
	jz	bt_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      bt_reg_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	mov	al,[postbyte_register]
	shr	al,4
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
	mov	ax,BA0Fh
	stosw
	mov	al,11000000b
	or	al,[postbyte_register]
	mov	ah,[extended_code]
	sub	ah,83h
	or	al,ah
	stosb
	mov	al,byte [value]
	stosb
	jmp	line_assembled
bs_instruction:
	mov	[extended_code],al
	mov	[base_code],0Fh
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	[postbyte_register],al
	call	find_next_operand
	call	get_memory_address
	jc	bs_reg_reg
	mov	edx,eax
	mov	al,[postbyte_register]
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	bs_reg_mem_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      bs_reg_mem_size_ok:
	cmp	ah,2
	je	bs_reg_mem_16bit
	cmp	ah,4
	je	bs_reg_mem_32bit
	jmp	invalid_operand_size
      bs_reg_mem_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	line_assembled
      bs_reg_mem_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	line_assembled
      bs_reg_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	mov	bh,bl
	shr	bh,4
	and	bl,111b
	mov	[postbyte_register],bl
	cmp	ah,bh
	jne	operand_sizes_do_not_match
	shl	al,3
	or	bl,al
	or	bl,11000000b
	cmp	ah,2
	je	bs_reg_reg_16bit
	cmp	ah,4
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
	stosw
	mov	al,bl
	stosb
	jmp	line_assembled
pm_word_instruction:
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[base_code],0Fh
	mov	[extended_code],ah
	mov	[postbyte_register],al
	call	get_memory_address
	jc	pm_reg
	mov	edx,eax
	mov	al,[operand_size]
	cmp	al,2
	je	pm_word_instruction_store
	or	al,al
	jnz	invalid_operand_size
      pm_word_instruction_store:
	call	store_instruction
	jmp	line_assembled
      pm_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	bl,al
	mov	bh,al
	and	bl,111b
	shr	bh,4
	cmp	bh,2
	jne	invalid_operand_size
	mov	al,0Fh
	mov	ah,[extended_code]
	stosw
	mov	al,[postbyte_register]
	shl	al,3
	or	al,bl
	or	al,11000000b
	stosb
	jmp	line_assembled
pm_pword_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],1
	mov	[postbyte_register],al
	call	get_memory_address
	jc	invalid_operand
	mov	edx,eax
	mov	al,[operand_size]
	cmp	al,6
	je	pm_pword_instruction_store
	or	al,al
	jnz	invalid_operand_size
      pm_pword_instruction_store:
	call	store_instruction
	jmp	line_assembled
lar_instruction:
	mov	[extended_code],al
	mov	[base_code],0Fh
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	[postbyte_register],al
	call	find_next_operand
	call	get_memory_address
	jc	lar_reg_reg
	mov	edx,eax
	mov	al,[postbyte_register]
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	lar_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      lar_size_ok:
	cmp	ah,2
	je	lar_16bit
	cmp	ah,4
	je	lar_32bit
	jmp	invalid_operand_size
      lar_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	line_assembled
      lar_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	line_assembled
      lar_reg_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	mov	bh,bl
	shr	bh,4
	and	bl,111b
	mov	[postbyte_register],bl
	cmp	ah,bh
	jne	operand_sizes_do_not_match
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	cmp	ah,2
	je	lar_reg_reg_16bit
	cmp	ah,4
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
	stosw
	mov	al,bl
	stosb
	jmp	line_assembled
invlpg_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],1
	mov	[postbyte_register],7
	call	get_memory_address
	jc	invalid_operand
	mov	edx,eax
	call	store_instruction
	jmp	line_assembled
basic_486_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	call	get_memory_address
	jc	basic_486_reg
	push	eax ebx ecx
	call	find_next_operand
	push	edi
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	basic_486_mem_reg_size_ok
	cmp	ah,[operand_size]
	jne	operand_sizes_do_not_match
      basic_486_mem_reg_size_ok:
	pop	ecx ebx edx
	cmp	ah,1
	je	basic_486_mem_reg_8bit
	cmp	ah,2
	je	basic_486_mem_reg_16bit
	cmp	ah,4
	je	basic_486_mem_reg_32bit
	jmp	invalid_operand_size
      basic_486_mem_reg_8bit:
	call	store_instruction
	jmp	line_assembled
      basic_486_mem_reg_16bit:
	call	operand_16bit_prefix
	inc	[extended_code]
	call	store_instruction
	jmp	line_assembled
      basic_486_mem_reg_32bit:
	call	operand_32bit_prefix
	inc	[extended_code]
	call	store_instruction
	jmp	line_assembled
      basic_486_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	mov	[postbyte_register],al
	call	find_next_operand
	mov	edi,general_registers
	call	get_register
	pop	edi
	jc	invalid_operand
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	bl,[postbyte_register]
	mov	bh,bl
	shr	bh,4
	and	bl,111b
	mov	[postbyte_register],bl
	cmp	ah,bh
	jne	operand_sizes_do_not_match
	shl	al,3
	or	bl,al
	or	bl,11000000b
	cmp	ah,1
	je	basic_486_reg_reg_8bit
	cmp	ah,2
	je	basic_486_reg_reg_16bit
	cmp	ah,4
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
	stosw
	mov	al,bl
	stosb
	jmp	line_assembled
bswap_instruction:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	ah,al
	and	ah,111b
	add	ah,C8h
	shr	al,4
	cmp	al,4
	jne	invalid_operand_size
	call	operand_32bit_prefix
	mov	al,0Fh
	stosw
	jmp	line_assembled
cmpxchg8b_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],C7h
	mov	[postbyte_register],1
	call	get_memory_address
	jc	invalid_operand
	mov	edx,eax
	mov	al,[operand_size]
	cmp	al,8
	je	cmpxchg8b_store
	or	al,al
	jnz	invalid_operand_size
      cmpxchg8b_store:
	call	store_instruction
	jmp	line_assembled
basic_fpu_instruction:
	mov	[postbyte_register],al
	call	get_memory_address
	jc	basic_fpu_freg
	mov	edx,eax
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
	mov	[base_code],D8h
	call	store_instruction
	jmp	line_assembled
      basic_fpu_mem_64bit:
	mov	[base_code],DCh
	call	store_instruction
	jmp	line_assembled
      basic_fpu_freg:
	push	edi
	mov	edi,fpu_registers
	call	get_register
	jc	invalid_operand
	pop	edi
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
	call	find_next_operand
	push	edi
	mov	edi,fpu_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	or	al,al
	jnz	invalid_operand
	mov	ah,[postbyte_register]
	or	ah,11000000b
	mov	al,DCh
	stosw
	jmp	line_assembled
      basic_fpu_st0:
	call	find_next_operand
	push	edi
	mov	edi,fpu_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	ah,[postbyte_register]
	shl	ah,3
	or	ah,al
	or	ah,11000000b
	mov	al,D8h
	stosw
	jmp	line_assembled
      basic_fpu_single_freg:
	shl	ah,3
	or	ah,al
	or	ah,11000000b
	mov	al,D8h
	stosw
	jmp	line_assembled
simple_fpu_instruction:
	mov	ah,al
	or	ah,11000000b
	mov	al,D9h
	stosw
	jmp	line_assembled
fi_instruction:
	mov	[postbyte_register],al
	call	get_memory_address
	jc	invalid_operand
	mov	edx,eax
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
	mov	[base_code],DAh
	call	store_instruction
	jmp	line_assembled
      fi_mem_16bit:
	mov	[base_code],DEh
	call	store_instruction
	jmp	line_assembled
fld_instruction:
	mov	[postbyte_register],al
	call	get_memory_address
	jc	fld_freg
	mov	edx,eax
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
	mov	[base_code],D9h
	call	store_instruction
	jmp	line_assembled
      fld_mem_64bit:
	mov	[base_code],DDh
	call	store_instruction
	jmp	line_assembled
      fld_mem_80bit:
	mov	al,[postbyte_register]
	cmp	al,0
	je	fld_mem_80bit_store
	dec	[postbyte_register]
	cmp	al,3
	je	fld_mem_80bit_store
	jmp	invalid_operand_size
      fld_mem_80bit_store:
	add	[postbyte_register],5
	mov	[base_code],DBh
	call	store_instruction
	jmp	line_assembled
      fld_freg:
	push	edi
	mov	edi,fpu_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	ah,[postbyte_register]
	shl	ah,3
	or	ah,al
	or	ah,11000000b
	cmp	[postbyte_register],2
	jae	fst_freg
	mov	al,D9h
	stosw
	jmp	line_assembled
      fst_freg:
	mov	al,DDh
	stosw
	jmp	line_assembled
fild_instruction:
	mov	[postbyte_register],al
	call	get_memory_address
	jc	invalid_operand
	mov	edx,eax
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
	mov	[base_code],DBh
	call	store_instruction
	jmp	line_assembled
      fild_mem_16bit:
	mov	[base_code],DFh
	call	store_instruction
	jmp	line_assembled
      fild_mem_64bit:
	mov	al,[postbyte_register]
	cmp	al,0
	je	fild_mem_64bit_store
	dec	[postbyte_register]
	cmp	al,3
	je	fild_mem_64bit_store
	jmp	invalid_operand_size
      fild_mem_64bit_store:
	add	[postbyte_register],5
	mov	[base_code],DFh
	call	store_instruction
	jmp	line_assembled
fbld_instruction:
	mov	[postbyte_register],al
	call	get_memory_address
	jc	invalid_operand
	mov	edx,eax
	mov	al,[operand_size]
	or	al,al
	jz	fbld_mem_80bit
	cmp	al,10
	je	fbld_mem_80bit
	jmp	invalid_operand_size
      fbld_mem_80bit:
	mov	[base_code],DFh
	call	store_instruction
	jmp	line_assembled
faddp_instruction:
	mov	[postbyte_register],al
	push	edi
	mov	edi,fpu_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	ah,[postbyte_register]
	shl	ah,3
	or	al,ah
	mov	[postbyte_register],al
	call	find_next_operand
	push	edi
	mov	edi,fpu_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	or	al,al
	jnz	invalid_operand
	mov	ah,[postbyte_register]
	or	ah,11000000b
	mov	al,DEh
	stosw
	jmp	line_assembled
fcompp_instruction:
	mov	ax,D9DEh
	stosw
	jmp	line_assembled
fxch_instruction:
	push	edi
	mov	edi,fpu_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	ah,1
	shl	ah,3
	or	ah,al
	or	ah,11000000b
	mov	al,D9h
	stosw
	jmp	line_assembled
ffree_instruction:
	mov	[postbyte_register],al
	push	edi
	mov	edi,fpu_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	mov	ah,[postbyte_register]
	shl	ah,3
	or	ah,al
	or	ah,11000000b
	mov	al,DDh
	stosw
	jmp	line_assembled
fldenv_instruction:
	mov	[postbyte_register],al
	call	get_memory_address
	jc	invalid_operand
	mov	edx,eax
	mov	[base_code],D9h
	call	store_instruction
	jmp	line_assembled
fldcw_instruction:
	mov	[postbyte_register],al
	call	get_memory_address
	jc	invalid_operand
	mov	edx,eax
	mov	al,[operand_size]
	or	al,al
	jz	fldcw_mem_16bit
	cmp	al,2
	je	fldcw_mem_16bit
	jmp	invalid_operand_size
      fldcw_mem_16bit:
	mov	[base_code],D9h
	call	store_instruction
	jmp	line_assembled
fsave_instruction:
	mov	[postbyte_register],al
	call	get_memory_address
	jc	invalid_operand
	mov	edx,eax
	mov	[base_code],DDh
	call	store_instruction
	jmp	line_assembled
fstsw_instruction:
	mov	[postbyte_register],7
	call	get_memory_address
	jc	fstsw_reg
	mov	edx,eax
	mov	al,[operand_size]
	or	al,al
	jz	fstsw_mem_16bit
	cmp	al,2
	je	fstsw_mem_16bit
	jmp	invalid_operand_size
      fstsw_mem_16bit:
	mov	[base_code],DDh
	call	store_instruction
	jmp	line_assembled
      fstsw_reg:
	push	edi
	mov	edi,general_registers
	call	get_register
	jc	invalid_operand
	pop	edi
	cmp	al,20h
	jne	invalid_operand
	mov	ax,E0DFh
	stosw
	jmp	line_assembled
finit_instruction:
	mov	ah,al
	mov	al,DBh
	stosw
	jmp	line_assembled

find_next_operand:
	lodsb
	cmp	al,20h
	je	find_next_operand
	cmp	al,','
	jne	invalid_operand
	ret
operand_16bit_prefix:
	cmp	[code_type],16
	je	size_prefix_ok
	mov	al,66h
	stosb
	ret
operand_32bit_prefix:
	cmp	[code_type],32
	je	size_prefix_ok
	mov	al,66h
	stosb
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
	stosb
	jmp	segment_prefix_ok
      segment_prefix_386:
	add	al,64h-5
	stosb
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
	or	ch,ch
	jnz	invalid_use_of_symbol
	or	edx,edx
	jz	address
	cmp	edx,128
	jb	address_8bit_value
	cmp	edx,-128
	jae	address_8bit_value
      address_16bit_value:
	cmp	edx,-8000h
	jl	value_out_of_range
	or	al,10000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stosb
	mov	eax,edx
	stosw
	ret
      address_8bit_value:
	or	al,01000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stosb
	mov	al,dl
	stosb
	ret
      address:
	cmp	al,110b
	je	address_8bit_value
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stosb
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
	or	ch,ch
	jnz	sib_address_32bit_value
	cmp	bh,5
	je	address_value
	or	edx,edx
	jz	sib_address
      address_value:
	cmp	edx,128
	jb	sib_address_8bit_value
	cmp	edx,-128
	jae	sib_address_8bit_value
      sib_address_32bit_value:
	or	al,10000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stosw
	or	ch,ch
	jz	store_sib_address_32bit_value
	cmp	ch,2
	je	sib_address_imported_value
	call	mark_relocation
	jmp	store_sib_address_32bit_value
      sib_address_imported_value:
	call	mark_import_relocation
	call	mark_relocation
      store_sib_address_32bit_value:
	mov	eax,edx
	stosd
	ret
      sib_address_8bit_value:
	or	al,01000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stosw
	mov	al,dl
	stosb
	ret
      sib_address:
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stosw
	ret
      only_index_register:
	or	ah,101b
	and	bl,111b
	shl	bl,3
	or	ah,bl
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stosw
	or	ch,ch
	jz	store_address_32bit_value
	cmp	ch,2
	je	address_imported_value
	call	mark_relocation
	jmp	store_address_32bit_value
      address_imported_value:
	call	mark_import_relocation
	call	mark_relocation
      store_address_32bit_value:
	mov	eax,edx
	stosd
	ret
      zero_index_register:
	mov	bl,4
	mov	cl,1
	jmp	base_and_index
      only_base_register:
	mov	al,bh
	and	al,111b
	cmp	al,4
	je	zero_index_register
	or	ch,ch
	jnz	simple_address_32bit_value
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
	stosb
	or	ch,ch
	jz	store_simple_address_32bit_value
	cmp	ch,2
	je	simple_address_imported_value
	call	mark_relocation
	jmp	store_simple_address_32bit_value
      simple_address_imported_value:
	call	mark_import_relocation
	call	mark_relocation
      store_simple_address_32bit_value:
	mov	eax,edx
	stosd
	ret
      simple_address_8bit_value:
	or	al,01000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stosb
	mov	al,dl
	stosb
	ret
      simple_address:
	cmp	al,5
	je	simple_address_8bit_value
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stosb
	ret
      address_immediate:
	cmp	[code_type],16
	je	addressing_16bit
	or	ch,ch
	jnz	address_immediate_32bit
	cmp	edx,10000h
	jb	address_immediate_16bit
      address_immediate_32bit:
	call	address_32bit_prefix
	call	store_instruction_code
	mov	al,101b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stosb
	or	ch,ch
	jz	store_32bit_address
	cmp	ch,2
	je	address_32bit_imported_value
	call	mark_relocation
	jmp	store_32bit_address
      address_32bit_imported_value:
	call	mark_import_relocation
	call	mark_relocation
      store_32bit_address:
	mov	eax,edx
	stosd
	ret
      addressing_16bit:
	or	ch,ch
	jnz	invalid_use_of_symbol
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
	stosb
	mov	eax,edx
	stosw
	ret
      store_instruction_code:
	mov	al,[base_code]
	stosb
	cmp	al,0Fh
	jne	instruction_code_ok
	mov	al,[extended_code]
	stosb
      instruction_code_ok:
	ret
      address_16bit_prefix:
	cmp	[code_type],16
	je	instruction_prefix_ok
	mov	al,67h
	stosb
	ret
      address_32bit_prefix:
	cmp	[code_type],32
	je	instruction_prefix_ok
	mov	al,67h
	stosb
      instruction_prefix_ok:
	ret
mark_relocation:
	push	eax ebx
	mov	eax,edi
	sub	eax,[org_start]
	mov	ebx,[relocations_end]
	mov	[ebx],eax
	add	ebx,4
	cmp	ebx,[relocations_memory_end]
	jae	out_of_memory
	mov	[relocations_end],ebx
	pop	ebx eax
	ret
mark_import_relocation:
	push	eax ebx
	mov	eax,edi
	sub	eax,[org_start]
	mov	ebx,[import_relocations_end]
	mov	[ebx],eax
	add	ebx,4
	mov	eax,[import_value]
	mov	[ebx],eax
	add	ebx,4
	cmp	ebx,[import_relocations_memory_end]
	jae	out_of_memory
	mov	[import_relocations_end],ebx
	pop	ebx eax
	ret

default_code_type db 16
code_type db 0
org_start dd 0
virtual_mode db 0
stored_org dd 0
stored_offset dd 0
entry_point dd 0
entry_code_type db 0
program_relocatable db 0

current_pass db 0
current_line dd 0
current_line_end dd 0
line_type db 0
home_line dd 0
error_line dd 0
error_home_line dd 0
error dd 0
instruction_end dd 0
current_offset dd 0
next_pass_needed db 0
operand_size db 0
postbyte_register db 0
segment_register db 0
base_code db 0
extended_code db 0
jump_type db 0
undefined_data_start dd 0
undefined_data_end dd 0
import_number dd 0
label temp_byte byte
temp_dword dd 0
