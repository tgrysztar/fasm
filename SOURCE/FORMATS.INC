
; flat assembler  version 1.10
; Copyright (c) 1999-2000, Tomasz Grysztar
; All rights reserved.

formatter:
	mov	[written_size],0
	mov	edx,[output_file]
	call	create
	jc	write_failed
	call	write_header
	mov	edx,[code_start]
	mov	ecx,[code_size]
	add	[written_size],ecx
	call	write
	jc	write_failed
	call	close
	ret

format_directive:
	cmp	edi,[code_start]
	jne	unexpected_instruction
	cmp	[output_format],0
	jne	unexpected_instruction
	lods	byte [esi]
	cmp	al,15h
	jne	invalid_argument
	lods	byte [esi]
	mov	[output_format],al
	cmp	al,2
	je	format_mz
	cmp	al,3
	je	format_pe
	jmp	instruction_assembled
entry_directive:
	mov	al,[output_format]
	cmp	al,2
	je	mz_entry
	cmp	al,3
	je	pe_entry
	jmp	unexpected_instruction
stack_directive:
	mov	al,[output_format]
	cmp	al,2
	je	mz_stack
	cmp	al,3
	je	pe_stack
	jmp	unexpected_instruction
heap_directive:
	mov	al,[output_format]
	cmp	al,2
	je	mz_heap
	cmp	al,3
	je	pe_heap
	jmp	unexpected_instruction
mark_relocation:
	cmp	[value_type],0
	je	relocation_ok
	cmp	[virtual_mode],0
	jne	relocation_ok
	cmp	[output_format],2
	je	mark_mz_relocation
	cmp	[output_format],3
	je	mark_pe_relocation
      relocation_ok:
	ret
write_header:
	mov	al,[output_format]
	cmp	al,2
	je	write_mz_header
	cmp	al,3
	je	write_pe_header
	ret

format_mz:
	mov	[initial_ss_defined],0
	mov	[initial_sp],1000h
	mov	[initial_cs],0
	mov	[initial_ip],0
	mov	[heap_size],0FFFFh
	mov	[code_type],16
	jmp	instruction_assembled
mark_mz_relocation:
	push	eax ebx
	mov	ebx,[number_of_relocations]
	shl	ebx,2
	inc	[number_of_relocations]
	add	ebx,[additional_memory]
	add	ebx,1Ch
	cmp	ebx,[additional_memory_end]
	je	out_of_memory
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
	pop	ebx eax
	ret
segment_directive:
	cmp	[output_format],2
	jne	unexpected_instruction
	cmp	[virtual_mode],0
	jne	unexpected_instruction
	lods	byte [esi]
	cmp	al,4
	jne	invalid_argument
	lods	dword [esi]
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
	je	.new
	cmp	cl,[ebx+9]
	je	symbol_already_defined
	xchg	[ebx],eax
	xor	edx,edx
	xchg	[ebx+4],edx
	cmp	eax,[ebx]
	jne	.changed
	or	edx,edx
	jnz	.changed
	jmp	segment_ok
      .changed:
	mov	[next_pass_needed],1
	jmp	segment_ok
      .new:
	mov	byte [ebx+8],1
	mov	[ebx+9],cl
	mov	byte [ebx+10],0
	mov	byte [ebx+11],1
	mov	[ebx],eax
	mov	dword [ebx+4],0
      segment_ok:
	mov	ah,16
	mov	al,[esi]
	cmp	al,18h
	jne	segment_type_ok
	lods	word [esi]
      segment_type_ok:
	mov	[code_type],ah
	jmp	instruction_assembled
mz_entry:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	[operand_size],1
	je	invalid_address
	cmp	[operand_size],2
	ja	invalid_address
	call	get_word_value
	cmp	[next_pass_needed],0
	je	check_initial_cs
	cmp	[current_pass],0
	je	initial_cs_ok
      check_initial_cs:
	cmp	[value_type],1
	jne	invalid_address
      initial_cs_ok:
	mov	[initial_cs],ax
	lods	byte [esi]
	cmp	al,':'
	jne	invalid_argument
	mov	[operand_size],0
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	[operand_size],1
	je	invalid_address
	cmp	[operand_size],2
	ja	invalid_address
	call	get_word_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	[initial_ip],ax
	jmp	instruction_assembled
mz_stack:
	call	get_operand_size
	cmp	[operand_size],1
	je	invalid_value
	cmp	[operand_size],2
	ja	invalid_value
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
	mov	[initial_sp],ax
	mov	[initial_ss_defined],0
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
	mov	[initial_ss],ax
	lods	byte [esi]
	cmp	al,':'
	jne	invalid_argument
	mov	[operand_size],0
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	cmp	[operand_size],1
	je	invalid_address
	cmp	[operand_size],2
	ja	invalid_address
	call	get_word_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	[initial_sp],ax
	mov	[initial_ss_defined],1
	jmp	instruction_assembled
mz_heap:
	cmp	[output_format],2
	jne	unexpected_instruction
	call	get_operand_size
	cmp	[operand_size],1
	je	invalid_value
	cmp	[operand_size],2
	ja	invalid_value
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	call	get_word_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	[heap_size],ax
	jmp	instruction_assembled
write_mz_header:
	cmp	[initial_ss_defined],0
	jne	mz_stack_ok
	mov	eax,[real_code_size]
	dec	eax
	shr	eax,4
	inc	eax
	mov	[initial_ss],ax
	shl	eax,4
	movzx	edx,[initial_sp]
	add	eax,edx
	mov	[real_code_size],eax
      mz_stack_ok:
	mov	edi,[additional_memory]
	mov	eax,[number_of_relocations]
	shl	eax,2
	add	eax,1Ch
	add	edi,eax
	mov	ecx,0Fh
	add	eax,0Fh
	and	eax,1111b
	sub	ecx,eax
	xor	al,al
	rep	stos byte [es:edi]
	sub	edi,[additional_memory]
	mov	ecx,edi
	shr	edi,4
	mov	edx,[additional_memory]
	mov	word [edx],'MZ' 	; signature
	mov	[edx+8],di		; header size in paragraphs
	mov	eax,[number_of_relocations]
	mov	[edx+6],ax		; number of relocation entries
	mov	eax,[code_size]
	add	eax,ecx
	mov	esi,eax
	shr	esi,9
	and	eax,111111111b
	inc	si
	or	ax,ax
	jnz	mz_size_ok
	mov	ax,512
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
	mov	si,[heap_size]
	add	si,ax
	setalc
	mov	ah,al
	or	si,ax
	mov	word [edx+0Ch],si	; maximum memory in addition to code
	mov	ax,[initial_ss]
	mov	[edx+0Eh],ax
	mov	ax,[initial_sp]
	mov	[edx+10h],ax
	mov	ax,[initial_cs]
	mov	[edx+16h],ax
	mov	ax,[initial_ip]
	mov	[edx+14h],ax
	mov	word [edx+12h],0	; no checksum
	mov	word [edx+18h],1Ch	; offset of relocation table
	mov	word [edx+1Ah],0	; overlay number
	add	[written_size],ecx
	call	write
	jc	write_failed
	ret

number_of_relocations dd ?
heap_size dw ?
initial_cs dw ?
initial_ip dw ?
initial_ss dw ?
initial_sp dw ?
initial_ss_defined db ?

format_pe:
	mov	[pe_flags],818Eh
	mov	[machine],14Ch		; intel 80386
	mov	[subsystem_type],3	; console
	mov	[subsystem_version],3 + 10 shl 16
	mov	[image_base],400000h
	mov	[number_of_dirs],16
	mov	[stack_reserve],1000h
	mov	[stack_commit],1000h
	mov	[heap_reserve],10000h
	mov	[heap_commit],0
	mov	[code_type],32
	mov	[reloc_labels],1
	mov	[entry_rva],1000h
	mov	[number_of_sections],0
	mov	[section_defined],0
	mov	dword [section_name],'.fla'
	mov	dword [section_name+4],'t'
	mov	[section_rva],1000h
	mov	[section_start],edi
	mov	[section_flags],0E0000060h
	mov	eax,edi
	sub	eax,[section_rva]
	sub	eax,[image_base]
	mov	[org_start],eax
      pe_settings:
	cmp	byte [esi],1Bh
	jne	instruction_assembled
	lods	word [esi]
	shr	ax,8
	test	al,0C0h
	jz	subsystem_setting
	test	al,80h
	jz	machine_setting
	cmp	al,80h
	je	pe_dll
	jmp	pe_settings
      pe_dll:
	or	[pe_flags],2000h
	jmp	pe_settings
      subsystem_setting:
	mov	[subsystem_type],ax
	cmp	byte [esi],'('
	jne	pe_settings
	inc	esi
	cmp	byte [esi],'.'
	jne	invalid_value
	inc	esi
	call	fp_to_version
	add	esi,12
	mov	[subsystem_version],eax
	jmp	pe_settings
      machine_setting:
	and	ax,3Fh
	add	ax,149h
	mov	[machine],ax
	jmp	pe_settings
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
	jne	unexpected_instruction
	cmp	[virtual_mode],0
	jne	unexpected_instruction
	cmp	edi,[code_start]
	jne	prepare_section
	cmp	[section_defined],0
	je	new_section
      prepare_section:
	call	close_section
      new_section:
	mov	[section_defined],1
	lods	byte [esi]
	cmp	al,27h
	jne	invalid_argument
	lea	edx,[esi+4]
	mov	ecx,[esi]
	lea	esi,[esi+4+ecx+1]
	cmp	ecx,8
	ja	name_too_long
	xor	eax,eax
	mov	dword [section_name],eax
	mov	dword [section_name+4],eax
	push	esi edi
	mov	edi,section_name
	mov	esi,edx
	rep	movs byte [edi],[esi]
	pop	edi esi
	mov	[code_type],32
	mov	[section_start],edi
	mov	[section_flags],0
	movzx	ebx,[number_of_sections]
	cmp	bx,(1000h-(40h+pe_stub_end-pe_stub)-(78h+16*8))/28h
	je	unexpected_instruction
	imul	ebx,28h
	add	ebx,[additional_memory]
	mov	eax,edi
	sub	eax,[section_rva]
	sub	eax,[image_base]
	mov	[org_start],eax
      get_section_flags:
	lods	byte [esi]
	cmp	al,1Ah
	je	set_directory
	cmp	al,19h
	je	section_flag
	cmp	al,18h
	je	section_code_type
	dec	esi
	jmp	instruction_assembled
      set_directory:
	lods	byte [esi]
	movzx	eax,al
	mov	[directories+eax*4],ebx
	jmp	get_section_flags
      section_code_type:
	lods	byte [esi]
	mov	[code_type],al
	cmp	al,16
	jne	get_section_flags
	or	byte [section_flags],4
	jmp	get_section_flags
      section_flag:
	lods	byte [esi]
	mov	cl,al
	mov	eax,1
	shl	eax,cl
	or	[section_flags],eax
	jmp	get_section_flags
      close_section:
	mov	eax,edi
	sub	eax,[section_start]
	mov	ebp,eax
	dec	ebp
	shr	ebp,12
	inc	ebp
	shl	ebp,12
	cmp	edi,[undefined_data_end]
	jne	align_section
	mov	edi,[undefined_data_start]
      align_section:
	mov	edx,edi
	sub	edx,[section_start]
	mov	ecx,edx
	dec	ecx
	shr	ecx,9
	inc	ecx
	shl	ecx,9
	mov	ebx,ecx
	sub	ecx,edx
	xor	al,al
	rep	stos byte [edi]
	mov	ecx,ebx
	push	edi
	movzx	edi,[number_of_sections]
	imul	edi,28h
	add	edi,[additional_memory]
	mov	eax,[additional_memory_end]
	sub	eax,28h
	cmp	edi,eax
	jae	out_of_memory
	inc	[number_of_sections]
	mov	eax,dword [section_name]
	stos	dword [edi]
	mov	eax,dword [section_name+4]
	stos	dword [edi]
	mov	eax,ebp
	stos	dword [edi]
	mov	eax,[section_rva]
	add	[section_rva],ebp
	stos	dword [edi]
	mov	eax,ecx
	stos	dword [edi]
	mov	eax,[section_start]
	sub	eax,[code_start]
	stos	dword [edi]
	mov	eax,edx
	stos	dword [edi]
	xor	eax,eax
	stos	dword [edi]
	stos	dword [edi]
	mov	eax,[section_flags]
	stos	dword [edi]
	pop	edi
	ret
pe_entry:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	mov	al,[operand_size]
	test	al,not 4
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
	sub	eax,[image_base]
	mov	[entry_rva],eax
	jmp	instruction_assembled
pe_stack:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	mov	al,[operand_size]
	test	al,not 4
	jnz	invalid_address
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	[stack_reserve],eax
	cmp	byte [esi],','
	jne	default_stack_commit
	lods	byte [esi]
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	mov	al,[operand_size]
	test	al,not 4
	jnz	invalid_address
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	[stack_commit],eax
	cmp	eax,[stack_reserve]
	ja	value_out_of_range
	jmp	instruction_assembled
      default_stack_commit:
	mov	[stack_commit],1000h
	mov	eax,[stack_reserve]
	cmp	eax,1000h
	ja	instruction_assembled
	mov	[stack_commit],eax
	jmp	instruction_assembled
pe_heap:
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	mov	al,[operand_size]
	test	al,not 4
	jnz	invalid_address
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	[heap_reserve],eax
	cmp	byte [esi],','
	jne	default_heap_commit
	lods	byte [esi]
	call	get_operand_size
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_argument
	mov	al,[operand_size]
	test	al,not 4
	jnz	invalid_address
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_dword_value
	cmp	[value_type],0
	jne	invalid_use_of_symbol
	mov	[heap_commit],eax
	cmp	eax,[heap_reserve]
	ja	value_out_of_range
	jmp	instruction_assembled
      default_heap_commit:
	mov	[heap_commit],0
	jmp	instruction_assembled

mark_pe_relocation:
	push	eax ebx
	mov	ebx,[additional_memory_end]
	sub	ebx,4
	cmp	ebx,[additional_memory]
	jbe	out_of_memory
	mov	[additional_memory_end],ebx
	inc	[number_of_relocations]
	mov	eax,edi
	sub	eax,[section_start]
	add	eax,[section_rva]
	mov	[ebx],eax
	pop	ebx eax
	ret
make_pe_reloc:
	push	esi
	mov	ecx,[number_of_relocations]
	jecxz	fixups_done
	mov	esi,[additional_memory_end]
	lea	esi,[esi+ecx*4-4]
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
	stos	dword [edi]
	mov	ebx,edi
	mov	eax,8
	stos	dword [edi]
      store_fixup:
	add	dword [ebx],2
	mov	eax,[esi]
	sub	esi,4
	and	ax,0FFFh
	or	ax,3000h
	stos	word [edi]
	loop	make_fixups
      fixups_done:
	pop	esi
	ret
finish_pe:
	call	close_section
	cmp	[number_of_relocations],0
	je	pe_reloc_ok
	mov	dword [section_name],'.rel'
	mov	dword [section_name+4],'oc'
	mov	[section_start],edi
	mov	[section_flags],42000040h
	movzx	eax,[number_of_sections]
	imul	eax,28h
	add	eax,[additional_memory]
	mov	[directories+14h],eax
	mov	eax,edi
	sub	eax,[section_rva]
	mov	[org_start],eax
	call	make_pe_reloc
	call	close_section
      pe_reloc_ok:
	mov	eax,edi
	sub	eax,[code_start]
	mov	[code_size],eax
	ret

write_pe_header:
	movzx	eax,[number_of_sections]
	imul	eax,28h
	add	eax,[additional_memory]
	xchg	[additional_memory],eax
	mov	[sections_list],eax
	call	create_pe_stub
	mov	edi,[additional_memory]
	mov	ecx,78h
	movzx	ax,[number_of_dirs]
	shl	ax,3
	add	cx,ax
	mov	esi,ecx
	xor	al,al
	rep	stos byte [edi]
	mov	edx,[additional_memory]
	mov	word [edx],'PE' 	; signature
	mov	ax,[machine]
	mov	[edx+4],ax		; CPU type
	movzx	ecx,[number_of_sections]
	mov	[edx+6],cx		; count of sections
	mov	ax,si
	sub	ax,18h
	mov	[edx+14h],ax		; size of optional header
	mov	ax,[pe_flags]
	mov	word [edx+16h],ax	; flags
	mov	word [edx+18h],10Bh	; magic optional header
	mov	eax,[entry_rva]
	mov	[edx+28h],eax		; entry point rva
	mov	dword [edx+38h],1000h	; section alignment
	mov	dword [edx+3Ch],200h	; file alignment
	imul	ecx,28h
	add	ecx,esi
	add	ecx,[stub_size]
	mov	[edx+54h],ecx		; size of headers
	mov	ax,[subsystem_type]
	mov	[edx+5Ch],ax		; subsystem type
	movzx	ax,[number_of_dirs]
	mov	[edx+74h],ax		; number of directories
	mov	eax,ecx
	dec	ecx
	shr	ecx,9
	inc	ecx
	shl	ecx,9
	sub	ecx,eax
	mov	[file_alignment],ecx
	add	[edx+54h],ecx		; align size of headers
	add	eax,ecx
	push	eax ebx
	xor	ebx,ebx
      process_dirs:
	mov	eax,[directories+ebx*4]
	or	eax,eax
	jz	dir_ok
	mov	ecx,[eax+18h]
	mov	eax,[eax+0Ch]
	mov	[edx+78h+ebx*8],eax	; directory rva
	mov	[edx+7Ch+ebx*8],ecx	; directory size
      dir_ok:
	inc	bl
	cmp	bl,[number_of_dirs]
	jb	process_dirs
	pop	ebx eax
	movzx	ecx,[number_of_sections]
	mov	edi,[sections_list]
	mov	ebp,1000h
      process_sections:
	push	eax
	mov	dword [edi+18h],0
	test	byte [edi+24h],20h
	jz	no_code_section
	mov	eax,[edi+8]
	add	[edx+1Ch],eax		; size of code
	cmp	dword [edx+2Ch],0
	jne	no_code_section
	mov	eax,[edi+0Ch]
	mov	[edx+2Ch],eax		; base of code
      no_code_section:
	test	byte [edi+24h],0C0h
	jz	no_data_section
	mov	eax,[edi+8]
	test	byte [edi+24h],40h
	jz	udata_section
	add	[edx+20h],eax		; size of initialized data
	jmp	data_section_size_ok
      udata_section:
	add	[edx+24h],eax		; size of uninitialized data
      data_section_size_ok:
	cmp	dword [edx+30h],0
	jne	no_data_section
	mov	eax,[edi+0Ch]
	mov	[edx+30h],eax		; base of data
      no_data_section:
	pop	eax
	add	[edi+14h],eax
	add	ebp,[edi+8]
	add	edi,28h
	loop	process_sections
	mov	[edx+50h],ebp		; size of image
	mov	eax,[image_base]
	mov	dword [edx+34h],eax	; base of image
	mov	eax,[stack_reserve]
	mov	[edx+60h],eax
	mov	eax,[stack_commit]
	mov	[edx+64h],eax
	mov	eax,[heap_reserve]
	mov	[edx+68h],eax
	mov	eax,[heap_commit]
	mov	[edx+6Ch],eax
	mov	word [edx+40h],1	; OS version
	mov	eax,[subsystem_version]
	mov	[edx+48h],eax
	mov	ecx,esi
	add	[written_size],ecx
	call	write
	jc	write_failed
	mov	edx,[sections_list]
	movzx	ecx,[number_of_sections]
	imul	ecx,28h
	add	[written_size],ecx
	call	write
	jc	write_failed
	mov	ecx,[file_alignment]
	mov	edi,[additional_memory]
	mov	edx,edi
	xor	al,al
	rep	stos byte [edi]
	mov	ecx,[file_alignment]
	add	[written_size],ecx
	call	write
	jc	write_failed
	ret

create_pe_stub:
	mov	edx,[additional_memory]
	mov	edi,edx
	xor	eax,eax
	mov	ecx,20h
	rep	stos dword [edi]
	mov	eax,40h+pe_stub_end-pe_stub
	mov	cx,100h+pe_stub_end-pe_stub
	mov	word [edx],'MZ' 	; signature
	mov	word [edx+4],1		; number of pages
	mov	word [edx+2],ax 	; number of bytes in last page
	mov	word [edx+8],4		; header size in paragraphs
	mov	word [edx+0Ah],10h	; minimum memory in addition to code
	mov	word [edx+0Ch],0FFFFh	; maximum memory in addition to code
	mov	word [edx+10h],cx	; initial SP
	mov	word [edx+3Ch],ax	; offset of PE header
	mov	word [edx+18h],40h
	lea	edi,[edx+40h]
	mov	esi,pe_stub
	mov	ecx,pe_stub_end-pe_stub
	rep	movs byte [edi],[esi]
	mov	ecx,eax
	mov	[stub_size],eax
	add	[written_size],ecx
	call	write
	jc	write_failed
	ret
    pe_stub:
	use16
	push	cs
	pop	ds
	mov	dx,pe_message-pe_stub
	mov	ah,9
	int	21h
	mov	ax,4C01h
	int	21h
    pe_message db 'this program cannot be run in DOS mode.',0Dh,0Ah,24h
	rq	1
    pe_stub_end:
	use32

stub_size dd ?
number_of_dirs db ?
number_of_sections db ?
section_defined db ?
section_name dq ?
section_start dd ?
section_rva dd ?
section_flags dd ?
sections_list dd ?
directories rd 16
entry_rva dd ?
image_base dd ?
stack_reserve dd ?
stack_commit dd ?
heap_reserve dd ?
heap_commit dd ?
file_alignment dd ?
pe_flags dw ?
machine dw ?
subsystem_type dw ?
subsystem_version dd ?
