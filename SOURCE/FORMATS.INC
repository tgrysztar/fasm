
; flat assembler source
; Copyright (c) 1999-2001, Tomasz Grysztar
; All rights reserved.

format_directive:
	cmp	edi,[code_start]
	jne	unexpected_instruction
	cmp	[output_format],0
	jne	unexpected_instruction
	lods	byte [esi]
	cmp	al,18h
	jne	invalid_argument
	lods	byte [esi]
	mov	[output_format],al
	cmp	al,2
	je	format_mz
	cmp	al,3
	je	format_pe
	jmp	instruction_assembled
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
mark_relocation:
	cmp	[value_type],0
	je	relocation_ok
	cmp	[virtual_data],0
	jne	relocation_ok
	cmp	[output_format],2
	je	mark_mz_relocation
	cmp	[output_format],3
	je	mark_pe_relocation
      relocation_ok:
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
	cmp	eax,10h
	jb	invalid_use_of_symbol
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
	mov	[org_start],edi
	mov	eax,edx
	call	undefined_data
	mov	eax,edi
	sub	eax,[code_start]
	shr	eax,4
	cmp	eax,10000h
	jae	value_out_of_range
	mov	cl,[current_pass]
	cmp	byte [ebx+8],0
	je	new_segment
	cmp	cl,[ebx+9]
	je	symbol_already_defined
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
	mov	byte [ebx+8],1
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
	setalc
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
	xor	edx,edx
      pe_settings:
	cmp	byte [esi],84h
	je	get_stub_name
	cmp	byte [esi],1Bh
	jne	pe_settings_ok
	lods	byte [esi]
	lods	byte [esi]
	test	al,80h+40h
	jz	subsystem_setting
	test	al,80h
	jz	machine_setting
	cmp	al,80h
	je	pe_dll
	jmp	pe_settings
      pe_dll:
	bts	[format_flags],8
	jc	symbol_already_defined
	jmp	pe_settings
      machine_setting:
	bts	[format_flags],6
	jc	symbol_already_defined
	and	ax,3Fh
	add	ax,149h
	mov	[machine],ax
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
	je	make_pe_stub
	add	edi,[stub_size]
	jmp	pe_stub_ok
      make_pe_stub:
	call	make_stub
      pe_stub_ok:
	mov	[header_data],edi
	mov	edx,edi
	mov	eax,edi
	sub	eax,[code_start]
	mov	[stub_size],eax
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
	mov	dword [edx+34h],400000h ; base of image
	mov	dword [edx+38h],1000h	; section alignment
	mov	dword [edx+3Ch],200h	; file alignment
	mov	word [edx+40h],1	; OS version
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
	mov	[org_start],eax
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
section_directive:
	cmp	[output_format],3
	jne	illegal_instruction
	cmp	[virtual_data],0
	jne	illegal_instruction
	call	close_section
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
	mov	[org_start],eax
      get_section_flags:
	lods	byte [esi]
	cmp	al,1Ah
	je	set_directory
	cmp	al,19h
	je	section_flag
	dec	esi
	cmp	al,13h
	jne	instruction_assembled
	lods	byte [esi]
	lods	byte [esi]
	mov	[code_type],al
	cmp	al,16
	jne	instruction_assembled
	or	byte [ebx+24h],4
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
      close_section:
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
	sub	dword [ebx+14h],eax
	mov	eax,[ebx+8]
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
	call	get_size_operator
	cmp	al,'('
	jne	invalid_argument
	test	ah,not 4
	jnz	invalid_address
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
	call	get_size_operator
	cmp	al,'('
	jne	invalid_argument
	test	ah,not 4
	jnz	invalid_address
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
	call	get_size_operator
	cmp	al,'('
	jne	invalid_argument
	test	ah,not 4
	jnz	invalid_address
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
	call	get_size_operator
	cmp	al,'('
	jne	invalid_argument
	test	ah,not 4
	jnz	invalid_address
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
	call	get_size_operator
	cmp	al,'('
	jne	invalid_argument
	test	ah,not 4
	jnz	invalid_address
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
finish_pe:
	call	close_section
	mov	edx,[header_data]
	mov	[edx+50h],eax
	cmp	[number_of_relocations],0
	jle	pe_flags_ok
	or	word [edx+16h],1
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
	mov	eax,edi
	sub	eax,[code_start]
	mov	[code_size],eax
	ret

