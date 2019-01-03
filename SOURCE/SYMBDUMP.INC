
; flat assembler core
; Copyright (c) 1999-2019, Tomasz Grysztar.
; All rights reserved.

dump_symbols:
	mov	edi,[code_start]
	call	setup_dump_header
	mov	esi,[input_file]
	call	copy_asciiz
	cmp	edi,[tagged_blocks]
	jae	out_of_memory
	mov	eax,edi
	sub	eax,ebx
	mov	[ebx-40h+0Ch],eax
	mov	esi,[output_file]
	call	copy_asciiz
	cmp	edi,[tagged_blocks]
	jae	out_of_memory
	mov	edx,[symbols_stream]
	mov	ebp,[free_additional_memory]
	and	[number_of_sections],0
	cmp	[output_format],4
	je	prepare_strings_table
	cmp	[output_format],5
	jne	strings_table_ready
	bt	[format_flags],0
	jc	strings_table_ready
      prepare_strings_table:
	cmp	edx,ebp
	je	strings_table_ready
	mov	al,[edx]
	test	al,al
	jz	prepare_string
	cmp	al,80h
	je	prepare_string
	add	edx,0Ch
	cmp	al,0C0h
	jb	prepare_strings_table
	add	edx,4
	jmp	prepare_strings_table
      prepare_string:
	mov	esi,edi
	sub	esi,ebx
	xchg	esi,[edx+4]
	test	al,al
	jz	prepare_section_string
	or	dword [edx+4],1 shl 31
	add	edx,0Ch
      prepare_external_string:
	mov	ecx,[esi]
	add	esi,4
	rep	movs byte [edi],[esi]
	mov	byte [edi],0
	inc	edi
	cmp	edi,[tagged_blocks]
	jae	out_of_memory
	jmp	prepare_strings_table
      prepare_section_string:
	mov	ecx,[number_of_sections]
	mov	eax,ecx
	inc	eax
	mov	[number_of_sections],eax
	xchg	eax,[edx+4]
	shl	ecx,2
	add	ecx,[free_additional_memory]
	mov	[ecx],eax
	add	edx,20h
	test	esi,esi
	jz	prepare_default_section_string
	cmp	[output_format],5
	jne	prepare_external_string
	bt	[format_flags],0
	jc	prepare_external_string
	mov	esi,[esi]
	add	esi,[resource_data]
      copy_elf_section_name:
	lods	byte [esi]
	cmp	edi,[tagged_blocks]
	jae	out_of_memory
	stos	byte [edi]
	test	al,al
	jnz	copy_elf_section_name
	jmp	prepare_strings_table
      prepare_default_section_string:
	mov	eax,'.fla'
	stos	dword [edi]
	mov	ax,'t'
	stos	word [edi]
	cmp	edi,[tagged_blocks]
	jae	out_of_memory
	jmp	prepare_strings_table
      strings_table_ready:
	mov	edx,[tagged_blocks]
	mov	ebp,[memory_end]
	sub	ebp,[labels_list]
	add	ebp,edx
      prepare_labels_dump:
	cmp	edx,ebp
	je	labels_dump_ok
	mov	eax,[edx+24]
	test	eax,eax
	jz	label_dump_name_ok
	cmp	eax,[memory_start]
	jb	label_name_outside_source
	cmp	eax,[source_start]
	ja	label_name_outside_source
	sub	eax,[memory_start]
	dec	eax
	mov	[edx+24],eax
	jmp	label_dump_name_ok
      label_name_outside_source:
	mov	esi,eax
	mov	eax,edi
	sub	eax,ebx
	or	eax,1 shl 31
	mov	[edx+24],eax
	movzx	ecx,byte [esi-1]
	lea	eax,[edi+ecx+1]
	cmp	edi,[tagged_blocks]
	jae	out_of_memory
	rep	movsb
	xor	al,al
	stosb
      label_dump_name_ok:
	mov	eax,[edx+28]
	test	eax,eax
	jz	label_dump_line_ok
	sub	eax,[memory_start]
	mov	[edx+28],eax
      label_dump_line_ok:
	test	byte [edx+9],4
	jz	convert_base_symbol_for_label
	xor	eax,eax
	mov	[edx],eax
	mov	[edx+4],eax
	jmp	base_symbol_for_label_ok
      convert_base_symbol_for_label:
	mov	eax,[edx+20]
	test	eax,eax
	jz	base_symbol_for_label_ok
	cmp	eax,[symbols_stream]
	mov	eax,[eax+4]
	jae	base_symbol_for_label_ok
	xor	eax,eax
      base_symbol_for_label_ok:
	mov	[edx+20],eax
	mov	ax,[current_pass]
	cmp	ax,[edx+16]
	je	label_defined_flag_ok
	and	byte [edx+8],not 1
      label_defined_flag_ok:
	cmp	ax,[edx+18]
	je	label_used_flag_ok
	and	byte [edx+8],not 8
      label_used_flag_ok:
	add	edx,LABEL_STRUCTURE_SIZE
	jmp	prepare_labels_dump
      labels_dump_ok:
	mov	eax,edi
	sub	eax,ebx
	mov	[ebx-40h+14h],eax
	add	eax,40h
	mov	[ebx-40h+18h],eax
	mov	ecx,[memory_end]
	sub	ecx,[labels_list]
	mov	[ebx-40h+1Ch],ecx
	add	eax,ecx
	mov	[ebx-40h+20h],eax
	mov	ecx,[source_start]
	sub	ecx,[memory_start]
	mov	[ebx-40h+24h],ecx
	add	eax,ecx
	mov	[ebx-40h+28h],eax
	mov	eax,[number_of_sections]
	shl	eax,2
	mov	[ebx-40h+34h],eax
	call	prepare_preprocessed_source
	mov	esi,[labels_list]
	mov	ebp,edi
      make_lines_dump:
	cmp	esi,[tagged_blocks]
	je	lines_dump_ok
	mov	eax,[esi-4]
	mov	ecx,[esi-8]
	sub	esi,8
	sub	esi,ecx
	cmp	eax,1
	je	process_line_dump
	cmp	eax,2
	jne	make_lines_dump
	add	dword [ebx-40h+3Ch],8
	jmp	make_lines_dump
      process_line_dump:
	push	ebx
	mov	ebx,[esi+8]
	mov	eax,[esi+4]
	sub	eax,[code_start]
	add	eax,[headers_size]
	test	byte [ebx+0Ah],1
	jz	store_offset
	xor	eax,eax
      store_offset:
	stos	dword [edi]
	mov	eax,[esi]
	sub	eax,[memory_start]
	stos	dword [edi]
	mov	eax,[esi+4]
	xor	edx,edx
	xor	cl,cl
	sub	eax,[ebx]
	sbb	edx,[ebx+4]
	sbb	cl,[ebx+8]
	stos	dword [edi]
	mov	eax,edx
	stos	dword [edi]
	mov	eax,[ebx+10h]
	stos	dword [edi]
	mov	eax,[ebx+14h]
	test	eax,eax
	jz	base_symbol_for_line_ok
	cmp	eax,[symbols_stream]
	mov	eax,[eax+4]
	jae	base_symbol_for_line_ok
	xor	eax,eax
      base_symbol_for_line_ok:
	stos	dword [edi]
	mov	al,[ebx+9]
	stos	byte [edi]
	mov	al,[esi+10h]
	stos	byte [edi]
	mov	al,[ebx+0Ah]
	and	al,1
	stos	byte [edi]
	mov	al,cl
	stos	byte [edi]
	pop	ebx
	cmp	edi,[tagged_blocks]
	jae	out_of_memory
	mov	eax,edi
	sub	eax,1Ch
	sub	eax,ebp
	mov	[esi],eax
	jmp	make_lines_dump
      lines_dump_ok:
	mov	edx,edi
	mov	eax,[current_offset]
	sub	eax,[code_start]
	add	eax,[headers_size]
	stos	dword [edi]
	mov	ecx,edi
	sub	ecx,ebx
	sub	ecx,[ebx-40h+14h]
	mov	[ebx-40h+2Ch],ecx
	add	ecx,[ebx-40h+28h]
	mov	[ebx-40h+30h],ecx
	add	ecx,[ebx-40h+34h]
	mov	[ebx-40h+38h],ecx
      find_inexisting_offsets:
	sub	edx,1Ch
	cmp	edx,ebp
	jb	write_symbols
	test	byte [edx+1Ah],1
	jnz	find_inexisting_offsets
	cmp	eax,[edx]
	jb	correct_inexisting_offset
	mov	eax,[edx]
	jmp	find_inexisting_offsets
      correct_inexisting_offset:
	and	dword [edx],0
	or	byte [edx+1Ah],2
	jmp	find_inexisting_offsets
      write_symbols:
	mov	edx,[symbols_file]
	call	create
	jc	write_failed
	mov	edx,[code_start]
	mov	ecx,[edx+14h]
	add	ecx,40h
	call	write
	jc	write_failed
	mov	edx,[tagged_blocks]
	mov	ecx,[memory_end]
	sub	ecx,[labels_list]
	call	write
	jc	write_failed
	mov	edx,[memory_start]
	mov	ecx,[source_start]
	sub	ecx,edx
	call	write
	jc	write_failed
	mov	edx,ebp
	mov	ecx,edi
	sub	ecx,edx
	call	write
	jc	write_failed
	mov	edx,[free_additional_memory]
	mov	ecx,[number_of_sections]
	shl	ecx,2
	call	write
	jc	write_failed
	mov	esi,[labels_list]
	mov	edi,[memory_start]
      make_references_dump:
	cmp	esi,[tagged_blocks]
	je	references_dump_ok
	mov	eax,[esi-4]
	mov	ecx,[esi-8]
	sub	esi,8
	sub	esi,ecx
	cmp	eax,2
	je	dump_reference
	cmp	eax,1
	jne	make_references_dump
	mov	edx,[esi]
	jmp	make_references_dump
      dump_reference:
	mov	eax,[memory_end]
	sub	eax,[esi]
	sub	eax,LABEL_STRUCTURE_SIZE
	stosd
	mov	eax,edx
	stosd
	cmp	edi,[tagged_blocks]
	jb	make_references_dump
	jmp	out_of_memory
      references_dump_ok:
	mov	edx,[memory_start]
	mov	ecx,edi
	sub	ecx,edx
	call	write
	jc	write_failed
	call	close
	ret
      setup_dump_header:
	xor	eax,eax
	mov	ecx,40h shr 2
	rep	stos dword [edi]
	mov	ebx,edi
	mov	dword [ebx-40h],'fas'+1Ah shl 24
	mov	dword [ebx-40h+4],VERSION_MAJOR + VERSION_MINOR shl 8 + 40h shl 16
	mov	dword [ebx-40h+10h],40h
	ret
prepare_preprocessed_source:
	mov	esi,[memory_start]
	mov	ebp,[source_start]
	test	ebp,ebp
	jnz	prepare_preprocessed_line
	mov	ebp,[current_line]
	inc	ebp
      prepare_preprocessed_line:
	cmp	esi,ebp
	jae	preprocessed_source_ok
	mov	eax,[memory_start]
	mov	edx,[input_file]
	cmp	[esi],edx
	jne	line_not_from_main_input
	mov	[esi],eax
      line_not_from_main_input:
	sub	[esi],eax
	test	byte [esi+7],1 shl 7
	jz	prepare_next_preprocessed_line
	sub	[esi+8],eax
	sub	[esi+12],eax
      prepare_next_preprocessed_line:
	call	skip_preprocessed_line
	jmp	prepare_preprocessed_line
      preprocessed_source_ok:
	ret
      skip_preprocessed_line:
	add	esi,16
      skip_preprocessed_line_content:
	lods	byte [esi]
	cmp	al,1Ah
	je	skip_preprocessed_symbol
	cmp	al,3Bh
	je	skip_preprocessed_symbol
	cmp	al,22h
	je	skip_preprocessed_string
	or	al,al
	jnz	skip_preprocessed_line_content
	ret
      skip_preprocessed_string:
	lods	dword [esi]
	add	esi,eax
	jmp	skip_preprocessed_line_content
      skip_preprocessed_symbol:
	lods	byte [esi]
	movzx	eax,al
	add	esi,eax
	jmp	skip_preprocessed_line_content
restore_preprocessed_source:
	mov	esi,[memory_start]
	mov	ebp,[source_start]
	test	ebp,ebp
	jnz	restore_preprocessed_line
	mov	ebp,[current_line]
	inc	ebp
      restore_preprocessed_line:
	cmp	esi,ebp
	jae	preprocessed_source_restored
	mov	eax,[memory_start]
	add	[esi],eax
	cmp	[esi],eax
	jne	preprocessed_line_source_restored
	mov	edx,[input_file]
	mov	[esi],edx
      preprocessed_line_source_restored:
	test	byte [esi+7],1 shl 7
	jz	restore_next_preprocessed_line
	add	[esi+8],eax
	add	[esi+12],eax
      restore_next_preprocessed_line:
	call	skip_preprocessed_line
	jmp	restore_preprocessed_line
      preprocessed_source_restored:
	ret
dump_preprocessed_source:
	mov	edi,[free_additional_memory]
	call	setup_dump_header
	mov	esi,[input_file]
	call	copy_asciiz
	cmp	edi,[additional_memory_end]
	jae	out_of_memory
	mov	eax,edi
	sub	eax,ebx
	dec	eax
	mov	[ebx-40h+0Ch],eax
	mov	eax,edi
	sub	eax,ebx
	mov	[ebx-40h+14h],eax
	add	eax,40h
	mov	[ebx-40h+20h],eax
	call	prepare_preprocessed_source
	sub	esi,[memory_start]
	mov	[ebx-40h+24h],esi
	mov	edx,[symbols_file]
	call	create
	jc	write_failed
	mov	edx,[free_additional_memory]
	mov	ecx,[edx+14h]
	add	ecx,40h
	call	write
	jc	write_failed
	mov	edx,[memory_start]
	mov	ecx,esi
	call	write
	jc	write_failed
	call	close
	ret