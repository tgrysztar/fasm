
; flat assembler source
; Copyright (c) 1999-2002, Tomasz Grysztar.
; All rights reserved.

formatter:
	cmp	[output_format],4
	je	coff_formatter
	mov	eax,edi
	sub	eax,[code_start]
	mov	[real_code_size],eax
	cmp	edi,[undefined_data_end]
	jne	calculate_code_size
	mov	edi,[undefined_data_start]
      calculate_code_size:
	sub	edi,[code_start]
	mov	[code_size],edi
	mov	[written_size],0
	mov	edx,[output_file]
	call	create
	jc	write_failed
	cmp	[output_format],2
	jne	write_code
	call	write_mz_header
      write_code:
	mov	edx,[code_start]
	mov	ecx,[code_size]
	add	[written_size],ecx
	call	write
	jc	write_failed
	call	close
	ret
format_directive:
	cmp	edi,[real_code_start]
	jne	unexpected_instruction
	cmp	[output_format],0
	jne	unexpected_instruction
	lods	byte [esi]
	cmp	al,18h
	jne	invalid_argument
	lods	byte [esi]
	test	al,0F0h
	jnz	complex_format
      select_format:
	cmp	[current_pass],0
	je	format_ok
	cmp	al,[selected_format]
	jne	unexpected_instruction
      format_ok:
	mov	[output_format],al
	cmp	al,2
	je	format_mz
	cmp	al,3
	je	format_pe
	cmp	al,4
	je	format_coff
	jmp	instruction_assembled
      complex_format:
	mov	bl,al
	shr	bl,4
	and	eax,0Fh
	or	[format_flags],eax
	lods	byte [esi]
	cmp	al,18h
	jne	invalid_argument
	lods	byte [esi]
	cmp	bl,al
	jne	invalid_argument
	jmp	select_format
entry_directive:
	bts	[format_flags],1
	jc	symbol_already_defined
	mov	al,[output_format]
	cmp	al,2
	je	mz_entry
	cmp	al,3
	je	pe_entry
	jmp	illegal_instruction
stack_directive:
	bts	[format_flags],2
	jc	symbol_already_defined
	mov	al,[output_format]
	cmp	al,2
	je	mz_stack
	cmp	al,3
	je	pe_stack
	jmp	illegal_instruction
heap_directive:
	bts	[format_flags],3
	jc	symbol_already_defined
	mov	al,[output_format]
	cmp	al,2
	je	mz_heap
	cmp	al,3
	je	pe_heap
	jmp	illegal_instruction
section_directive:
	cmp	[virtual_data],0
	jne	illegal_instruction
	mov	al,[output_format]
	cmp	al,3
	je	pe_section
	cmp	al,4
	je	coff_section
	jmp	illegal_instruction
mark_relocation:
	cmp	[value_type],0
	je	relocation_ok
	cmp	[virtual_data],0
	jne	relocation_ok
	cmp	[output_format],2
	je	mark_mz_relocation
	cmp	[output_format],3
	je	mark_pe_relocation
	cmp	[output_format],4
	je	mark_coff_relocation
      relocation_ok:
	ret
close_pass:
	mov	al,[output_format]
	cmp	al,3
	je	close_pe
	cmp	al,4
	je	close_coff
	ret

format_mz:
	mov	edx,[additional_memory]
	mov	[header_data],edx
	push	edi
	mov	edi,edx
	mov	ecx,1Ch shr 2
	xor	eax,eax
	rep	stos dword [edi]
	mov	[additional_memory],edi
	pop	edi
	mov	word [edx+0Ch],0FFFFh
	mov	word [edx+10h],1000h
	mov	[code_type],16
	jmp	instruction_assembled
mark_mz_relocation:
	push	eax ebx
	inc	[number_of_relocations]
	mov	ebx,[additional_memory]
	mov	eax,edi
	sub	eax,[code_start]
	mov	[ebx],ax
	shr	eax,16
	shl	ax,12
	mov	[ebx+2],ax
	cmp	word [ebx],0FFFFh
	jne	mz_relocation_ok
	inc	word [ebx+2]
	sub	word [ebx],10h
      mz_relocation_ok:
	add	ebx,4
	cmp	ebx,[structures_buffer]
	jae	out_of_memory
	mov	[additional_memory],ebx
	pop	ebx eax
	ret
segment_directive:
	cmp	[output_format],2
	jne	illegal_instruction
	cmp	[virtual_data],0
	jne	illegal_instruction
	lods	byte [esi]
	cmp	al,2
	jne	invalid_argument
	lods	dword [esi]
	cmp	eax,0Fh
	jb	invalid_use_of_symbol
	je	reserved_word_used_as_symbol
	inc	esi
	mov	ebx,eax
	mov	eax,edi
	sub	eax,[code_start]
	mov	ecx,0Fh
	add	eax,0Fh
	and	eax,1111b
	sub	ecx,eax
	mov	edx,edi
	xor	al,al
	rep	stos byte [edi]
	mov	[org_origin],edi
	mov	[org_sib],0
	mov	[org_start],edi
	mov	eax,edx
	call	undefined_data
	mov	eax,edi
	sub	eax,[code_start]
	shr	eax,4
	cmp	eax,10000h
	jae	value_out_of_range
	mov	cl,[current_pass]
	test	byte [ebx+8],1
	jz	new_segment
	cmp	cl,[ebx+9]
	je	symbol_already_defined
	mov	[ebx+9],cl
	xchg	[ebx],eax
	xor	edx,edx
	xchg	[ebx+4],edx
	cmp	eax,[ebx]
	jne	changed_segment
	or	edx,edx
	jnz	changed_segment
	jmp	segment_ok
      changed_segment:
	or	[next_pass_needed],-1
	jmp	segment_ok
      new_segment:
	or	byte [ebx+8],1
	mov	[ebx+9],cl
	mov	byte [ebx+10],0
	mov	byte [ebx+11],1
	mov	[ebx],eax
	mov	dword [ebx+4],0
      segment_ok:
	mov	al,16
	cmp	byte [esi],13h
	jne	segment_type_ok
	lods	byte [esi]
	lods	byte [esi]
      segment_type_ok:
	mov	[code_type],al
	jmp	instruction_assembled
mz_entry:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	call	get_word_value
	cmp	[next_pass_needed],0
	je	check_initial_cs
	cmp	[current_pass],0
	je	initial_cs_ok
      check_initial_cs:
	cmp	[value_type],1
	jne	invalid_address
      initial_cs_ok:
	mov	edx,[header_data]
	mov	[edx+16h],ax
	lods	byte [esi]
	cmp	al,':'
	jne	invalid_argument
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	ja	invalid_address
	call	get_word_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	edx,[header_data]
	mov	[edx+14h],ax
	jmp	instruction_assembled
mz_stack:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	call	get_word_value
	cmp	byte [esi],':'
	je	stack_pointer
	cmp	ax,10h
	jb	invalid_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	edx,[header_data]
	mov	[edx+10h],ax
	jmp	instruction_assembled
      stack_pointer:
	cmp	[next_pass_needed],0
	je	check_initial_ss
	cmp	[current_pass],0
	je	initial_ss_ok
      check_initial_ss:
	cmp	[value_type],1
	jne	invalid_address
      initial_ss_ok:
	mov	edx,[header_data]
	mov	[edx+0Eh],ax
	lods	byte [esi]
	cmp	al,':'
	jne	invalid_argument
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	call	get_word_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	edx,[header_data]
	mov	[edx+10h],ax
	bts	[format_flags],4
	jmp	instruction_assembled
mz_heap:
	cmp	[output_format],2
	jne	illegal_instruction
	lods	byte [esi]
	call	get_size_operator
	cmp	ah,1
	je	invalid_value
	cmp	ah,2
	ja	invalid_value
	cmp	al,'('
	jne	invalid_argument
	call	get_word_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	edx,[header_data]
	mov	[edx+0Ch],ax
	jmp	instruction_assembled
write_mz_header:
	mov	edx,[header_data]
	bt	[format_flags],4
	jc	mz_stack_ok
	mov	eax,[real_code_size]
	dec	eax
	shr	eax,4
	inc	eax
	mov	[edx+0Eh],ax
	shl	eax,4
	movzx	ecx,word [edx+10h]
	add	eax,ecx
	mov	[real_code_size],eax
      mz_stack_ok:
	mov	edi,[additional_memory]
	mov	eax,[number_of_relocations]
	shl	eax,2
	add	eax,1Ch
	sub	edi,eax
	xchg	edi,[additional_memory]
	mov	ecx,0Fh
	add	eax,0Fh
	and	eax,1111b
	sub	ecx,eax
	xor	al,al
	rep	stos byte [edi]
	sub	edi,[additional_memory]
	mov	ecx,edi
	shr	edi,4
	mov	word [edx],'MZ' 	; signature
	mov	[edx+8],di		; header size in paragraphs
	mov	eax,[number_of_relocations]
	mov	[edx+6],ax		; number of relocation entries
	mov	eax,[code_size]
	add	eax,ecx
	mov	esi,eax
	shr	esi,9
	and	eax,1FFh
	inc	si
	or	ax,ax
	jnz	mz_size_ok
	mov	ax,200h
	dec	si
      mz_size_ok:
	mov	[edx+2],ax		; number of bytes in last page
	mov	[edx+4],si		; number of pages
	mov	eax,[real_code_size]
	dec	eax
	shr	eax,4
	inc	eax
	mov	esi,[code_size]
	dec	esi
	shr	esi,4
	inc	esi
	sub	eax,esi
	mov	[edx+0Ah],ax		; minimum memory in addition to code
	add	[edx+0Ch],ax		; maximum memory in addition to code
	salc
	mov	ah,al
	or	[edx+0Ch],ax
	mov	word [edx+18h],1Ch	; offset of relocation table
	add	[written_size],ecx
	call	write
	jc	write_failed
	ret

make_stub:
	or	edx,edx
	jnz	stub_from_file
	push	esi
	mov	edx,edi
	xor	eax,eax
	mov	ecx,20h
	rep	stos dword [edi]
	mov	eax,40h+default_stub_end-default_stub
	mov	cx,100h+default_stub_end-default_stub
	mov	word [edx],'MZ'
	mov	word [edx+4],1
	mov	word [edx+2],ax
	mov	word [edx+8],4
	mov	word [edx+0Ah],10h
	mov	word [edx+0Ch],0FFFFh
	mov	word [edx+10h],cx
	mov	word [edx+3Ch],ax
	mov	word [edx+18h],40h
	lea	edi,[edx+40h]
	mov	esi,default_stub
	mov	ecx,default_stub_end-default_stub
	rep	movs byte [edi],[esi]
	pop	esi
	jmp	stub_ok
      default_stub:
	use16
	push	cs
	pop	ds
	mov	dx,stub_message-default_stub
	mov	ah,9
	int	21h
	mov	ax,4C01h
	int	21h
      stub_message db 'this program cannot be run in DOS mode.',0Dh,0Ah,24h
	rq	1
      default_stub_end:
	use32
      stub_from_file:
	call	open
	jc	file_not_found
	mov	edx,edi
	mov	ecx,1Ch
	push	esi
	mov	esi,edx
	call	read
	jc	binary_stub
	cmp	word [esi],'MZ'
	jne	binary_stub
	add	edi,1Ch
	movzx	ecx,word [esi+6]
	dec	ecx
	sar	ecx,3
	inc	ecx
	shl	ecx,2
	add	ecx,(40h-1Ch) shr 2
	lea	eax,[edi+ecx*4]
	cmp	edi,[display_buffer]
	jae	out_of_memory
	xor	eax,eax
	rep	stos dword [edi]
	mov	edx,40h
	xchg	dx,[esi+18h]
	xor	al,al
	call	lseek
	movzx	ecx,word [esi+6]
	shl	ecx,2
	lea	edx,[esi+40h]
	call	read
	mov	edx,edi
	sub	edx,esi
	shr	edx,4
	xchg	dx,[esi+8]
	shl	edx,4
	xor	al,al
	call	lseek
	movzx	ecx,word [esi+4]
	dec	ecx
	shl	ecx,9
	sub	ecx,eax
	movzx	eax,word [esi+2]
	add	ecx,eax
	mov	edx,edi
	push	ecx
	dec	ecx
	shr	ecx,3
	inc	ecx
	shl	ecx,1
	lea	eax,[edi+ecx*4]
	cmp	edi,[display_buffer]
	jae	out_of_memory
	xor	eax,eax
	rep	stos dword [edi]
	pop	ecx
	call	read
	call	close
	mov	edx,edi
	sub	edx,esi
	mov	ax,dx
	and	ax,1FFh
	mov	[esi+2],ax
	dec	edx
	shr	edx,9
	inc	edx
	mov	[esi+4],dx
	mov	eax,edi
	sub	eax,esi
	mov	[esi+3Ch],eax
	pop	esi
      stub_ok:
	ret
      binary_stub:
	mov	esi,edi
	mov	ecx,40h shr 2
	xor	eax,eax
	rep	stos dword [edi]
	mov	al,2
	xor	edx,edx
	call	lseek
	push	eax
	xor	al,al
	xor	edx,edx
	call	lseek
	mov	ecx,[esp]
	add	ecx,40h
	dec	ecx
	shr	ecx,3
	inc	ecx
	shl	ecx,3
	mov	ax,cx
	and	ax,1FFh
	mov	[esi+2],ax
	mov	eax,ecx
	dec	eax
	shr	eax,9
	inc	eax
	mov	[esi+4],ax
	mov	[esi+3Ch],ecx
	sub	ecx,40h
	mov	eax,10000h
	sub	eax,ecx
	jbe	binary_heap_ok
	shr	eax,4
	mov	[esi+0Ah],ax
      binary_heap_ok:
	mov	word [esi],'MZ'
	mov	word [esi+8],4
	mov	ax,0FFFFh
	mov	[esi+0Ch],ax
	dec	ax
	mov	[esi+10h],ax
	sub	ax,0Eh
	mov	[esi+0Eh],ax
	mov	[esi+16h],ax
	mov	word [esi+14h],100h
	mov	word [esi+18h],40h
	mov	eax,[display_buffer]
	sub	eax,ecx
	cmp	edi,eax
	jae	out_of_memory
	mov	edx,edi
	shr	ecx,2
	xor	eax,eax
	rep	stos dword [edi]
	pop	ecx
	call	read
	call	close
	pop	esi
	ret

format_pe:
	mov	[machine],14Ch		; intel 80386
	mov	[subsystem],3		; console subsystem
	mov	[subsystem_version],3 + 10 shl 16
	mov	[image_base],400000h
	xor	edx,edx
      pe_settings:
	cmp	byte [esi],84h
	je	get_stub_name
	cmp	byte [esi],80h
	je	get_pe_base
	cmp	byte [esi],1Bh
	jne	pe_settings_ok
	lods	byte [esi]
	lods	byte [esi]
	test	al,80h+40h
	jz	subsystem_setting
	cmp	al,80h
	jne	pe_settings
	bts	[format_flags],8
	jc	symbol_already_defined
	jmp	pe_settings
      subsystem_setting:
	bts	[format_flags],7
	jc	symbol_already_defined
	and	ax,3Fh
	mov	[subsystem],ax
	cmp	byte [esi],'('
	jne	pe_settings
	inc	esi
	cmp	byte [esi],'.'
	jne	invalid_value
	inc	esi
	push	edx
	call	fp_to_version
	pop	edx
	add	esi,12
	mov	[subsystem_version],eax
	jmp	pe_settings
      get_pe_base:
	bts	[format_flags],9
	jc	symbol_already_defined
	lods	word [esi]
	cmp	ah,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	push	edx edi
	add	edi,[stub_size]
	call	get_dword_value
	pop	edi edx
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	[image_base],eax
	cmp	byte [esi],84h
	jne	pe_settings_ok
      get_stub_name:
	lods	byte [esi]
	lods	word [esi]
	cmp	ax,'('
	jne	invalid_argument
	lods	dword [esi]
	mov	edx,esi
	add	esi,eax
	inc	esi
      pe_settings_ok:
	cmp	[current_pass],0
	jne	pe_stub_ok
	call	make_stub
	mov	[real_code_start],edi
      pe_stub_ok:
	mov	eax,edi
	sub	eax,[code_start]
	mov	[stub_size],eax
	mov	[header_data],edi
	mov	edx,edi
	imul	ecx,[number_of_sections],28h
	add	ecx,18h+0E0h
	add	ecx,eax
	dec	ecx
	shr	ecx,9
	inc	ecx
	shl	ecx,9
	sub	ecx,eax
	mov	eax,[display_buffer]
	sub	eax,ecx
	cmp	edi,eax
	jae	out_of_memory
	shr	ecx,2
	xor	eax,eax
	rep	stos dword [edi]
	mov	word [edx],'PE' 	; signature
	mov	ax,[machine]
	mov	word [edx+4],ax
	mov	dword [edx+14h],0E0h	; size of optional header
	mov	dword [edx+16h],10B818Eh; flags and magic value
	mov	dword [edx+38h],1000h	; section alignment
	mov	dword [edx+3Ch],200h	; file alignment
	mov	word [edx+40h],1	; OS version
	mov	eax,[image_base]
	mov	dword [edx+34h],eax
	mov	ax,[subsystem]
	mov	[edx+5Ch],ax
	mov	eax,[subsystem_version]
	mov	[edx+48h],eax
	mov	word [edx+1Ah],VERSION_MAJOR + VERSION_MINOR shl 8
	mov	eax,edi
	sub	eax,[code_start]
	mov	[edx+54h],eax		; size of headers
	mov	dword [edx+60h],1000h	; stack reserve
	mov	dword [edx+64h],1000h	; stack commit
	mov	dword [edx+68h],10000h	; heap reserve
	mov	dword [edx+6Ch],0	; heap commit
	mov	dword [edx+74h],16	; number of directories
	dec	eax
	shr	eax,12
	inc	eax
	shl	eax,12
	mov	[edx+28h],eax		; entry point rva
	mov	[code_type],32
	or	[reloc_labels],-1
	mov	[number_of_sections],0
	mov	[sections_data],edi
	lea	ebx,[edx+18h+0E0h]
	mov	[current_section],ebx
	mov	dword [ebx],'.fla'
	mov	dword [ebx+4],'t'
	mov	[ebx+14h],edi
	mov	[ebx+0Ch],eax
	mov	dword [ebx+24h],0E0000060h
	neg	eax
	add	eax,edi
	sub	eax,[edx+34h]
	mov	[org_origin],eax
	mov	[org_sib],0
	mov	[org_start],edi
	bt	[format_flags],8
	jnc	instruction_assembled
	or	dword [edx+16h],2000h
	jmp	instruction_assembled
      fp_to_version:
	cmp	byte [esi+11],0
	jne	invalid_value
	cmp	byte [esi+10],2
	ja	invalid_value
	mov	dx,[esi+8]
	cmp	dx,8000h
	je	zero_version
	mov	eax,[esi+4]
	cmp	dx,7
	jg	invalid_value
	mov	cx,7
	sub	cx,dx
	mov	eax,[esi+4]
	shr	eax,cl
	mov	ebx,eax
	shr	ebx,24
	cmp	bl,100
	jae	invalid_value
	and	eax,0FFFFFFh
	mov	ecx,100
	mul	ecx
	shrd	eax,edx,24
	jnc	version_ok
	inc	eax
      version_ok:
	shl	eax,16
	mov	ax,bx
	ret
      zero_version:
	xor	eax,eax
	ret
pe_section:
	call	close_pe_section
	bts	[format_flags],5
	lea	ecx,[ebx+28h]
	cmp	ecx,[sections_data]
	jbe	new_section
	sub	ebx,28h
	or	[next_pass_needed],-1
      new_section:
	mov	[ebx+0Ch],eax
	lods	word [esi]
	cmp	ax,'('
	jne	invalid_argument
	lea	edx,[esi+4]
	mov	ecx,[esi]
	lea	esi,[esi+4+ecx+1]
	cmp	ecx,8
	ja	name_too_long
	xor	eax,eax
	mov	[ebx],eax
	mov	[ebx+4],eax
	push	esi edi
	mov	edi,ebx
	mov	esi,edx
	rep	movs byte [edi],[esi]
	pop	edi esi
	mov	[code_type],32
	mov	dword [ebx+24h],0
	mov	[ebx+14h],edi
	mov	edx,[header_data]
	mov	eax,edi
	sub	eax,[ebx+0Ch]
	sub	eax,[edx+34h]
	mov	[org_origin],eax
	mov	[org_sib],0
	mov	[org_start],edi
	or	[reloc_labels],-1
      get_section_flags:
	lods	byte [esi]
	cmp	al,1Ah
	je	set_directory
	cmp	al,19h
	je	section_flag
	dec	esi
	jmp	instruction_assembled
      set_directory:
	movzx	eax,byte [esi]
	inc	esi
	mov	ecx,ebx
	xchg	ecx,[edx+78h+eax*8]
	or	ecx,ecx
	jnz	symbol_already_defined
	mov	dword [edx+78h+eax*8+4],-1
	cmp	al,5
	jne	get_section_flags
	call	make_pe_fixups
	jmp	get_section_flags
      section_flag:
	lods	byte [esi]
	mov	cl,al
	mov	eax,1
	shl	eax,cl
	or	dword [ebx+24h],eax
	jmp	get_section_flags
      close_pe_section:
	mov	ebx,[current_section]
	mov	eax,edi
	sub	eax,[ebx+14h]
	jnz	finish_section
	bt	[format_flags],5
	jc	finish_section
	mov	eax,[ebx+0Ch]
	ret
      finish_section:
	mov	[ebx+8],eax
	cmp	edi,[undefined_data_end]
	jne	align_section
	mov	edi,[undefined_data_start]
      align_section:
	mov	[undefined_data_end],0
	mov	edx,edi
	sub	edx,[ebx+14h]
	mov	ecx,edx
	dec	ecx
	shr	ecx,9
	inc	ecx
	shl	ecx,9
	mov	[ebx+10h],ecx
	sub	ecx,edx
	xor	al,al
	rep	stos byte [edi]
	mov	eax,[code_start]
	sub	[ebx+14h],eax
	mov	eax,[ebx+8]
	or	eax,eax
	jz	udata_ok
	cmp	dword [ebx+10h],0
	jne	udata_ok
	or	byte [ebx+24h],80h
      udata_ok:
	dec	eax
	shr	eax,12
	inc	eax
	shl	eax,12
	add	eax,[ebx+0Ch]
	add	ebx,28h
	mov	[current_section],ebx
	inc	word [number_of_sections]
	jz	illegal_instruction
	ret
data_directive:
	cmp	[output_format],3
	jne	illegal_instruction
	lods	byte [esi]
	cmp	al,1Ah
	je	predefined_data_type
	cmp	al,'('
	jne	invalid_argument
	call	get_byte_value
	cmp	al,16
	jb	data_type_ok
	jmp	invalid_value
      predefined_data_type:
	movzx	eax,byte [esi]
	inc	esi
      data_type_ok:
	mov	ebx,[current_section]
	mov	ecx,edi
	sub	ecx,[ebx+14h]
	add	ecx,[ebx+0Ch]
	mov	edx,[header_data]
	xchg	ecx,[edx+78h+eax*8]
	or	ecx,ecx
	jnz	symbol_already_defined
	call	allocate_structure_data
	mov	word [ebx],data_directive-assembler
	mov	[ebx+2],al
	cmp	al,5
	jne	instruction_assembled
	call	make_pe_fixups
	jmp	instruction_assembled
      end_data:
	cmp	[output_format],3
	jne	illegal_instruction
	call	find_structure_data
	jc	unexpected_instruction
	movzx	eax,byte [ebx+2]
	mov	edx,[current_section]
	mov	ecx,edi
	sub	ecx,[edx+14h]
	add	ecx,[edx+0Ch]
	mov	edx,[header_data]
	sub	ecx,[edx+78h+eax*8]
	mov	[edx+78h+eax*8+4],ecx
	jmp	remove_structure_data
pe_entry:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[next_pass_needed],0
	je	check_pe_entry
	cmp	[current_pass],0
	je	pe_entry_ok
      check_pe_entry:
	cmp	[value_type],2
	jne	invalid_address
      pe_entry_ok:
	mov	edx,[header_data]
	sub	eax,[edx+34h]
	mov	[edx+28h],eax
	jmp	instruction_assembled
pe_stack:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	edx,[header_data]
	mov	[edx+60h],eax
	cmp	byte [esi],','
	jne	default_stack_commit
	lods	byte [esi]
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	edx,[header_data]
	mov	[edx+64h],eax
	cmp	eax,[edx+60h]
	ja	value_out_of_range
	jmp	instruction_assembled
      default_stack_commit:
	mov	dword [edx+64h],1000h
	mov	eax,[edx+60h]
	cmp	eax,1000h
	ja	instruction_assembled
	mov	dword [edx+64h],eax
	jmp	instruction_assembled
pe_heap:
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	edx,[header_data]
	mov	[edx+68h],eax
	cmp	byte [esi],','
	jne	default_heap_commit
	lods	byte [esi]
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	edx,[header_data]
	mov	[edx+6Ch],eax
	cmp	eax,[edx+68h]
	ja	value_out_of_range
	jmp	instruction_assembled
      default_heap_commit:
	mov	dword [edx+6Ch],0
	jmp	instruction_assembled
mark_pe_relocation:
	push	eax ebx
	mov	ebx,[current_section]
	mov	eax,edi
	sub	eax,[ebx+14h]
	add	eax,[ebx+0Ch]
	mov	ebx,[additional_memory]
	inc	[number_of_relocations]
	jz	invalid_use_of_symbol
	mov	[ebx],eax
	add	ebx,4
	cmp	ebx,[structures_buffer]
	jae	out_of_memory
	mov	[additional_memory],ebx
	pop	ebx eax
	ret
make_pe_fixups:
	push	ebx edx esi
	mov	ecx,[number_of_relocations]
	jecxz	fixups_done
	mov	esi,[additional_memory]
	mov	eax,ecx
	shl	eax,2
	sub	esi,eax
	mov	[additional_memory],esi
	or	[number_of_relocations],-1
	mov	edx,1000h
	mov	ebp,edi
      make_fixups:
	cmp	[esi],edx
	jb	store_fixup
	mov	eax,edi
	sub	eax,ebp
	test	eax,11b
	jz	fixups_block
	xor	ax,ax
	stos	word [edi]
	add	dword [ebx],2
      fixups_block:
	mov	eax,edx
	add	edx,1000h
	cmp	[esi],edx
	jae	fixups_block
	stos	dword [edi]
	mov	ebx,edi
	mov	eax,8
	stos	dword [edi]
      store_fixup:
	add	dword [ebx],2
	mov	eax,[esi]
	and	ax,0FFFh
	or	ax,3000h
	stos	word [edi]
	add	esi,4
	loop	make_fixups
      fixups_done:
	pop	esi edx ebx
	ret
close_pe:
	call	close_pe_section
	mov	edx,[header_data]
	mov	[edx+50h],eax
	call	make_timestamp
	mov	edx,[header_data]
	mov	[edx+8],eax
	mov	eax,[number_of_relocations]
	cmp	eax,0
	jle	pe_flags_ok
	or	word [edx+16h],1
	shl	eax,2
	sub	[additional_memory],eax
      pe_flags_ok:
	mov	eax,[number_of_sections]
	mov	[edx+6],ax
	xor	ecx,ecx
      process_directories:
	mov	eax,[edx+78h+ecx*8]
	or	eax,eax
	jz	directory_ok
	cmp	dword [edx+78h+ecx*8+4],-1
	jne	directory_ok
      section_data:
	mov	ebx,[edx+78h+ecx*8]
	mov	eax,[ebx+0Ch]
	mov	[edx+78h+ecx*8],eax	; directory rva
	mov	eax,[ebx+8]
	mov	[edx+78h+ecx*8+4],eax	; directory size
      directory_ok:
	inc	cl
	cmp	cl,10h
	jb	process_directories
	ret

format_coff:
	mov	eax,[additional_memory]
	mov	[sections_data],eax
	mov	ebx,eax
	add	eax,14h
	cmp	eax,[structures_buffer]
	jae	out_of_memory
	mov	[additional_memory],eax
	xor	eax,eax
	mov	[ebx],eax
	mov	[ebx+4],eax
	mov	[ebx+8],edi
	mov	eax,60h
	bt	[format_flags],0
	jnc	flat_section_flags_ok
	or	eax,0E0000000h
      flat_section_flags_ok:
	mov	dword [ebx+10h],eax
	mov	[current_section],ebx
	mov	[number_of_sections],1
	mov	[code_type],32
	or	[reloc_labels],-1
	jmp	instruction_assembled
coff_section:
	call	close_coff_section
	mov	ebx,[additional_memory]
	lea	eax,[ebx+14h]
	cmp	eax,[structures_buffer]
	jae	out_of_memory
	mov	[additional_memory],eax
	mov	[current_section],ebx
	inc	[number_of_sections]
	xor	eax,eax
	mov	[ebx],al
	mov	[ebx+8],edi
	mov	[org_origin],edi
	mov	[org_sib],0
	mov	[org_start],edi
	or	[reloc_labels],-1
	mov	[ebx+10h],eax
	lods	word [esi]
	cmp	ax,'('
	jne	invalid_argument
	mov	[ebx+4],esi
	mov	ecx,[esi]
	lea	esi,[esi+4+ecx+1]
	cmp	ecx,8
	ja	name_too_long
      coff_section_flags:
	cmp	byte [esi],19h
	jne	instruction_assembled
	inc	esi
	lods	byte [esi]
	bt	[format_flags],0
	jc	coff_section_flag_ok
	cmp	al,7
	ja	invalid_argument
      coff_section_flag_ok:
	mov	cl,al
	mov	eax,1
	shl	eax,cl
	or	dword [ebx+10h],eax
	jmp	coff_section_flags
      close_coff_section:
	mov	ebx,[current_section]
	mov	eax,edi
	mov	edx,[ebx+8]
	sub	eax,edx
	mov	[ebx+0Ch],eax
	xor	eax,eax
	xchg	[undefined_data_end],eax
	cmp	eax,edi
	jne	coff_section_ok
	cmp	edx,[undefined_data_start]
	jne	coff_section_ok
	mov	edi,edx
	or	dword [ebx+10h],80h
      coff_section_ok:
	ret
public_directive:
	cmp	[output_format],4
	jne	illegal_instruction
	lods	word [esi]
	cmp	ax,'('
	jne	invalid_argument
	mov	edx,esi
	lods	dword [esi]
	lea	esi,[esi+eax+1]
	mov	ebx,[additional_memory]
	lea	eax,[ebx+10h]
	cmp	eax,[structures_buffer]
	jae	out_of_memory
	mov	[additional_memory],eax
	mov	byte [ebx],2
	mov	[ebx+4],edx
	lods	byte [esi]
	cmp	al,2
	jne	invalid_argument
	lods	dword [esi]
	cmp	eax,0Fh
	jb	invalid_use_of_symbol
	je	reserved_word_used_as_symbol
	inc	esi
	mov	[ebx+8],eax
	mov	eax,[current_line]
	mov	[ebx+0Ch],eax
	jmp	instruction_assembled
extrn_directive:
	cmp	[output_format],4
	jne	illegal_instruction
	lods	word [esi]
	cmp	ax,'('
	jne	invalid_argument
	mov	ebx,esi
	lods	dword [esi]
	lea	esi,[esi+eax+1]
	mov	edx,[additional_memory]
	lea	eax,[edx+0Ch]
	cmp	eax,[structures_buffer]
	jae	out_of_memory
	mov	[additional_memory],eax
	mov	byte [edx],1
	mov	[edx+4],ebx
	lods	byte [esi]
	cmp	al,2
	jne	invalid_argument
	lods	dword [esi]
	cmp	eax,0Fh
	jb	invalid_use_of_symbol
	je	reserved_word_used_as_symbol
	inc	esi
	mov	ebx,eax
	xor	ah,ah
	lods	byte [esi]
	cmp	al,':'
	je	get_extrn_size
	dec	esi
	cmp	al,11h
	jne	extrn_size_ok
      get_extrn_size:
	lods	word [esi]
	cmp	al,11h
	jne	invalid_argument
      extrn_size_ok:
	mov	[ebx+10],ah
	movzx	eax,ah
	mov	[edx+8],eax
	mov	byte [ebx+11],2
	mov	cl,[current_pass]
	test	byte [ebx+8],1
	jz	new_extrn
	cmp	cl,[ebx+9]
	je	symbol_already_defined
	mov	[ebx+9],cl
	xchg	[ebx+4],edx
	cmp	[current_pass],0
	je	instruction_assembled
	cmp	edx,[ebx+4]
	je	instruction_assembled
	or	[next_pass_needed],-1
	jmp	instruction_assembled
      new_extrn:
	or	byte [ebx+8],1
	mov	[ebx+9],cl
	mov	dword [ebx+4],edx
	jmp	instruction_assembled
mark_coff_relocation:
	cmp	[value_type],3
	je	relative
	push	ebx eax
	mov	al,6
	jmp	coff_relocation
      relative:
	cmp	[reloc_labels],-1
	jne	invalid_use_of_symbol
	push	ebx
	bt	[format_flags],0
	jnc	relative_ok
	mov	ebx,[current_section]
	mov	ebx,[ebx+8]
	sub	ebx,edi
	sub	eax,ebx
	add	eax,4
      relative_ok:
	push	eax
	mov	al,20
      coff_relocation:
	mov	ebx,[additional_memory]
	add	ebx,0Ch
	cmp	ebx,[structures_buffer]
	jae	out_of_memory
	mov	[additional_memory],ebx
	mov	byte [ebx-0Ch],al
	mov	eax,edi
	sub	eax,[org_origin]
	mov	[ebx-0Ch+4],eax
	mov	eax,[symbol_identifier]
	mov	[ebx-0Ch+8],eax
	pop	eax ebx
	ret
close_coff:
	call	close_coff_section
	cmp	[next_pass_needed],0
	je	coff_closed
	mov	eax,[sections_data]
	mov	[additional_memory],eax
      coff_closed:
	ret
coff_formatter:
	sub	edi,[code_start]
	mov	[code_size],edi
	mov	ebx,[sections_data]
	cmp	dword [ebx+0Ch],0
	jne	sections_count_ok
	cmp	[number_of_sections],1
	je	sections_count_ok
	add	ebx,14h
	mov	[sections_data],ebx
	dec	[number_of_sections]
      sections_count_ok:
	mov	edi,[additional_memory]
	mov	ebx,edi
	mov	[header_data],edi
	mov	ecx,28h shr 2
	imul	ecx,[number_of_sections]
	add	ecx,14h shr 2
	lea	eax,[edi+ecx*4]
	cmp	eax,[structures_buffer]
	jae	out_of_memory
	xor	eax,eax
	rep	stos dword [edi]
	mov	[additional_memory],edi
	mov	word [ebx],14Ch
	mov	word [ebx+12h],104h
	bt	[format_flags],0
	jnc	coff_flags_ok
	or	byte [ebx+12h],80h
      coff_flags_ok:
	push	ebx
	call	make_timestamp
	pop	ebx
	mov	[ebx+4],eax
	mov	eax,[number_of_sections]
	mov	[ebx+2],ax
	mov	esi,[sections_data]
	xor	ecx,ecx
      enumerate_symbols:
	cmp	esi,[header_data]
	je	symbols_enumerated
	mov	dl,[esi]
	or	dl,dl
	jz	enumerate_section
	cmp	dl,1
	je	enumerate_extrn
	cmp	dl,2
	je	enumerate_public
	add	esi,0Ch
	jmp	enumerate_symbols
      enumerate_section:
	mov	edx,ecx
	shl	edx,8
	mov	[esi],edx
	inc	ecx
	add	esi,14h
	jmp	enumerate_symbols
      enumerate_extrn:
	mov	edx,eax
	shl	edx,8
	mov	dl,1
	mov	[esi],edx
	inc	eax
	add	esi,0Ch
	jmp	enumerate_symbols
      enumerate_public:
	mov	edx,eax
	shl	edx,8
	mov	dl,2
	mov	[esi],edx
	inc	eax
	add	esi,10h
	jmp	enumerate_symbols
      symbols_enumerated:
	mov	[ebx+0Ch],eax
	mov	ebp,[additional_memory]
	sub	ebp,[header_data]
	mov	edi,[header_data]
	add	edi,14h
	mov	esi,[sections_data]
      find_section:
	cmp	esi,[header_data]
	je	sections_finished
	mov	al,[esi]
	or	al,al
	jz	section_found
	add	esi,0Ch
	cmp	al,2
	jne	find_section
	add	esi,4
	jmp	find_section
      section_found:
	push	esi edi
	mov	esi,[esi+4]
	or	esi,esi
	jz	default_section
	mov	ecx,[esi]
	add	esi,4
	rep	movs byte [edi],[esi]
	jmp	section_name_ok
      default_section:
	mov	al,'.'
	stos	byte [edi]
	mov	eax,'flat'
	stos	dword [edi]
      section_name_ok:
	pop	edi esi
	mov	eax,[esi+0Ch]
	mov	[edi+10h],eax
	mov	eax,[esi+10h]
	mov	[edi+24h],eax
	test	al,80h
	jnz	section_ptr_ok
	mov	eax,[esi+8]
	sub	eax,[code_start]
	add	eax,ebp
	mov	[edi+14h],eax
      section_ptr_ok:
	mov	ebx,[code_start]
	mov	edx,[code_size]
	add	ebx,edx
	add	edx,ebp
	xor	ecx,ecx
	add	esi,14h
      find_relocations:
	cmp	esi,[header_data]
	je	section_relocations_done
	mov	al,[esi]
	or	al,al
	jz	section_relocations_done
	cmp	al,2
	ja	add_relocation
	jb	next_relocation
	add	esi,10h
	jmp	find_relocations
      add_relocation:
	lea	eax,[ebx+0Ah]
	cmp	eax,[display_buffer]
	ja	out_of_memory
	mov	eax,[esi+4]
	mov	[ebx],eax
	mov	eax,[esi+8]
	mov	eax,[eax]
	shr	eax,8
	mov	[ebx+4],eax
	movzx	ax,byte [esi]
	mov	[ebx+8],ax
	add	ebx,0Ah
	inc	ecx
      next_relocation:
	add	esi,0Ch
	jmp	find_relocations
      section_relocations_done:
	cmp	ecx,10000h
	jae	out_of_memory		; not honest, it's a COFF limitation
	mov	[edi+20h],cx
	jcxz	section_relocations_ok
	mov	[edi+18h],edx
      section_relocations_ok:
	sub	ebx,[code_start]
	mov	[code_size],ebx
	add	edi,28h
	jmp	find_section
      sections_finished:
	mov	edx,[header_data]
	mov	ebx,[code_size]
	add	ebp,ebx
	mov	[edx+8],ebp
	add	ebx,[code_start]
	mov	edi,ebx
	mov	ecx,[edx+0Ch]
	imul	ecx,12h shr 1
	xor	eax,eax
	shr	ecx,1
	jnc	zero_symbols_table
	stos	word [edi]
      zero_symbols_table:
	rep	stos dword [edi]
	mov	edx,edi
	stos	dword [edi]
	mov	esi,[sections_data]
      make_section_symbols:
	cmp	esi,[header_data]
	je	section_symbols_ok
	mov	al,[esi]
	or	al,al
	jz	add_section_symbol
	add	esi,0Ch
	cmp	al,2
	jne	make_section_symbols
	add	esi,4
	jmp	make_section_symbols
      add_section_symbol:
	call	store_symbol_name
	mov	eax,[esi]
	shr	eax,8
	inc	eax
	mov	[ebx+0Ch],ax
	mov	byte [ebx+10h],3
	add	esi,14h
	add	ebx,12h
	jmp	make_section_symbols
      section_symbols_ok:
	mov	esi,[sections_data]
      make_symbols_table:
	cmp	esi,[header_data]
	je	symbols_table_ok
	mov	al,[esi]
	cmp	al,1
	je	add_extrn_symbol
	cmp	al,2
	je	add_public_symbol
	add	esi,0Ch
	or	al,al
	jnz	make_symbols_table
	add	esi,8
	jmp	make_symbols_table
      add_extrn_symbol:
	call	store_symbol_name
	mov	byte [ebx+10h],2
	add	esi,0Ch
	add	ebx,12h
	jmp	make_symbols_table
      add_public_symbol:
	call	store_symbol_name
	mov	eax,[esi+0Ch]
	mov	[current_line],eax
	mov	eax,[esi+8]
	mov	cl,[eax+11]
	or	cl,cl
	jz	public_constant
	cmp	cl,2
	jne	invalid_use_of_symbol
	mov	ecx,[eax+4]
	mov	ecx,[ecx]
	or	cl,cl
	jnz	invalid_use_of_symbol
	shr	ecx,8
	inc	cx
	mov	[ebx+0Ch],cx
      store_public_symbol:
	mov	eax,[eax]
	mov	[ebx+8],eax
	mov	byte [ebx+10h],2
	add	esi,10h
	add	ebx,12h
	jmp	make_symbols_table
      public_constant:
	mov	word [ebx+0Ch],0FFFFh
	mov	ecx,[eax]
	shr	ecx,31
	neg	ecx
	cmp	ecx,[eax+4]
	jne	value_out_of_range
	jmp	store_public_symbol
      symbols_table_ok:
	mov	eax,edi
	sub	eax,edx
	mov	[edx],eax
	sub	edi,[code_start]
	mov	[code_size],edi
	mov	[written_size],0
	mov	edx,[output_file]
	call	create
	jc	write_failed
	mov	edx,[header_data]
	mov	ecx,[additional_memory]
	sub	ecx,edx
	add	[written_size],ecx
	call	write
	jc	write_failed
	mov	edx,[code_start]
	mov	ecx,[code_size]
	add	[written_size],ecx
	call	write
	jc	write_failed
	call	close
	ret
      store_symbol_name:
	push	esi
	mov	esi,[esi+4]
	or	esi,esi
	jz	default_name
	lods	dword [esi]
	mov	ecx,eax
	cmp	ecx,8
	ja	add_string
	push	edi
	mov	edi,ebx
	rep	movs byte [edi],[esi]
	pop	edi esi
	ret
      default_name:
	mov	dword [ebx],'.fla'
	mov	dword [ebx+4],'t'
	pop	esi
	ret
      add_string:
	mov	eax,edi
	sub	eax,edx
	mov	[ebx+4],eax
	inc	ecx
	rep	movs byte [edi],[esi]
	pop	esi
	ret

formatter_symbols:
 db 6,'binary',18h,1
 db 4,'code',19h,5
 db 4,'coff',18h,4
 db 7,'console',1Bh,3
 db 4,'data',19h,6
 db 11,'discardable',19h,25
 db 3,'dll',1Bh,80h
 db 10,'executable',19h,29
 db 6,'export',1Ah,0
 db 6,'fixups',1Ah,5
 db 3,'gui',1Bh,2
 db 6,'import',1Ah,1
 db 2,'ms',18h,41h
 db 2,'mz',18h,2
 db 6,'native',1Bh,1
 db 2,'pe',18h,3
 db 8,'readable',19h,30
 db 8,'resource',1Ah,2
 db 9,'shareable',19h,28
 db 9,'writeable',19h,31
 db 0
