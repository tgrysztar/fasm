
; flat assembler core
; Copyright (c) 1999-2003, Tomasz Grysztar.
; All rights reserved.

simple_instruction:
	stos	byte [edi]
	jmp	instruction_assembled
simple_instruction_16bit:
	cmp	[code_type],32
	je	size_prefix
	stos	byte [edi]
	jmp	instruction_assembled
      size_prefix:
	mov	ah,al
	mov	al,66h
	stos	word [edi]
	jmp	instruction_assembled
simple_instruction_32bit:
	cmp	[code_type],16
	je	size_prefix
	stos	byte [edi]
	jmp	instruction_assembled
simple_extended_instruction:
	mov	ah,al
	mov	al,0Fh
	stos	word [edi]
	jmp	instruction_assembled
prefix_instruction:
	stos	byte [edi]
	jmp	continue_line
segment_prefix:
	mov	ah,al
	shr	ah,4
	cmp	ah,6
	jne	illegal_instruction
	and	al,1111b
	mov	[segment_register],al
	call	store_segment_prefix
	jmp	continue_line
int_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	ah,1
	ja	invalid_operand_size
	cmp	al,'('
	jne	invalid_operand
	call	get_byte_value
	mov	ah,al
	mov	al,0CDh
	stos	word [edi]
	jmp	instruction_assembled
aa_instruction:
	push	eax
	mov	bl,10
	cmp	byte [esi],'('
	jne	.store
	inc	esi
	xor	al,al
	xchg	al,[operand_size]
	cmp	al,1
	ja	invalid_operand_size
	call	get_byte_value
	mov	bl,al
      .store:
	cmp	[operand_size],0
	jne	invalid_operand
	pop	eax
	mov	ah,bl
	stos	word [edi]
	jmp	instruction_assembled

basic_instruction:
	mov	[base_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	basic_reg
	cmp	al,'['
	jne	invalid_operand
      basic_mem:
	call	get_address
	push	edx bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	cmp	byte [esi],11h
	sete	al
	mov	[imm_sized],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	je	basic_mem_imm
	cmp	al,10h
	jne	invalid_operand
      basic_mem_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	al,ah
	cmp	al,1
	je	basic_mem_reg_8bit
	cmp	al,2
	je	basic_mem_reg_16bit
	cmp	al,4
	je	basic_mem_reg_32bit
	jmp	invalid_operand_size
      basic_mem_reg_8bit:
	call	store_instruction
	jmp	instruction_assembled
      basic_mem_reg_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      basic_mem_reg_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      basic_mem_imm:
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
	pop	cx bx edx
	mov	[base_code],80h
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      basic_mem_imm_16bit:
	call	get_word_value
	mov	word [value],ax
	mov	al,[base_code]
	shr	al,3
	mov	[postbyte_register],al
	call	operand_16bit_prefix
	pop	cx bx edx
	cmp	[value_type],0
	jne	.store
	cmp	[imm_sized],0
	jne	.store
	cmp	word [value],80h
	jb	basic_mem_simm_8bit
	cmp	word [value],-80h
	jae	basic_mem_simm_8bit
      .store:
	mov	[base_code],81h
	call	store_instruction
	mov	ax,word [value]
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      basic_mem_simm_8bit:
	mov	[base_code],83h
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      basic_mem_imm_32bit:
	call	get_dword_value
	mov	dword [value],eax
	mov	al,[base_code]
	shr	al,3
	mov	[postbyte_register],al
	call	operand_32bit_prefix
	pop	cx bx edx
	cmp	[value_type],0
	jne	.store
	cmp	[imm_sized],0
	jne	.store
	cmp	dword [value],80h
	jb	basic_mem_simm_8bit
	cmp	dword [value],-80h
	jae	basic_mem_simm_8bit
      .store:
	mov	[base_code],81h
	call	store_instruction
	mov	eax,dword [value]
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      basic_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	cmp	byte [esi],11h
	sete	al
	mov	[imm_sized],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	basic_reg_reg
	cmp	al,'('
	je	basic_reg_imm
	cmp	al,'['
	jne	invalid_operand
      basic_reg_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	basic_reg_mem_8bit
	cmp	al,2
	je	basic_reg_mem_16bit
	cmp	al,4
	je	basic_reg_mem_32bit
	jmp	invalid_operand_size
      basic_reg_mem_8bit:
	add	[base_code],2
	call	store_instruction
	jmp	instruction_assembled
      basic_reg_mem_16bit:
	call	operand_16bit_prefix
	add	[base_code],3
	call	store_instruction
	jmp	instruction_assembled
      basic_reg_mem_32bit:
	call	operand_32bit_prefix
	add	[base_code],3
	call	store_instruction
	jmp	instruction_assembled
      basic_reg_reg:
	lods	byte [esi]
	call	convert_register
	shl	al,3
	mov	bl,[postbyte_register]
	or	bl,al
	or	bl,11000000b
	mov	al,ah
	cmp	al,1
	je	basic_reg_reg_8bit
	cmp	al,2
	je	basic_reg_reg_16bit
	cmp	al,4
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
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      basic_reg_imm:
	mov	al,[operand_size]
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
	stos	word [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      basic_al_imm:
	mov	al,[base_code]
	add	al,4
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      basic_reg_imm_16bit:
	call	get_word_value
	mov	dx,ax
	call	operand_16bit_prefix
	mov	ah,[base_code]
	or	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	ah,bl
	cmp	[value_type],0
	jne	.store
	cmp	[imm_sized],0
	jne	.store
	cmp	dx,80h
	jb	basic_reg_simm_8bit
	cmp	dx,-80h
	jae	basic_reg_simm_8bit
      .store:
	or	bl,bl
	jz	basic_ax_imm
	mov	al,81h
	stos	word [edi]
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      basic_reg_simm_8bit:
	mov	al,83h
	stos	word [edi]
	mov	ax,dx
	stos	byte [edi]
	jmp	instruction_assembled
      basic_ax_imm:
	mov	al,[base_code]
	add	al,5
	stos	byte [edi]
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      basic_reg_imm_32bit:
	call	get_dword_value
	mov	edx,eax
	call	operand_32bit_prefix
	mov	ah,[base_code]
	or	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	ah,bl
	cmp	[value_type],0
	jne	.store
	cmp	[imm_sized],0
	jne	.store
	cmp	edx,80h
	jb	basic_reg_simm_8bit
	cmp	edx,-80h
	jae	basic_reg_simm_8bit
      .store:
	or	bl,bl
	jz	basic_eax_imm
	mov	al,81h
	stos	word [edi]
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      basic_eax_imm:
	mov	al,[base_code]
	add	al,5
	stos	byte [edi]
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
single_operand_instruction:
	mov	[base_code],0F6h
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	single_reg
	cmp	al,'['
	jne	invalid_operand
      single_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	single_mem_8bit
	cmp	al,2
	je	single_mem_16bit
	cmp	al,4
	je	single_mem_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      single_mem_8bit:
	call	store_instruction
	jmp	instruction_assembled
      single_mem_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      single_mem_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      single_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	al,ah
	cmp	al,1
	je	single_reg_8bit
	cmp	al,2
	je	single_reg_16bit
	cmp	al,4
	je	single_reg_32bit
	jmp	invalid_operand_size
      single_reg_8bit:
	mov	ah,bl
	mov	al,0F6h
	stos	word [edi]
	jmp	instruction_assembled
      single_reg_16bit:
	call	operand_16bit_prefix
	mov	ah,bl
	mov	al,0F7h
	stos	word [edi]
	jmp	instruction_assembled
      single_reg_32bit:
	call	operand_32bit_prefix
	mov	ah,bl
	mov	al,0F7h
	stos	word [edi]
	jmp	instruction_assembled
mov_instruction:
	mov	[base_code],88h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	mov_reg
	cmp	al,'['
	jne	invalid_operand
      mov_mem:
	call	get_address
	push	edx bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	je	mov_mem_imm
	cmp	al,10h
	jne	invalid_operand
      mov_mem_reg:
	lods	byte [esi]
	cmp	al,60h
	jae	mov_mem_sreg
	call	convert_register
	mov	[postbyte_register],al
	pop	cx bx edx
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
	jmp	instruction_assembled
      mov_mem_al:
	cmp	ch,2
	je	mov_mem_address16_al
	test	ch,4
	jnz	mov_mem_address32_al
	or	ch,ch
	jnz	invalid_address_size
	cmp	[code_type],32
	je	mov_mem_address32_al
	cmp	edx,10000h
	jb	mov_mem_address16_al
      mov_mem_address32_al:
	call	address_32bit_prefix
	call	store_segment_prefix_if_necessary
	mov	al,0A2h
      store_mov_address32:
	stos	byte [edi]
	push	instruction_assembled
	jmp	store_address_32bit_value
      mov_mem_address16_al:
	call	address_16bit_prefix
	call	store_segment_prefix_if_necessary
	mov	al,0A2h
      store_mov_address16:
	stos	byte [edi]
	mov	eax,edx
	stos	word [edi]
	cmp	edx,10000h
	jge	value_out_of_range
	jmp	instruction_assembled
      mov_mem_reg_16bit:
	call	operand_16bit_prefix
	mov	al,[postbyte_register]
	or	al,bl
	or	al,bh
	jz	mov_mem_ax
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      mov_mem_ax:
	cmp	ch,2
	je	mov_mem_address16_ax
	test	ch,4
	jnz	mov_mem_address32_ax
	or	ch,ch
	jnz	invalid_address_size
	cmp	[code_type],32
	je	mov_mem_address32_ax
	cmp	edx,10000h
	jb	mov_mem_address16_ax
      mov_mem_address32_ax:
	call	address_32bit_prefix
	call	store_segment_prefix_if_necessary
	mov	al,0A3h
	jmp	store_mov_address32
      mov_mem_address16_ax:
	call	address_16bit_prefix
	call	store_segment_prefix_if_necessary
	mov	al,0A3h
	jmp	store_mov_address16
      mov_mem_reg_32bit:
	call	operand_32bit_prefix
	mov	al,[postbyte_register]
	or	al,bl
	or	al,bh
	jz	mov_mem_ax
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      mov_mem_sreg:
	cmp	al,70h
	jae	invalid_operand
	sub	al,61h
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	ah,[operand_size]
	or	ah,ah
	jz	mov_mem_sreg_size_ok
	cmp	ah,2
	je	mov_mem16_sreg
	cmp	ah,4
	je	mov_mem32_sreg
	jmp	invalid_operand_size
      mov_mem32_sreg:
	call	operand_32bit_prefix
	jmp	mov_mem_sreg_size_ok
      mov_mem16_sreg:
	call	operand_16bit_prefix
      mov_mem_sreg_size_ok:
	mov	[base_code],8Ch
	call	store_instruction
	jmp	instruction_assembled
      mov_mem_imm:
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
	pop	cx bx edx
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      mov_mem_imm_16bit:
	call	get_word_value
	mov	word [value],ax
	mov	[postbyte_register],0
	mov	[base_code],0C7h
	call	operand_16bit_prefix
	pop	cx bx edx
	call	store_instruction
	mov	ax,word [value]
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      mov_mem_imm_32bit:
	call	get_dword_value
	mov	dword [value],eax
	mov	[postbyte_register],0
	mov	[base_code],0C7h
	call	operand_32bit_prefix
	pop	cx bx edx
	call	store_instruction
	mov	eax,dword [value]
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      mov_reg:
	lods	byte [esi]
	cmp	al,50h
	jae	mov_sreg
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	je	mov_reg_mem
	cmp	al,'('
	je	mov_reg_imm
	cmp	al,10h
	jne	invalid_operand
      mov_reg_reg:
	lods	byte [esi]
	cmp	al,50h
	jae	mov_reg_sreg
	call	convert_register
	shl	al,3
	mov	bl,[postbyte_register]
	or	bl,al
	or	bl,11000000b
	mov	al,ah
	cmp	al,1
	je	mov_reg_reg_8bit
	cmp	al,2
	je	mov_reg_reg_16bit
	cmp	al,4
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
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      mov_reg_sreg:
	mov	ah,al
	shr	ah,4
	cmp	ah,5
	je	mov_reg_creg
	cmp	ah,7
	je	mov_reg_dreg
	ja	invalid_operand
	sub	al,61h
	mov	bl,[postbyte_register]
	shl	al,3
	or	bl,al
	or	bl,11000000b
	cmp	[operand_size],4
	je	mov_reg_sreg32
	cmp	[operand_size],2
	jne	invalid_operand_size
	call	operand_16bit_prefix
	jmp	mov_reg_sreg_store
     mov_reg_sreg32:
	call	operand_32bit_prefix
     mov_reg_sreg_store:
	mov	al,8Ch
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      mov_reg_creg:
	mov	bh,20h
	jmp	mov_reg_xrx
      mov_reg_dreg:
	mov	bh,21h
      mov_reg_xrx:
	and	al,111b
	mov	bl,[postbyte_register]
	shl	al,3
	or	bl,al
	or	bl,11000000b
	cmp	[operand_size],4
	jne	invalid_operand_size
	mov	ah,bh
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      mov_reg_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	mov_reg_mem_8bit
	cmp	al,2
	je	mov_reg_mem_16bit
	cmp	al,4
	je	mov_reg_mem_32bit
	jmp	invalid_operand_size
      mov_reg_mem_8bit:
	mov	al,[postbyte_register]
	or	al,bl
	or	al,bh
	jz	mov_al_mem
	add	[base_code],2
	call	store_instruction
	jmp	instruction_assembled
      mov_al_mem:
	cmp	ch,2
	je	mov_al_mem_address16
	test	ch,4
	jnz	mov_al_mem_address32
	or	ch,ch
	jnz	invalid_address_size
	cmp	[code_type],32
	je	mov_al_mem_address32
	cmp	edx,10000h
	jb	mov_al_mem_address16
      mov_al_mem_address32:
	call	address_32bit_prefix
	call	store_segment_prefix_if_necessary
	mov	al,0A0h
	jmp	store_mov_address32
      mov_al_mem_address16:
	call	address_16bit_prefix
	call	store_segment_prefix_if_necessary
	mov	al,0A0h
	jmp	store_mov_address16
      mov_reg_mem_16bit:
	call	operand_16bit_prefix
	mov	al,[postbyte_register]
	or	al,bl
	or	al,bh
	jz	mov_ax_mem
	add	[base_code],3
	call	store_instruction
	jmp	instruction_assembled
      mov_ax_mem:
	cmp	ch,2
	je	mov_ax_mem_address16
	test	ch,4
	jnz	mov_ax_mem_address32
	or	ch,ch
	jnz	invalid_address_size
	cmp	[code_type],32
	je	mov_ax_mem_address32
	cmp	edx,10000h
	jb	mov_ax_mem_address16
      mov_ax_mem_address32:
	call	address_32bit_prefix
	call	store_segment_prefix_if_necessary
	mov	al,0A1h
	jmp	store_mov_address32
      mov_ax_mem_address16:
	call	address_16bit_prefix
	call	store_segment_prefix_if_necessary
	mov	al,0A1h
	jmp	store_mov_address16
      mov_reg_mem_32bit:
	call	operand_32bit_prefix
	mov	al,[postbyte_register]
	or	al,bl
	or	al,bh
	jz	mov_ax_mem
	add	[base_code],3
	call	store_instruction
	jmp	instruction_assembled
      mov_reg_imm:
	mov	al,[operand_size]
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
	stos	word [edi]
	jmp	instruction_assembled
      mov_reg_imm_16bit:
	call	get_word_value
	mov	dx,ax
	call	operand_16bit_prefix
	mov	al,[postbyte_register]
	and	al,111b
	add	al,0B8h
	stos	byte [edi]
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      mov_reg_imm_32bit:
	call	get_dword_value
	mov	edx,eax
	call	operand_32bit_prefix
	mov	al,[postbyte_register]
	and	al,111b
	add	al,0B8h
	stos	byte [edi]
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      mov_sreg:
	mov	ah,al
	shr	ah,4
	cmp	ah,5
	je	mov_creg
	cmp	ah,7
	je	mov_dreg
	ja	invalid_operand
	sub	al,61h
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	je	mov_sreg_mem
	cmp	al,10h
	jne	invalid_operand
      mov_sreg_reg:
	lods	byte [esi]
	call	convert_register
	or	ah,ah
	jz	mov_sreg_reg_size_ok
	cmp	ah,4
	je	mov_sreg_reg32
	cmp	ah,2
	je	mov_sreg_reg16
	jmp	invalid_operand_size
      mov_sreg_reg32:
	mov	ah,al
	call	operand_32bit_prefix
	mov	al,ah
	jmp	mov_sreg_reg_size_ok
      mov_sreg_reg16:
	mov	ah,al
	call	operand_16bit_prefix
	mov	al,ah
      mov_sreg_reg_size_ok:
	mov	bl,11000000b
	or	bl,al
	mov	al,[postbyte_register]
	shl	al,3
	or	bl,al
	mov	al,8Eh
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      mov_sreg_mem:
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	mov_sreg_mem_size_ok
	cmp	al,2
	je	mov_sreg_mem16
	cmp	al,4
	je	mov_sreg_mem32
	jmp	invalid_operand_size
      mov_sreg_mem32:
	call	operand_32bit_prefix
	jmp	mov_sreg_mem_size_ok
      mov_sreg_mem16:
	call	operand_16bit_prefix
      mov_sreg_mem_size_ok:
	mov	[base_code],8Eh
	call	store_instruction
	jmp	instruction_assembled
      mov_creg:
	mov	dl,22h
	jmp	mov_xrx
      mov_dreg:
	mov	dl,23h
      mov_xrx:
	and	al,111b
	mov	bh,al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	mov	bl,11000000b
	or	bl,al
	mov	al,bh
	shl	al,3
	or	bl,al
	mov	al,0Fh
	mov	ah,dl
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
cmov_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	je	cmov_reg_mem
	cmp	al,10h
	jne	invalid_operand
      cmov_reg_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	al,ah
	cmp	al,2
	je	cmov_reg_reg_16bit
	cmp	al,4
	je	cmov_reg_reg_32bit
	jmp	invalid_operand_size
      cmov_reg_reg_32bit:
	call	operand_32bit_prefix
	jmp	cmov_reg_reg_store
      cmov_reg_reg_16bit:
	call	operand_16bit_prefix
      cmov_reg_reg_store:
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      cmov_reg_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	cmov_reg_mem_16bit
	cmp	al,4
	je	cmov_reg_mem_32bit
	jmp	invalid_operand_size
      cmov_reg_mem_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      cmov_reg_mem_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
test_instruction:
	mov	[base_code],84h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	test_reg
	cmp	al,'['
	jne	invalid_operand
      test_mem:
	call	get_address
	push	edx bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	je	test_mem_imm
	cmp	al,10h
	jne	invalid_operand
      test_mem_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	al,ah
	cmp	al,1
	je	test_mem_reg_8bit
	cmp	al,2
	je	test_mem_reg_16bit
	cmp	al,4
	je	test_mem_reg_32bit
	jmp	invalid_operand_size
      test_mem_reg_8bit:
	call	store_instruction
	jmp	instruction_assembled
      test_mem_reg_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      test_mem_reg_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      test_mem_imm:
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
	pop	cx bx edx
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      test_mem_imm_16bit:
	call	get_word_value
	mov	word [value],ax
	mov	[postbyte_register],0
	mov	[base_code],0F7h
	call	operand_16bit_prefix
	pop	cx bx edx
	call	store_instruction
	mov	ax,word [value]
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      test_mem_imm_32bit:
	call	get_dword_value
	mov	dword [value],eax
	mov	[postbyte_register],0
	mov	[base_code],0F7h
	call	operand_32bit_prefix
	pop	cx bx edx
	call	store_instruction
	mov	eax,dword [value]
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      test_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	je	test_reg_mem
	cmp	al,'('
	je	test_reg_imm
	cmp	al,10h
	jne	invalid_operand
      test_reg_reg:
	lods	byte [esi]
	call	convert_register
	shl	al,3
	mov	bl,[postbyte_register]
	or	bl,al
	or	bl,11000000b
	mov	al,ah
	cmp	al,1
	je	test_reg_reg_8bit
	cmp	al,2
	je	test_reg_reg_16bit
	cmp	al,4
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
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      test_reg_imm:
	mov	al,[operand_size]
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
	stos	word [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      test_al_imm:
	mov	al,0A8h
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      test_reg_imm_16bit:
	call	get_word_value
	mov	dx,ax
	call	operand_16bit_prefix
	mov	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	bl,bl
	jz	test_ax_imm
	or	ah,bl
	mov	al,0F7h
	stos	word [edi]
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      test_ax_imm:
	mov	al,0A9h
	stos	byte [edi]
	mov	ax,dx
	stos	word [edi]
	jmp	instruction_assembled
      test_reg_imm_32bit:
	call	get_dword_value
	mov	edx,eax
	call	operand_32bit_prefix
	mov	ah,11000000b
	mov	bl,[postbyte_register]
	and	bl,111b
	or	bl,bl
	jz	test_eax_imm
	or	ah,bl
	mov	al,0F7h
	stos	word [edi]
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      test_eax_imm:
	mov	al,0A9h
	stos	byte [edi]
	mov	eax,edx
	stos	dword [edi]
	jmp	instruction_assembled
      test_reg_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	test_reg_mem_8bit
	cmp	al,2
	je	test_reg_mem_16bit
	cmp	al,4
	je	test_reg_mem_32bit
	jmp	invalid_operand_size
      test_reg_mem_8bit:
	call	store_instruction
	jmp	instruction_assembled
      test_reg_mem_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      test_reg_mem_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
xchg_instruction:
	mov	[base_code],86h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	xchg_reg
	cmp	al,'['
	jne	invalid_operand
      xchg_mem:
	call	get_address
	push	edx bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	test_mem_reg
	jmp	invalid_operand
      xchg_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	je	test_reg_mem
	cmp	al,10h
	jne	invalid_operand
      xchg_reg_reg:
	lods	byte [esi]
	call	convert_register
	mov	bh,al
	mov	bl,[postbyte_register]
	shl	[postbyte_register],3
	or	al,11000000b
	or	[postbyte_register],al
	mov	al,ah
	cmp	al,1
	je	xchg_reg_reg_8bit
	cmp	al,2
	je	xchg_reg_reg_16bit
	cmp	al,4
	je	xchg_reg_reg_32bit
	jmp	invalid_operand_size
      xchg_reg_reg_32bit:
	call	operand_32bit_prefix
	or	bh,bh
	jz	xchg_ax_reg
	xchg	bh,bl
	or	bh,bh
	jz	xchg_ax_reg
	inc	[base_code]
	jmp	xchg_reg_reg_8bit
      xchg_reg_reg_16bit:
	call	operand_16bit_prefix
	or	bh,bh
	jz	xchg_ax_reg
	xchg	bh,bl
	or	bh,bh
	jz	xchg_ax_reg
	inc	[base_code]
      xchg_reg_reg_8bit:
	mov	al,[base_code]
	mov	ah,[postbyte_register]
	stos	word [edi]
	jmp	instruction_assembled
      xchg_ax_reg:
	mov	al,90h
	add	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
push_instruction:
	mov	[extended_code],al
      push_next:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	push_reg
	cmp	al,'('
	je	push_imm
	cmp	al,'['
	jne	invalid_operand
      push_mem:
	call	get_address
	mov	al,[operand_size]
	mov	ah,[extended_code]
	cmp	al,2
	je	push_mem_16bit
	cmp	al,4
	je	push_mem_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	ah,2
	je	push_mem_16bit
	cmp	ah,4
	je	push_mem_32bit
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      push_mem_16bit:
	cmp	ah,4
	je	invalid_operand_size
	call	operand_16bit_prefix
	mov	[base_code],0FFh
	mov	[postbyte_register],110b
	call	store_instruction
	jmp	push_done
      push_mem_32bit:
	cmp	ah,2
	je	invalid_operand_size
	call	operand_32bit_prefix
	mov	[base_code],0FFh
	mov	[postbyte_register],110b
	call	store_instruction
	jmp	push_done
      push_reg:
	lods	byte [esi]
	cmp	al,60h
	jae	push_sreg
	call	convert_register
	mov	dl,al
	add	dl,50h
	mov	al,ah
	mov	ah,[extended_code]
	cmp	al,2
	je	push_reg_16bit
	cmp	al,4
	je	push_reg_32bit
	jmp	invalid_operand_size
      push_reg_16bit:
	cmp	ah,4
	je	invalid_operand_size
	call	operand_16bit_prefix
	mov	al,dl
	stos	byte [edi]
	jmp	push_done
      push_reg_32bit:
	cmp	ah,2
	je	invalid_operand_size
	call	operand_32bit_prefix
	mov	al,dl
	stos	byte [edi]
	jmp	push_done
      push_sreg:
	mov	bl,[operand_size]
	mov	bh,[extended_code]
	cmp	bl,4
	je	push_sreg32
	cmp	bl,2
	je	push_sreg16
	or	bl,bl
	jnz	invalid_operand_size
	cmp	bh,4
	je	push_sreg32
	cmp	bh,2
	je	push_sreg16
	jmp	push_sreg_store
      push_sreg16:
	cmp	bh,4
	je	invalid_operand_size
	mov	bl,al
	call	operand_16bit_prefix
	mov	al,bl
	jmp	push_sreg_store
      push_sreg32:
	cmp	bh,2
	je	invalid_operand_size
	mov	bl,al
	call	operand_32bit_prefix
	mov	al,bl
      push_sreg_store:
	cmp	al,70h
	jae	invalid_operand
	sub	al,61h
	cmp	al,4
	jae	push_sreg_386
	shl	al,3
	add	al,6
	stos	byte [edi]
	jmp	push_done
      push_sreg_386:
	sub	al,4
	shl	al,3
	mov	ah,0A0h
	add	ah,al
	mov	al,0Fh
	stos	word [edi]
	jmp	push_done
      push_imm:
	mov	al,[operand_size]
	mov	ah,[extended_code]
	or	al,al
	je	push_imm_size_ok
	or	ah,ah
	je	push_imm_size_ok
	cmp	al,ah
	jne	invalid_operand_size
      push_imm_size_ok:
	cmp	al,2
	je	push_imm_16bit
	cmp	al,4
	je	push_imm_32bit
	cmp	ah,2
	je	push_imm_optimized_16bit
	cmp	ah,4
	je	push_imm_optimized_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[code_type],16
	je	push_imm_optimized_16bit
      push_imm_optimized_32bit:
	call	get_dword_value
	mov	edx,eax
	cmp	[value_type],0
	jne	push_imm_32bit_forced
	cmp	eax,-80h
	jl	push_imm_32bit_forced
	cmp	eax,80h
	jge	push_imm_32bit_forced
	call	operand_32bit_prefix
      push_imm_8bit:
	mov	ah,al
	mov	al,6Ah
	stos	word [edi]
	jmp	push_done
      push_imm_optimized_16bit:
	call	get_word_value
	mov	dx,ax
	cmp	[value_type],0
	jne	push_imm_16bit_forced
	cmp	ax,-80h
	jl	push_imm_16bit_forced
	cmp	ax,80h
	jge	push_imm_16bit_forced
	call	operand_16bit_prefix
	jmp	push_imm_8bit
      push_imm_16bit:
	call	get_word_value
	mov	dx,ax
	call	operand_16bit_prefix
      push_imm_16bit_forced:
	mov	al,68h
	stos	byte [edi]
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	push_done
      push_imm_32bit:
	call	get_dword_value
	mov	edx,eax
	call	operand_32bit_prefix
      push_imm_32bit_forced:
	mov	al,68h
	stos	byte [edi]
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
      push_done:
	lods	byte [esi]
	dec	esi
	cmp	al,0Fh
	je	instruction_assembled
	or	al,al
	jz	instruction_assembled
	mov	[operand_size],0
	mov	[forced_size],0
	jmp	push_next
pop_instruction:
	mov	[extended_code],al
      pop_next:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	pop_reg
	cmp	al,'['
	jne	invalid_operand
      pop_mem:
	call	get_address
	mov	al,[operand_size]
	mov	ah,[extended_code]
	cmp	al,2
	je	pop_mem_16bit
	cmp	al,4
	je	pop_mem_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	ah,2
	je	pop_mem_16bit
	cmp	ah,4
	je	pop_mem_32bit
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      pop_mem_16bit:
	cmp	ah,4
	je	invalid_operand_size
	call	operand_16bit_prefix
	mov	[base_code],08Fh
	mov	[postbyte_register],0
	call	store_instruction
	jmp	pop_done
      pop_mem_32bit:
	cmp	ah,2
	je	invalid_operand_size
	call	operand_32bit_prefix
	mov	[base_code],08Fh
	mov	[postbyte_register],0
	call	store_instruction
	jmp	pop_done
      pop_reg:
	lods	byte [esi]
	cmp	al,60h
	jae	pop_sreg
	call	convert_register
	mov	dl,al
	add	dl,58h
	mov	al,ah
	mov	ah,[extended_code]
	cmp	al,2
	je	pop_reg_16bit
	cmp	al,4
	je	pop_reg_32bit
	jmp	invalid_operand_size
      pop_reg_16bit:
	cmp	ah,4
	je	invalid_operand_size
	call	operand_16bit_prefix
	mov	al,dl
	stos	byte [edi]
	jmp	pop_done
      pop_reg_32bit:
	cmp	ah,2
	je	invalid_operand_size
	call	operand_32bit_prefix
	mov	al,dl
	stos	byte [edi]
	jmp	pop_done
      pop_sreg:
	mov	bl,[operand_size]
	mov	bh,[extended_code]
	cmp	bl,4
	je	pop_sreg32
	cmp	bl,2
	je	pop_sreg16
	or	bl,bl
	jnz	invalid_operand_size
	cmp	bh,4
	je	pop_sreg32
	cmp	bh,2
	je	pop_sreg16
	jmp	pop_sreg_store
      pop_sreg16:
	cmp	bh,4
	je	invalid_operand_size
	mov	bl,al
	call	operand_16bit_prefix
	mov	al,bl
	jmp	pop_sreg_store
      pop_sreg32:
	cmp	bh,2
	je	invalid_operand_size
	mov	bl,al
	call	operand_32bit_prefix
	mov	al,bl
      pop_sreg_store:
	cmp	al,70h
	jae	invalid_operand
	sub	al,61h
	cmp	al,1
	je	illegal_instruction
	cmp	al,4
	jae	pop_sreg_386
	shl	al,3
	add	al,7
	stos	byte [edi]
	jmp	pop_done
      pop_sreg_386:
	sub	al,4
	shl	al,3
	mov	ah,0A1h
	add	ah,al
	mov	al,0Fh
	stos	word [edi]
      pop_done:
	lods	byte [esi]
	dec	esi
	cmp	al,0Fh
	je	instruction_assembled
	or	al,al
	jz	instruction_assembled
	mov	[operand_size],0
	mov	[forced_size],0
	jmp	pop_next
inc_instruction:
	mov	[base_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	inc_reg
	cmp	al,'['
	je	inc_mem
	jne	invalid_operand
      inc_mem:
	call	get_address
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
	mov	al,0FEh
	xchg	al,[base_code]
	mov	[postbyte_register],al
	call	store_instruction
	jmp	instruction_assembled
      inc_mem_16bit:
	call	operand_16bit_prefix
	mov	al,0FFh
	xchg	al,[base_code]
	mov	[postbyte_register],al
	call	store_instruction
	jmp	instruction_assembled
      inc_mem_32bit:
	call	operand_32bit_prefix
	mov	al,0FFh
	xchg	al,[base_code]
	mov	[postbyte_register],al
	call	store_instruction
	jmp	instruction_assembled
      inc_reg:
	lods	byte [esi]
	call	convert_register
	mov	dl,al
	shr	al,4
	mov	al,ah
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
	stos	word [edi]
	jmp	instruction_assembled
      inc_reg_16bit:
	call	operand_16bit_prefix
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      inc_reg_32bit:
	call	operand_32bit_prefix
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
arpl_instruction:
	mov	[base_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	arpl_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	cmp	ah,2
	jne	invalid_operand_size
	mov	[base_code],63h
	call	store_instruction
	jmp	instruction_assembled
      arpl_reg:
	lods	byte [esi]
	call	convert_register
	cmp	ah,2
	jne	invalid_operand_size
	mov	dl,al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	ah,2
	jne	invalid_operand_size
	mov	ah,al
	shl	ah,3
	or	ah,dl
	or	ah,11000000b
	mov	al,63h
	stos	word [edi]
	jmp	instruction_assembled
bound_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	bound_16bit
	cmp	al,4
	je	bound_32bit
	jmp	invalid_operand_size
      bound_32bit:
	call	operand_32bit_prefix
	mov	[base_code],62h
	call	store_instruction
	jmp	instruction_assembled
      bound_16bit:
	call	operand_16bit_prefix
	mov	[base_code],62h
	call	store_instruction
	jmp	instruction_assembled
set_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	set_reg
	cmp	al,'['
	jne	invalid_operand
      set_mem:
	call	get_address
	cmp	[operand_size],1
	ja	invalid_operand_size
	mov	[postbyte_register],0
	call	store_instruction
	jmp	instruction_assembled
      set_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	cmp	ah,1
	jne	invalid_operand_size
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,11000000b
	or	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
ret_instruction_16bit:
	mov	ah,al
	call	operand_16bit_prefix
	mov	al,ah
	jmp	ret_instruction
ret_instruction_32bit:
	mov	ah,al
	call	operand_32bit_prefix
	mov	al,ah
ret_instruction:
	mov	[base_code],al
	lods	byte [esi]
	dec	esi
	or	al,al
	jz	simple_ret
	cmp	al,0Fh
	je	simple_ret
	lods	byte [esi]
	call	get_size_operator
	or	ah,ah
	jz	ret_imm
	cmp	ah,2
	je	ret_imm
	jmp	invalid_operand_size
      ret_imm:
	cmp	al,'('
	jne	invalid_operand
	call	get_word_value
	cmp	[next_pass_needed],0
	jne	ret_imm_ok
	cmp	[value_type],0
	jne	invalid_use_of_symbol
      ret_imm_ok:
	mov	dx,ax
	mov	al,[base_code]
	stos	byte [edi]
	mov	ax,dx
	stos	word [edi]
	jmp	instruction_assembled
      simple_ret:
	mov	al,[base_code]
	inc	al
	stos	byte [edi]
	jmp	instruction_assembled
lea_instruction:
	mov	[base_code],8Dh
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	al,[operand_size]
	push	eax
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	cmp	al,2
	je	lea_16bit
	cmp	al,4
	je	lea_32bit
	jmp	invalid_operand_size
      lea_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      lea_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
ls_instruction:
	or	al,al
	jz	les_instruction
	cmp	al,3
	jz	lds_instruction
	add	al,0B0h
	mov	[extended_code],al
	mov	[base_code],0Fh
	jmp	ls_code_ok
      les_instruction:
	mov	[base_code],0C4h
	jmp	ls_code_ok
      lds_instruction:
	mov	[base_code],0C5h
      ls_code_ok:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	add	[operand_size],2
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,4
	je	ls_16bit
	cmp	al,6
	je	ls_32bit
	jmp	invalid_operand_size
      ls_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	cmp	[operand_size],0
	je	instruction_assembled
	cmp	[operand_size],4
	jne	invalid_operand_size
	jmp	instruction_assembled
      ls_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	cmp	[operand_size],0
	je	instruction_assembled
	cmp	[operand_size],6
	jne	invalid_operand_size
	jmp	instruction_assembled
enter_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	ah,2
	je	enter_imm16_size_ok
	or	ah,ah
	jnz	invalid_operand_size
      enter_imm16_size_ok:
	cmp	al,'('
	jne	invalid_operand
	call	get_word_value
	cmp	[next_pass_needed],0
	jne	enter_imm16_ok
	cmp	[value_type],0
	jne	invalid_use_of_symbol
      enter_imm16_ok:
	push	eax
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	ah,1
	je	enter_imm8_size_ok
	or	ah,ah
	jnz	invalid_operand_size
      enter_imm8_size_ok:
	cmp	al,'('
	jne	invalid_operand
	call	get_byte_value
	mov	dl,al
	pop	ebx
	mov	al,0C8h
	stos	byte [edi]
	mov	ax,bx
	stos	word [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
sh_instruction:
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	sh_reg
	cmp	al,'['
	jne	invalid_operand
      sh_mem:
	call	get_address
	push	edx bx cx
	mov	al,[operand_size]
	push	eax
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	je	sh_mem_imm
	cmp	al,10h
	jne	invalid_operand
      sh_mem_reg:
	lods	byte [esi]
	cmp	al,11h
	jne	invalid_operand
	pop	eax cx bx edx
	cmp	al,1
	je	sh_mem_cl_8bit
	cmp	al,2
	je	sh_mem_cl_16bit
	cmp	al,4
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
	jmp	instruction_assembled
      sh_mem_cl_16bit:
	mov	[base_code],0D3h
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      sh_mem_cl_32bit:
	mov	[base_code],0D3h
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      sh_mem_imm:
	mov	al,[operand_size]
	or	al,al
	jz	sh_mem_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      sh_mem_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	pop	eax cx bx edx
	cmp	al,1
	je	sh_mem_imm_8bit
	cmp	al,2
	je	sh_mem_imm_16bit
	cmp	al,4
	je	sh_mem_imm_32bit
	or	al,al
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
	stos	byte [edi]
	jmp	instruction_assembled
      sh_mem_1_8bit:
	mov	[base_code],0D0h
	call	store_instruction
	jmp	instruction_assembled
      sh_mem_imm_16bit:
	cmp	byte [value],1
	je	sh_mem_1_16bit
	mov	[base_code],0C1h
	call	operand_16bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      sh_mem_1_16bit:
	mov	[base_code],0D1h
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      sh_mem_imm_32bit:
	cmp	byte [value],1
	je	sh_mem_1_32bit
	mov	[base_code],0C1h
	call	operand_32bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      sh_mem_1_32bit:
	mov	[base_code],0D1h
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      sh_reg:
	lods	byte [esi]
	call	convert_register
	shl	[postbyte_register],3
	or	al,11000000b
	or	[postbyte_register],al
	mov	al,ah
	push	eax
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	je	sh_reg_imm
	cmp	al,10h
	jne	invalid_operand
      sh_reg_reg:
	lods	byte [esi]
	cmp	al,11h
	jne	invalid_operand
	pop	eax
	mov	bl,[postbyte_register]
	cmp	al,1
	je	sh_reg_cl_8bit
	cmp	al,2
	je	sh_reg_cl_16bit
	cmp	al,4
	je	sh_reg_cl_32bit
	jmp	invalid_operand_size
      sh_reg_cl_8bit:
	mov	al,0D2h
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      sh_reg_cl_16bit:
	call	operand_16bit_prefix
	mov	al,0D3h
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      sh_reg_cl_32bit:
	call	operand_32bit_prefix
	mov	al,0D3h
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      sh_reg_imm:
	mov	al,[operand_size]
	or	al,al
	jz	sh_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      sh_reg_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	pop	eax
	mov	bl,[postbyte_register]
	cmp	al,1
	je	sh_reg_imm_8bit
	cmp	al,2
	je	sh_reg_imm_16bit
	cmp	al,4
	je	sh_reg_imm_32bit
	jmp	invalid_operand_size
      sh_reg_imm_8bit:
	cmp	byte [value],1
	je	sh_reg_1_8bit
	mov	al,0C0h
	stos	byte [edi]
	mov	al,bl
	mov	ah,byte [value]
	stos	word [edi]
	jmp	instruction_assembled
      sh_reg_1_8bit:
	mov	al,0D0h
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      sh_reg_imm_16bit:
	cmp	byte [value],1
	je	sh_reg_1_16bit
	call	operand_16bit_prefix
	mov	al,0C1h
	stos	byte [edi]
	mov	al,bl
	mov	ah,byte [value]
	stos	word [edi]
	jmp	instruction_assembled
      sh_reg_1_16bit:
	call	operand_16bit_prefix
	mov	al,0D1h
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      sh_reg_imm_32bit:
	cmp	byte [value],1
	je	sh_reg_1_32bit
	call	operand_32bit_prefix
	mov	al,0C1h
	stos	byte [edi]
	mov	al,bl
	mov	ah,byte [value]
	stos	word [edi]
	jmp	instruction_assembled
      sh_reg_1_32bit:
	call	operand_32bit_prefix
	mov	al,0D1h
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
shd_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	shd_reg
	cmp	al,'['
	jne	invalid_operand
      shd_mem:
	call	get_address
	push	edx bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	xor	al,al
	xchg	al,[operand_size]
	push	eax
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	je	shd_mem_reg_imm
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,11h
	jne	invalid_operand
	pop	eax cx bx edx
	cmp	al,2
	je	shd_mem_reg_cl_16bit
	cmp	al,4
	je	shd_mem_reg_cl_32bit
	jmp	invalid_operand_size
      shd_mem_reg_cl_16bit:
	call	operand_16bit_prefix
	inc	[extended_code]
	call	store_instruction
	jmp	instruction_assembled
      shd_mem_reg_cl_32bit:
	call	operand_32bit_prefix
	inc	[extended_code]
	call	store_instruction
	jmp	instruction_assembled
      shd_mem_reg_imm:
	mov	al,[operand_size]
	or	al,al
	jz	shd_mem_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      shd_mem_reg_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	pop	eax cx bx edx
	cmp	al,2
	je	shd_mem_reg_imm_16bit
	cmp	al,4
	je	shd_mem_reg_imm_32bit
	jmp	invalid_operand_size
      shd_mem_reg_imm_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      shd_mem_reg_imm_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      shd_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	al,3
	or	bl,al
	or	bl,11000000b
	mov	al,ah
	push	ax bx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	je	shd_reg_reg_imm
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,11h
	jne	invalid_operand
	pop	bx ax
	cmp	al,2
	je	shd_reg_reg_cl_16bit
	cmp	al,4
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
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      shd_reg_reg_imm:
	mov	al,[operand_size]
	or	al,al
	jz	shd_reg_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      shd_reg_reg_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	pop	bx ax
	cmp	al,2
	je	shd_reg_reg_imm_16bit
	cmp	al,4
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
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
movx_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	mov	al,ah
	cmp	al,2
	je	movx_16bit
	cmp	al,4
	je	movx_32bit
	jmp	invalid_operand_size
      movx_16bit:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	movx_16bit_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	movx_16bit_mem_8bit
	or	al,al
	jnz	invalid_operand_size
      movx_16bit_mem_8bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      movx_16bit_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	cmp	ah,1
	jne	invalid_operand_size
	call	operand_16bit_prefix
	mov	al,0Fh
	stos	byte [edi]
	mov	al,[extended_code]
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      movx_32bit:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	movx_32bit_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	movx_32bit_mem_8bit
	cmp	al,2
	je	movx_32bit_mem_16bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[current_pass],0
	jne	operand_size_not_specified
	cmp	[next_pass_needed],0
	je	operand_size_not_specified
      movx_32bit_mem_8bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      movx_32bit_mem_16bit:
	inc	[extended_code]
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      movx_32bit_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	al,ah
	cmp	al,1
	je	movx_32bit_reg_8bit
	cmp	al,2
	je	movx_32bit_reg_16bit
	jmp	invalid_operand_size
      movx_32bit_reg_8bit:
	call	operand_32bit_prefix
	mov	al,0Fh
	stos	byte [edi]
	mov	al,[extended_code]
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      movx_32bit_reg_16bit:
	call	operand_32bit_prefix
	mov	al,0Fh
	stos	byte [edi]
	mov	al,[extended_code]
	inc	al
	stos	byte [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
bt_instruction:
	mov	[postbyte_register],al
	shl	al,3
	add	al,83h
	mov	[extended_code],al
	mov	[base_code],0Fh
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	bt_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	push	eax bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	cmp	byte [esi],'('
	je	bt_mem_imm
	cmp	byte [esi],11h
	jne	bt_mem_reg
	cmp	byte [esi+2],'('
	je	bt_mem_imm
      bt_mem_reg:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	al,ah
	cmp	al,2
	je	bt_mem_reg_16bit
	cmp	al,4
	je	bt_mem_reg_32bit
	jmp	invalid_operand_size
      bt_mem_reg_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      bt_mem_reg_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      bt_mem_imm:
	xor	al,al
	xchg	al,[operand_size]
	push	eax
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	jne	invalid_operand
	mov	al,[operand_size]
	or	al,al
	jz	bt_mem_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      bt_mem_imm_size_ok:
	mov	[extended_code],0BAh
	call	get_byte_value
	mov	byte [value],al
	pop	eax
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
	pop	cx bx edx
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      bt_mem_imm_32bit:
	call	operand_32bit_prefix
	pop	cx bx edx
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      bt_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	cmp	byte [esi],'('
	je	bt_reg_imm
	cmp	byte [esi],11h
	jne	bt_reg_reg
	cmp	byte [esi+2],'('
	je	bt_reg_imm
      bt_reg_reg:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	al,3
	or	bl,al
	or	bl,11000000b
	mov	al,ah
	cmp	al,2
	je	bt_reg_reg_16bit
	cmp	al,4
	je	bt_reg_reg_32bit
	jmp	invalid_operand_size
      bt_reg_reg_16bit:
	call	operand_16bit_prefix
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      bt_reg_reg_32bit:
	call	operand_32bit_prefix
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      bt_reg_imm:
	xor	al,al
	xchg	al,[operand_size]
	push	eax
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	jne	invalid_operand
	mov	al,[operand_size]
	or	al,al
	jz	bt_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      bt_reg_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	pop	eax
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
	mov	ax,0BA0Fh
	stos	word [edi]
	mov	al,11000000b
	or	al,[postbyte_register]
	mov	ah,[extended_code]
	sub	ah,83h
	or	al,ah
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
bs_instruction:
	mov	[extended_code],al
	mov	[base_code],0Fh
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	bs_reg_reg
	cmp	al,'['
	jne	invalid_argument
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	bs_reg_mem_16bit
	cmp	al,4
	je	bs_reg_mem_32bit
	jmp	invalid_operand_size
      bs_reg_mem_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      bs_reg_mem_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      bs_reg_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	al,ah
	cmp	al,2
	je	bs_reg_reg_16bit
	cmp	al,4
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
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled

pm_word_instruction:
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[base_code],0Fh
	mov	[extended_code],ah
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	pm_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	.store
	or	al,al
	jnz	invalid_operand_size
      .store:
	call	store_instruction
	jmp	instruction_assembled
      pm_reg:
	lods	byte [esi]
	call	convert_register
	cmp	ah,2
	jne	invalid_operand_size
	mov	bl,al
	mov	al,0Fh
	mov	ah,[extended_code]
	stos	word [edi]
	mov	al,[postbyte_register]
	shl	al,3
	or	al,bl
	or	al,11000000b
	stos	byte [edi]
	jmp	instruction_assembled
pm_pword_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],1
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,6
	je	.store
	or	al,al
	jnz	invalid_operand_size
      .store:
	call	store_instruction
	jmp	instruction_assembled

imul_instruction:
	mov	[base_code],0F6h
	mov	[postbyte_register],5
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	imul_reg
	cmp	al,'['
	jne	invalid_operand
      imul_mem:
	call	get_address
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
	jmp	instruction_assembled
      imul_mem_16bit:
	call	operand_16bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      imul_mem_32bit:
	call	operand_32bit_prefix
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      imul_reg:
	lods	byte [esi]
	call	convert_register
	cmp	byte [esi],','
	je	imul_reg_
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
	stos	word [edi]
	jmp	instruction_assembled
      imul_reg_16bit:
	call	operand_16bit_prefix
	mov	ah,bl
	mov	al,0F7h
	stos	word [edi]
	jmp	instruction_assembled
      imul_reg_32bit:
	call	operand_32bit_prefix
	mov	ah,bl
	mov	al,0F7h
	stos	word [edi]
	jmp	instruction_assembled
      imul_reg_:
	mov	[postbyte_register],al
	inc	esi
	cmp	byte [esi],'('
	je	imul_reg_imm
	cmp	byte [esi],11h
	jne	imul_reg__
	cmp	byte [esi+2],'('
	je	imul_reg_imm
      imul_reg__:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	imul_reg_reg
	cmp	al,'['
	je	imul_reg_mem
	jne	invalid_operand
      imul_reg_mem:
	call	get_address
	push	edx bx cx
	cmp	byte [esi],','
	je	imul_reg_mem_imm
	mov	al,[operand_size]
	cmp	al,2
	je	imul_reg_mem_16bit
	cmp	al,4
	je	imul_reg_mem_32bit
	jmp	invalid_operand_size
      imul_reg_mem_16bit:
	call	operand_16bit_prefix
	jmp	imul_reg_mem_store
      imul_reg_mem_32bit:
	call	operand_32bit_prefix
      imul_reg_mem_store:
	pop	cx bx edx
	mov	[base_code],0Fh
	mov	[extended_code],0AFh
	call	store_instruction
	jmp	instruction_assembled
      imul_reg_mem_imm:
	inc	esi
	xor	cl,cl
	xchg	cl,[operand_size]
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	jne	invalid_operand
	mov	al,[operand_size]
	mov	[operand_size],cl
	cmp	al,1
	je	imul_reg_mem_imm_8bit
	cmp	al,2
	je	imul_reg_mem_imm_16bit
	cmp	al,4
	je	imul_reg_mem_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	cl,2
	je	imul_reg_mem_imm_16bit
	cmp	cl,4
	je	imul_reg_mem_imm_32bit
	jmp	invalid_operand_size
      imul_reg_mem_imm_8bit:
	call	get_byte_value
	mov	byte [value],al
	pop	cx bx edx
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
	stos	byte [edi]
	jmp	instruction_assembled
      imul_reg_mem_32bit_imm_8bit:
	call	operand_32bit_prefix
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      imul_reg_mem_imm_16bit:
	call	get_word_value
	mov	word [value],ax
	pop	cx bx edx
	mov	[base_code],69h
	cmp	[operand_size],2
	jne	invalid_operand_size
	call	operand_16bit_prefix
	call	store_instruction
	mov	ax,word [value]
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      imul_reg_mem_imm_32bit:
	call	get_dword_value
	mov	dword [value],eax
	pop	cx bx edx
	mov	[base_code],69h
	cmp	[operand_size],4
	jne	invalid_operand_size
	call	operand_32bit_prefix
	call	store_instruction
	mov	eax,dword [value]
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      imul_reg_imm:
	mov	dl,[postbyte_register]
	mov	bl,dl
	dec	esi
	jmp	imul_reg_reg_imm
      imul_reg_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	mov	dl,al
	cmp	byte [esi],','
	je	imul_reg_reg_imm
	mov	al,ah
	cmp	al,2
	je	imul_reg_reg_16bit
	cmp	al,4
	je	imul_reg_reg_32bit
	jmp	invalid_operand_size
      imul_reg_reg_16bit:
	call	operand_16bit_prefix
	jmp	imul_reg_reg_store
      imul_reg_reg_32bit:
	call	operand_32bit_prefix
      imul_reg_reg_store:
	mov	ax,0AF0Fh
	stos	word [edi]
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stos	byte [edi]
	jmp	instruction_assembled
      imul_reg_reg_imm:
	inc	esi
	xor	cl,cl
	xchg	cl,[operand_size]
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	jne	invalid_operand
	mov	al,[operand_size]
	mov	[operand_size],cl
	cmp	al,1
	je	imul_reg_reg_imm_8bit
	cmp	al,2
	je	imul_reg_reg_imm_16bit
	cmp	al,4
	je	imul_reg_reg_imm_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	cl,2
	je	imul_reg_reg_imm_16bit
	cmp	cl,4
	je	imul_reg_reg_imm_32bit
	jmp	invalid_operand_size
      imul_reg_reg_imm_8bit:
	push	bx dx
	call	get_byte_value
	pop	dx bx
      imul_reg_reg_imm_8bit_store:
	mov	byte [value],al
	cmp	[operand_size],2
	je	imul_reg_reg_16bit_imm_8bit
	cmp	[operand_size],4
	je	imul_reg_reg_32bit_imm_8bit
	jmp	invalid_operand_size
      imul_reg_reg_16bit_imm_8bit:
	call	operand_16bit_prefix
	mov	al,6Bh
	stos	byte [edi]
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      imul_reg_reg_32bit_imm_8bit:
	call	operand_32bit_prefix
	mov	al,6Bh
	stos	byte [edi]
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      imul_reg_reg_imm_16bit:
	push	bx dx
	call	get_word_value
	pop	dx bx
	cmp	[value_type],0
	jne	imul_reg_reg_imm_16bit_forced
	cmp	ax,-80h
	jl	imul_reg_reg_imm_16bit_forced
	cmp	ax,80h
	jl	imul_reg_reg_imm_8bit_store
      imul_reg_reg_imm_16bit_forced:
	mov	word [value],ax
	call	operand_16bit_prefix
	mov	al,69h
	stos	byte [edi]
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stos	byte [edi]
	mov	ax,word [value]
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      imul_reg_reg_imm_32bit:
	push	bx dx
	call	get_dword_value
	pop	dx bx
	cmp	[value_type],0
	jne	imul_reg_reg_imm_32bit_forced
	cmp	ax,-80h
	jl	imul_reg_reg_imm_32bit_forced
	cmp	ax,80h
	jl	imul_reg_reg_imm_8bit_store
      imul_reg_reg_imm_32bit_forced:
	mov	dword [value],eax
	call	operand_32bit_prefix
	mov	al,69h
	stos	byte [edi]
	mov	al,dl
	shl	bl,3
	or	al,bl
	or	al,11000000b
	stos	byte [edi]
	mov	eax,dword [value]
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
in_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	or	al,al
	jnz	invalid_operand
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	al,ah
	push	eax
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	je	in_imm
	cmp	al,10h
	je	in_reg
	jmp	invalid_operand
      in_reg:
	lods	byte [esi]
	cmp	al,22h
	jne	invalid_operand
	pop	eax
	cmp	al,1
	je	in_al_dx
	cmp	al,2
	je	in_ax_dx
	cmp	al,4
	je	in_eax_dx
	jmp	invalid_operand_size
      in_al_dx:
	mov	al,0ECh
	stos	byte [edi]
	jmp	instruction_assembled
      in_ax_dx:
	call	operand_16bit_prefix
	mov	al,0EDh
	stos	byte [edi]
	jmp	instruction_assembled
      in_eax_dx:
	call	operand_32bit_prefix
	mov	al,0EDh
	stos	byte [edi]
	jmp	instruction_assembled
      in_imm:
	mov	al,[operand_size]
	or	al,al
	jz	in_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      in_imm_size_ok:
	call	get_byte_value
	mov	dl,al
	pop	eax
	cmp	al,1
	je	in_al_imm
	cmp	al,2
	je	in_ax_imm
	cmp	al,4
	je	in_eax_imm
	jmp	invalid_operand_size
      in_al_imm:
	mov	al,0E4h
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      in_ax_imm:
	call	operand_16bit_prefix
	mov	al,0E5h
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      in_eax_imm:
	call	operand_32bit_prefix
	mov	al,0E5h
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
out_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	je	out_imm
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,22h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	or	al,al
	jnz	invalid_operand
	mov	al,ah
	cmp	al,1
	je	out_dx_al
	cmp	al,2
	je	out_dx_ax
	cmp	al,4
	je	out_dx_eax
	jmp	invalid_operand_size
      out_dx_al:
	mov	al,0EEh
	stos	byte [edi]
	jmp	instruction_assembled
      out_dx_ax:
	call	operand_16bit_prefix
	mov	al,0EFh
	stos	byte [edi]
	jmp	instruction_assembled
      out_dx_eax:
	call	operand_32bit_prefix
	mov	al,0EFh
	stos	byte [edi]
	jmp	instruction_assembled
      out_imm:
	mov	al,[operand_size]
	or	al,al
	jz	out_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      out_imm_size_ok:
	call	get_byte_value
	mov	byte [value],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	or	al,al
	jnz	invalid_operand
	mov	al,ah
	cmp	al,1
	je	out_imm_al
	cmp	al,2
	je	out_imm_ax
	cmp	al,4
	je	out_imm_eax
	jmp	invalid_operand_size
      out_imm_al:
	mov	al,0E6h
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      out_imm_ax:
	call	operand_16bit_prefix
	mov	al,0E7h
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      out_imm_eax:
	call	operand_32bit_prefix
	mov	al,0E7h
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
lar_instruction:
	mov	[extended_code],al
	mov	[base_code],0Fh
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	lar_reg_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,2
	je	lar_16bit
	cmp	al,4
	je	lar_32bit
	jmp	invalid_operand_size
      lar_16bit:
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      lar_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      lar_reg_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	al,ah
	cmp	al,2
	je	lar_reg_reg_16bit
	cmp	al,4
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
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
invlpg_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],1
	mov	[postbyte_register],7
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	call	store_instruction
	jmp	instruction_assembled
basic_486_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	basic_486_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	push	edx bx cx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	al,ah
	cmp	al,1
	je	basic_486_mem_reg_8bit
	cmp	al,2
	je	basic_486_mem_reg_16bit
	cmp	al,4
	je	basic_486_mem_reg_32bit
	jmp	invalid_operand_size
      basic_486_mem_reg_8bit:
	call	store_instruction
	jmp	instruction_assembled
      basic_486_mem_reg_16bit:
	call	operand_16bit_prefix
	inc	[extended_code]
	call	store_instruction
	jmp	instruction_assembled
      basic_486_mem_reg_32bit:
	call	operand_32bit_prefix
	inc	[extended_code]
	call	store_instruction
	jmp	instruction_assembled
      basic_486_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	shl	al,3
	or	bl,al
	or	bl,11000000b
	mov	al,ah
	cmp	al,1
	je	basic_486_reg_reg_8bit
	cmp	al,2
	je	basic_486_reg_reg_16bit
	cmp	al,4
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
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
bswap_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	mov	ah,al
	add	ah,0C8h
	call	operand_32bit_prefix
	mov	al,0Fh
	stos	word [edi]
	jmp	instruction_assembled
cmpxchg8b_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0C7h
	mov	[postbyte_register],1
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	cmp	al,8
	je	.store
	or	al,al
	jnz	invalid_operand_size
      .store:
	call	store_instruction
	jmp	instruction_assembled

conditional_jump:
	mov	[base_code],al
	lods	byte [esi]
	call	get_jump_operator
	cmp	[jump_type],2
	je	invalid_operand
	call	get_size_operator
	cmp	al,'('
	jne	invalid_operand
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_relative_offset
	sub	eax,2
	cmp	[next_pass_needed],0
	jne	conditional_jump_value_ok
	cmp	[value_type],1
	je	invalid_use_of_symbol
	ja	conditional_jump_32bit
      conditional_jump_value_ok:
	mov	bl,[operand_size]
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
	stos	word [edi]
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
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
	stos	word [edi]
	mov	eax,edx
	stos	word [edi]
	cmp	eax,10000h
	jge	jump_out_of_range
	cmp	eax,-10000h
	jl	jump_out_of_range
	jmp	instruction_assembled
      conditional_jump_8bit:
	mov	edx,eax
	mov	ah,al
	mov	al,[base_code]
	stos	word [edi]
	cmp	edx,80h
	jge	jump_out_of_range
	cmp	edx,-80h
	jl	jump_out_of_range
	jmp	instruction_assembled
      jump_out_of_range:
	cmp	[error_line],0
	jne	instruction_assembled
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],relative_jump_out_of_range
	jmp	instruction_assembled
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
	lods	byte [esi]
	call	get_jump_operator
	cmp	[jump_type],2
	je	invalid_operand
	call	get_size_operator
	cmp	al,'('
	jne	invalid_operand
	cmp	byte [esi],'.'
	je	invalid_value
	call	get_relative_offset
	cmp	[next_pass_needed],0
	jne	loop_value_ok
	cmp	[value_type],0
	jne	invalid_use_of_symbol
      loop_value_ok:
	mov	bl,[operand_size]
	cmp	bl,1
	je	loop_8bit
	or	bl,bl
	jnz	invalid_operand_size
      loop_8bit:
	sub	eax,2
	mov	edx,eax
	mov	al,[base_code]
	stos	byte [edi]
	mov	eax,edx
	stos	byte [edi]
	cmp	eax,80h
	jge	jump_out_of_range
	cmp	eax,-80h
	jl	jump_out_of_range
	jmp	instruction_assembled
call_instruction:
	mov	[postbyte_register],10b
	mov	[base_code],0E8h
	mov	[extended_code],9Ah
	jmp	process_jmp
jmp_instruction:
	mov	[postbyte_register],100b
	mov	[base_code],0E9h
	mov	[extended_code],0EAh
      process_jmp:
	lods	byte [esi]
	call	get_jump_operator
	call	get_size_operator
	cmp	al,10h
	je	jmp_reg
	cmp	al,'('
	je	jmp_imm
	cmp	al,'['
	jne	invalid_operand
      jmp_mem:
	call	get_address
	mov	[base_code],0FFh
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
	inc	[postbyte_register]
	call	store_instruction
	jmp	instruction_assembled
      jmp_mem_32bit:
	cmp	[jump_type],2
	je	jmp_mem_far_32bit
	cmp	[jump_type],1
	je	jmp_mem_near_32bit
	cmp	[code_type],16
	je	jmp_mem_far_32bit
      jmp_mem_near_32bit:
	call	operand_32bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      jmp_mem_far_32bit:
	call	operand_16bit_prefix
	inc	[postbyte_register]
	call	store_instruction
	jmp	instruction_assembled
      jmp_mem_16bit:
	cmp	[jump_type],2
	je	invalid_operand_size
	call	operand_16bit_prefix
	call	store_instruction
	jmp	instruction_assembled
      jmp_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	or	bl,11000000b
	mov	al,ah
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
	mov	al,[postbyte_register]
	shl	al,3
	or	bl,al
	mov	ah,bl
	mov	al,0FFh
	stos	word [edi]
	jmp	instruction_assembled
      jmp_reg_far32bit:
	call	operand_32bit_prefix
	mov	al,[postbyte_register]
	inc	al
	shl	al,3
	or	bl,al
	mov	ah,bl
	mov	al,0FFh
	stos	word [edi]
	jmp	instruction_assembled
      jmp_reg_16bit:
	cmp	[jump_type],2
	je	invalid_operand_size
	call	operand_16bit_prefix
	mov	al,[postbyte_register]
	shl	al,3
	or	bl,al
	mov	ah,bl
	mov	al,0FFh
	stos	word [edi]
	jmp	instruction_assembled
      jmp_imm:
	cmp	byte [esi],'.'
	je	invalid_value
	mov	ebx,esi
	dec	esi
	call	skip_symbol
	xchg	esi,ebx
	cmp	byte [ebx],':'
	je	jmp_far
	call	get_relative_offset
	cmp	[jump_type],2
	je	invalid_operand
	sub	eax,2
	cmp	[next_pass_needed],0
	jne	jmp_value_ok
	cmp	[value_type],1
	je	invalid_use_of_symbol
	ja	jmp_32bit
      jmp_value_ok:
	mov	bl,[operand_size]
	cmp	bl,1
	je	jmp_8bit
	cmp	bl,2
	je	jmp_16bit
	cmp	bl,4
	je	jmp_32bit
	or	bl,bl
	jnz	invalid_operand_size
	cmp	[base_code],0E9h
	jne	jmp_no8bit
	cmp	eax,80h
	jb	jmp_8bit
	cmp	eax,-80h
	jae	jmp_8bit
      jmp_no8bit:
	cmp	[code_type],32
	je	jmp_32bit
      jmp_16bit:
	dec	eax
	mov	edx,eax
	mov	ecx,edi
	call	operand_16bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	al,[base_code]
	stos	byte [edi]
	mov	eax,edx
	stos	word [edi]
	cmp	eax,10000h
	jge	jump_out_of_range
	cmp	eax,-10000h
	jl	jump_out_of_range
	jmp	instruction_assembled
      jmp_32bit:
	sub	eax,3
	mov	edx,eax
	mov	ecx,edi
	call	operand_32bit_prefix
	sub	edx,edi
	add	edx,ecx
	mov	al,[base_code]
	stos	byte [edi]
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      jmp_8bit:
	cmp	[base_code],0E9h
	jne	invalid_operand_size
	mov	edx,eax
	mov	ah,al
	mov	al,0EBh
	stos	word [edi]
	cmp	edx,80h
	jge	jump_out_of_range
	cmp	edx,-80h
	jl	jump_out_of_range
	jmp	instruction_assembled
      jmp_far:
	cmp	[jump_type],1
	je	invalid_operand
	call	get_word_value
	push	eax
	inc	esi
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	mov	al,[value_type]
	push	eax [symbol_identifier]
	cmp	byte [esi],'.'
	je	invalid_value
	mov	bl,[operand_size]
	cmp	bl,4
	je	jmp_far_16bit
	cmp	bl,6
	je	jmp_far_32bit
	or	bl,bl
	jnz	invalid_operand_size
	cmp	[code_type],32
	je	jmp_far_32bit
      jmp_far_16bit:
	call	get_word_value
	mov	ebx,eax
	call	operand_16bit_prefix
	mov	al,[extended_code]
	stos	byte [edi]
	mov	ax,bx
	call	mark_relocation
	stos	word [edi]
      jmp_far_segment:
	pop	[symbol_identifier] eax
	mov	[value_type],al
	pop	eax
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      jmp_far_32bit:
	call	get_dword_value
	mov	ebx,eax
	call	operand_32bit_prefix
	mov	al,[extended_code]
	stos	byte [edi]
	mov	eax,ebx
	call	mark_relocation
	stos	dword [edi]
	jmp	jmp_far_segment
ins_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
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
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
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
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,22h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
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
	cmp	[segment_register],4
	je	outs_segment_ok
	call	store_segment_prefix
      outs_segment_ok:
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
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
	jnz	invalid_address
	cmp	[segment_register],1
	ja	invalid_address
	push	ebx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	edx
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
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
	cmp	al,2
	je	movs_16bit
	cmp	al,4
	jne	invalid_address
	call	address_32bit_prefix
	jmp	movs_store
      movs_16bit:
	call	address_16bit_prefix
      movs_store:
	cmp	[segment_register],4
	je	movs_segment_ok
	call	store_segment_prefix
      movs_segment_ok:
	mov	al,0A4h
	mov	bl,[operand_size]
	cmp	bl,1
	je	simple_instruction
	inc	al
	cmp	bl,2
	je	simple_instruction_16bit
	cmp	bl,4
	je	simple_instruction_32bit
	or	bl,bl
	jz	operand_size_not_specified
	jmp	invalid_operand_size
lods_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
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
	cmp	[segment_register],4
	je	lods_segment_ok
	call	store_segment_prefix
      lods_segment_ok:
	mov	al,0ACh
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
stos_instruction:
	mov	[base_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
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
cmps_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
	jnz	invalid_address
	mov	al,[segment_register]
	push	ax bx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
	jnz	invalid_address
	pop	dx ax
	cmp	[segment_register],1
	ja	invalid_address
	mov	[segment_register],al
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
	cmp	al,2
	je	cmps_16bit
	cmp	al,4
	jne	invalid_address
	call	address_32bit_prefix
	jmp	cmps_store
      cmps_16bit:
	call	address_16bit_prefix
      cmps_store:
	cmp	[segment_register],4
	je	cmps_segment_ok
	call	store_segment_prefix
      cmps_segment_ok:
	mov	al,0A6h
	mov	bl,[operand_size]
	cmp	bl,1
	je	simple_instruction
	inc	al
	cmp	bl,2
	je	simple_instruction_16bit
	cmp	bl,4
	je	simple_instruction_32bit
	or	bl,bl
	jz	operand_size_not_specified
	jmp	invalid_operand_size
xlat_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	eax,eax
	jnz	invalid_address
	or	bl,ch
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
	call	store_segment_prefix_if_necessary
	mov	al,0D7h
	cmp	[operand_size],1
	jbe	simple_instruction
	jmp	invalid_operand_size

basic_fpu_instruction:
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	basic_fpu_streg
	cmp	al,'['
	je	basic_fpu_mem
	dec	esi
	mov	ah,[postbyte_register]
	cmp	ah,2
	jb	invalid_operand
	cmp	ah,3
	ja	invalid_operand
	shl	ah,3
	or	ah,11000001b
	mov	al,0D8h
	stos	word [edi]
	jmp	instruction_assembled
      basic_fpu_mem:
	call	get_address
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
	mov	[base_code],0D8h
	call	store_instruction
	jmp	instruction_assembled
      basic_fpu_mem_64bit:
	mov	[base_code],0DCh
	call	store_instruction
	jmp	instruction_assembled
      basic_fpu_streg:
	lods	byte [esi]
	call	convert_fpu_register
	mov	ah,[postbyte_register]
	cmp	ah,2
	je	basic_fpu_single_streg
	cmp	ah,3
	je	basic_fpu_single_streg
	or	al,al
	jz	basic_fpu_st0
	test	ah,110b
	jz	basic_fpu_streg_st0
	xor	ah,1
      basic_fpu_streg_st0:
	shl	ah,3
	or	al,ah
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_fpu_register
	or	al,al
	jnz	invalid_operand
	mov	ah,[postbyte_register]
	or	ah,11000000b
	mov	al,0DCh
	stos	word [edi]
	jmp	instruction_assembled
      basic_fpu_st0:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_fpu_register
	mov	ah,[postbyte_register]
	shl	ah,3
	or	ah,al
	or	ah,11000000b
	mov	al,0D8h
	stos	word [edi]
	jmp	instruction_assembled
      basic_fpu_single_streg:
	shl	ah,3
	or	ah,al
	or	ah,11000000b
	mov	al,0D8h
	stos	word [edi]
	jmp	instruction_assembled
simple_fpu_instruction:
	mov	ah,al
	or	ah,11000000b
	mov	al,0D9h
	stos	word [edi]
	jmp	instruction_assembled
fi_instruction:
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
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
	mov	[base_code],0DAh
	call	store_instruction
	jmp	instruction_assembled
      fi_mem_16bit:
	mov	[base_code],0DEh
	call	store_instruction
	jmp	instruction_assembled
fld_instruction:
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	fld_streg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
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
	mov	[base_code],0D9h
	call	store_instruction
	jmp	instruction_assembled
      fld_mem_64bit:
	mov	[base_code],0DDh
	call	store_instruction
	jmp	instruction_assembled
      fld_mem_80bit:
	mov	al,[postbyte_register]
	cmp	al,0
	je	.store
	dec	[postbyte_register]
	cmp	al,3
	je	.store
	jmp	invalid_operand_size
      .store:
	add	[postbyte_register],5
	mov	[base_code],0DBh
	call	store_instruction
	jmp	instruction_assembled
      fld_streg:
	lods	byte [esi]
	call	convert_fpu_register
	mov	ah,[postbyte_register]
	shl	ah,3
	or	ah,al
	or	ah,11000000b
	cmp	[postbyte_register],2
	jae	fst_streg
	mov	al,0D9h
	stos	word [edi]
	jmp	instruction_assembled
      fst_streg:
	mov	al,0DDh
	stos	word [edi]
	jmp	instruction_assembled
fild_instruction:
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
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
	mov	[base_code],0DBh
	call	store_instruction
	jmp	instruction_assembled
      fild_mem_16bit:
	mov	[base_code],0DFh
	call	store_instruction
	jmp	instruction_assembled
      fild_mem_64bit:
	mov	al,[postbyte_register]
	cmp	al,0
	je	.store
	dec	[postbyte_register]
	cmp	al,3
	je	.store
	jmp	invalid_operand_size
      .store:
	add	[postbyte_register],5
	mov	[base_code],0DFh
	call	store_instruction
	jmp	instruction_assembled
fbld_instruction:
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	fbld_mem_80bit
	cmp	al,10
	je	fbld_mem_80bit
	jmp	invalid_operand_size
      fbld_mem_80bit:
	mov	[base_code],0DFh
	call	store_instruction
	jmp	instruction_assembled
faddp_instruction:
	mov	[postbyte_register],al
	mov	ebx,esi
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	faddp_streg
	mov	esi,ebx
	mov	ah,[postbyte_register]
	shl	ah,3
	or	ah,11000001b
	mov	al,0DEh
	stos	word [edi]
	jmp	instruction_assembled
      faddp_streg:
	lods	byte [esi]
	call	convert_fpu_register
	mov	ah,[postbyte_register]
	shl	ah,3
	or	al,ah
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_fpu_register
	or	al,al
	jnz	invalid_operand
	mov	ah,[postbyte_register]
	or	ah,11000000b
	mov	al,0DEh
	stos	word [edi]
	jmp	instruction_assembled
fcompp_instruction:
	mov	ax,0D9DEh
	stos	word [edi]
	jmp	instruction_assembled
fucompp_instruction:
	mov	ax,0E9DAh
	stos	word [edi]
	jmp	instruction_assembled
fxch_instruction:
	mov	dx,01D9h
	jmp	fpu_single_operand
ffreep_instruction:
	mov	dx,00DFh
	jmp	fpu_single_operand
ffree_instruction:
	mov	dl,0DDh
	mov	dh,al
      fpu_single_operand:
	mov	ebx,esi
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	fpu_streg
	or	dh,dh
	jz	invalid_operand
	mov	esi,ebx
	shl	dh,3
	or	dh,11000001b
	mov	ax,dx
	stos	word [edi]
	jmp	instruction_assembled
      fpu_streg:
	lods	byte [esi]
	call	convert_fpu_register
	shl	dh,3
	or	dh,al
	or	dh,11000000b
	mov	ax,dx
	stos	word [edi]
	jmp	instruction_assembled
fstenv_instruction:
	mov	byte [edi],9Bh
	inc	edi
fldenv_instruction:
	mov	[base_code],0D9h
	jmp	fpu_mem
fsave_instruction:
	mov	byte [edi],9Bh
	inc	edi
fnsave_instruction:
	mov	[base_code],0DDh
      fpu_mem:
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	cmp	[operand_size],0
	jne	invalid_operand_size
	call	store_instruction
	jmp	instruction_assembled
fstcw_instruction:
	mov	byte [edi],9Bh
	inc	edi
fldcw_instruction:
	mov	[postbyte_register],al
	mov	[base_code],0D9h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	fldcw_mem_16bit
	cmp	al,2
	je	fldcw_mem_16bit
	jmp	invalid_operand_size
      fldcw_mem_16bit:
	call	store_instruction
	jmp	instruction_assembled
fstsw_instruction:
	mov	al,9Bh
	stos	byte [edi]
fnstsw_instruction:
	mov	[base_code],0DDh
	mov	[postbyte_register],7
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	fstsw_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	fstsw_mem_16bit
	cmp	al,2
	je	fstsw_mem_16bit
	jmp	invalid_operand_size
      fstsw_mem_16bit:
	call	store_instruction
	jmp	instruction_assembled
      fstsw_reg:
	lods	byte [esi]
	call	convert_register
	cmp	ax,0200h
	jne	invalid_operand
	mov	ax,0E0DFh
	stos	word [edi]
	jmp	instruction_assembled
finit_instruction:
	mov	byte [edi],9Bh
	inc	edi
fninit_instruction:
	mov	ah,al
	mov	al,0DBh
	stos	word [edi]
	jmp	instruction_assembled
fcmov_instruction:
	mov	dh,0DAh
	jmp	fcomi_streg
fcomi_instruction:
	mov	dh,0DBh
	jmp	fcomi_streg
fcomip_instruction:
	mov	dh,0DFh
      fcomi_streg:
	mov	dl,al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_fpu_register
	mov	ah,al
	cmp	byte [esi],','
	je	fcomi_st0_streg
	add	ah,dl
	mov	al,dh
	stos	word [edi]
	jmp	instruction_assembled
      fcomi_st0_streg:
	or	ah,ah
	jnz	invalid_operand
	inc	esi
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_fpu_register
	mov	ah,al
	add	ah,dl
	mov	al,dh
	stos	word [edi]
	jmp	instruction_assembled

movd_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	movd_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	test	[operand_size],not 4
	jnz	invalid_operand_size
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	call	make_mmx_prefix
	mov	[postbyte_register],al
	mov	[base_code],0Fh
	mov	[extended_code],7Eh
	call	store_mmx_instruction
	jmp	instruction_assembled
      movd_reg:
	lods	byte [esi]
	cmp	al,80h
	jae	movd_mmreg
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	mov	[operand_size],0
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	call	make_mmx_prefix
	pop	ebx
	shl	al,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	ax,7E0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      movd_mmreg:
	call	convert_mmx_register
	call	make_mmx_prefix
	push	eax
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	movd_mmreg_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	mov	[postbyte_register],al
	test	[operand_size],not 4
	jnz	invalid_operand_size
	mov	[base_code],0Fh
	mov	[extended_code],6Eh
	call	store_mmx_instruction
	jmp	instruction_assembled
      movd_mmreg_reg:
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	pop	ebx
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	ax,6E0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      make_mmx_prefix:
	mov	[mmx_prefix],0
	cmp	[operand_size],16
	jne	no_mmx_prefix
	mov	[mmx_prefix],66h
      no_mmx_prefix:
	ret
movq_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	movq_mmreg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	test	[operand_size],not 8
	jnz	invalid_operand_size
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	mov	[postbyte_register],al
	mov	[base_code],0Fh
	cmp	ah,16
	je	movq_mem_xmmreg
	mov	[extended_code],7Fh
	call	store_instruction
	jmp	instruction_assembled
     movq_mem_xmmreg:
	mov	[extended_code],0D6h
	mov	[mmx_prefix],66h
	call	store_mmx_instruction
	jmp	instruction_assembled
     movq_mmreg:
	lods	byte [esi]
	call	convert_mmx_register
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	movq_mmreg_mmreg
	call	get_address
	test	[operand_size],not 8
	jnz	invalid_operand_size
	pop	eax
	mov	[postbyte_register],al
	mov	[base_code],0Fh
	cmp	[operand_size],16
	je	movq_xmmreg_mem
	mov	[extended_code],6Fh
	call	store_instruction
	jmp	instruction_assembled
      movq_xmmreg_mem:
	mov	[extended_code],7Eh
	mov	[mmx_prefix],0F3h
	call	store_instruction
	jmp	instruction_assembled
      movq_mmreg_mmreg:
	pop	ebx
	lods	byte [esi]
	call	convert_mmx_register
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	cmp	ah,16
	je	movq_xmmreg_xmmreg
	mov	ax,6F0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
      movq_xmmreg_xmmreg:
	mov	ax,0FF3h
	stos	word [edi]
	mov	al,07Eh
	mov	ah,bl
	stos	word [edi]
	jmp	instruction_assembled
movdq_instruction:
	mov	[mmx_prefix],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	movdq_mmreg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	[postbyte_register],al
	mov	[base_code],0Fh
	mov	[extended_code],7Fh
	call	store_mmx_instruction
	jmp	instruction_assembled
      movdq_mmreg:
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	movdq_mmreg_mmreg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	mov	[postbyte_register],al
	mov	[base_code],0Fh
	mov	[extended_code],6Fh
	call	store_mmx_instruction
	jmp	instruction_assembled
      movdq_mmreg_mmreg:
	pop	ebx
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	ax,6F0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
movq2dq_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	bl,al
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,8
	jne	invalid_operand_size
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	ax,0FF3h
	stos	word [edi]
	mov	al,0D6h
	mov	ah,bl
	stos	word [edi]
	jmp	instruction_assembled
movdq2q_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,8
	jne	invalid_operand_size
	mov	bl,al
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	ax,0FF2h
	stos	word [edi]
	mov	al,0D6h
	mov	ah,bl
	stos	word [edi]
	jmp	instruction_assembled

mmx_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	call	make_mmx_prefix
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	mmx_mmreg_mmreg
	cmp	al,'['
	jne	invalid_operand
      mmx_mmreg_mem:
	call	get_address
	pop	eax
	mov	[postbyte_register],al
	call	store_mmx_instruction
	jmp	instruction_assembled
      mmx_mmreg_mmreg:
	lods	byte [esi]
	call	convert_mmx_register
	pop	ebx
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
mmx_ps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	call	make_mmx_prefix
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	mmx_mmreg_mmreg
	cmp	al,'('
	je	mmx_ps_mmreg_imm8
	cmp	al,'['
	je	mmx_mmreg_mem
	jmp	invalid_operand
      mmx_ps_mmreg_imm8:
	call	get_byte_value
	mov	byte [value],al
	test	[operand_size],not 1
	jnz	invalid_value
	mov	al,[extended_code]
	mov	ah,al
	shr	al,4
	and	ah,1111b
	add	ah,70h
	mov	[extended_code],ah
	sub	al,0Ch
	shl	al,1
	pop	ebx
	shl	al,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
pmovmskb_instruction:
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jnz	invalid_operand_size
	mov	bl,al
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	call	make_mmx_prefix
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	al,0Fh
	mov	ah,[extended_code]
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	cmp	[extended_code],0C5h
	jne	instruction_assembled
      mmx_imm8:
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	test	ah,not 1
	jnz	invalid_operand_size
	cmp	al,'('
	jne	invalid_operand
	call	get_byte_value
	stos	byte [edi]
	jmp	instruction_assembled
pinsrw_instruction:
	mov	[extended_code],al
	mov	[base_code],0Fh
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	call	make_mmx_prefix
	mov	[postbyte_register],al
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	pinsrw_mmreg_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	cmp	[operand_size],0
	je	pinsrw_store
	cmp	[operand_size],2
	jne	invalid_operand_size
      pinsrw_store:
	call	store_mmx_instruction
	jmp	mmx_imm8
      pinsrw_mmreg_reg:
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	mov	bl,[postbyte_register]
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	al,0Fh
	mov	ah,[extended_code]
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	mmx_imm8
pshufw_instruction:
	mov	[mmx_size],8
	mov	[mmx_prefix],al
	jmp	pshuf_instruction
pshufd_instruction:
	mov	[mmx_size],16
	mov	[mmx_prefix],al
      pshuf_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],70h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,[mmx_size]
	jne	invalid_operand_size
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	pshufw_mmreg_mmreg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	mov	[postbyte_register],al
	call	store_mmx_instruction
	jmp	mmx_imm8
      pshufw_mmreg_mmreg:
	lods	byte [esi]
	call	convert_mmx_register
	pop	ebx
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	mmx_imm8

sse_ps_instruction:
	mov	[mmx_size],16
	mov	[mmx_prefix],0
	jmp	sse_instruction
sse_pd_instruction:
	mov	[mmx_size],16
	mov	[mmx_prefix],66h
	jmp	sse_instruction
sse_ss_instruction:
	mov	[mmx_size],4
	mov	[mmx_prefix],0F3h
	jmp	sse_instruction
sse_sd_instruction:
	mov	[mmx_size],8
	mov	[mmx_prefix],0F2h
	jmp	sse_instruction
comiss_instruction:
	mov	[mmx_size],4
	mov	[mmx_prefix],0
	jmp	sse_instruction
comisd_instruction:
	mov	[mmx_size],8
	mov	[mmx_prefix],66h
	jmp	sse_instruction
cvtps2pd_instruction:
	mov	[mmx_size],8
	mov	[mmx_prefix],0
	jmp	sse_instruction
cvtpd2dq_instruction:
	mov	[mmx_size],16
	mov	[mmx_prefix],0F2h
	jmp	sse_instruction
cvtdq2pd_instruction:
	mov	[mmx_size],16
	mov	[mmx_prefix],0F3h
sse_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
      sse_xmmreg:
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
      sse_reg:
	push	eax
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	sse_xmmreg_xmmreg
	call	get_address
	pop	eax
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	sse_mem_size_ok
	mov	al,[mmx_size]
	cmp	[operand_size],al
	jne	invalid_operand_size
      sse_mem_size_ok:
	call	store_mmx_instruction
	cmp	[extended_code],0C6h
	je	mmx_imm8
	jmp	instruction_assembled
      sse_xmmreg_xmmreg:
	cmp	[extended_code],12h
	je	invalid_operand
	cmp	[extended_code],16h
	je	invalid_operand
      sse_xmmreg_xmmreg_ok:
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	pop	ebx
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	al,0Fh
	mov	ah,[extended_code]
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	cmp	[extended_code],0C6h
	jne	instruction_assembled
	jmp	mmx_imm8
ps_dq_instruction:
	mov	bl,al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	ax,0F66h
	stos	word [edi]
	mov	ah,bl
	mov	al,73h
	stos	word [edi]
	jmp	mmx_imm8
movps_instruction:
	mov	[mmx_prefix],0
	jmp	sse_movp
movpd_instruction:
	mov	[mmx_prefix],66h
      sse_movp:
	mov	[base_code],0Fh
	mov	[extended_code],al
	mov	[mmx_size],16
	jmp	sse_mov_instruction
movss_instruction:
	mov	[mmx_size],4
	mov	[mmx_prefix],0F3h
	jmp	sse_movs
movsd_instruction:
	mov	al,0A5h
	mov	ah,[esi]
	or	ah,ah
	jz	simple_instruction_32bit
	cmp	ah,0Fh
	je	simple_instruction_32bit
	mov	al,66h
	stos	byte [edi]
	mov	[mmx_size],8
	mov	[mmx_prefix],0F2h
      sse_movs:
	mov	[base_code],0Fh
	mov	[extended_code],10h
	jmp	sse_mov_instruction
movlps_instruction:
	mov	[mmx_prefix],0
	jmp	sse_movlp
movlpd_instruction:
	mov	[mmx_prefix],66h
      sse_movlp:
	mov	[base_code],0Fh
	mov	[extended_code],al
	mov	[mmx_size],8
	jmp	sse_mov_instruction
sse_mov_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	sse_xmmreg
	cmp	al,'['
	jne	invalid_operand
	inc	[extended_code]
	call	get_address
	cmp	[operand_size],0
	je	sse_mem_xmmreg
	mov	al,[mmx_size]
	cmp	[operand_size],al
	jne	invalid_operand_size
	mov	[operand_size],0
      sse_mem_xmmreg:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	[postbyte_register],al
	call	store_mmx_instruction
	jmp	instruction_assembled
movhlps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	mov	[mmx_size],0
	mov	[mmx_prefix],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	sse_xmmreg_xmmreg_ok
	jmp	invalid_operand
movmskps_instruction:
	mov	[mmx_prefix],0
	jmp	sse_movmsk
movmskpd_instruction:
	mov	[mmx_prefix],66h
      sse_movmsk:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	mov	[operand_size],0
	mov	bl,al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	ax,500Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
cmpps_instruction:
	mov	[mmx_prefix],0
	jmp	cmppx_instruction
cmppd_instruction:
	mov	[mmx_prefix],66h
      cmppx_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0C2h
	mov	[mmx_size],16
	mov	[nextbyte],-1
	jmp	sse_cmp_instruction
cmp_ps_instruction:
	mov	[mmx_prefix],0
	jmp	cmp_px_instruction
cmp_pd_instruction:
	mov	[mmx_prefix],66h
      cmp_px_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0C2h
	mov	[mmx_size],16
	mov	[nextbyte],al
	jmp	sse_cmp_instruction
cmpss_instruction:
	mov	[mmx_size],4
	mov	[mmx_prefix],0F3h
	jmp	cmpsx_instruction
cmpsd_instruction:
	mov	al,0A7h
	mov	ah,[esi]
	or	ah,ah
	jz	simple_instruction_32bit
	cmp	ah,0Fh
	je	simple_instruction_32bit
	mov	[mmx_size],8
	mov	[mmx_prefix],0F2h
      cmpsx_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0C2h
	mov	[nextbyte],-1
	jmp	sse_cmp_instruction
cmp_ss_instruction:
	mov	[mmx_size],4
	mov	[mmx_prefix],0F3h
	jmp	cmp_sx_instruction
cmp_sd_instruction:
	mov	[mmx_size],8
	mov	[mmx_prefix],0F2h
      cmp_sx_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0C2h
	mov	[nextbyte],al
sse_cmp_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	sse_cmp_xmmreg_xmmreg
	cmp	al,'['
	jne	invalid_operand
	mov	[operand_size],0
	call	get_address
	pop	eax
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	sse_cmp_xmmreg_mem_store
	mov	al,[mmx_size]
	cmp	[operand_size],al
	jne	invalid_operand_size
      sse_cmp_xmmreg_mem_store:
	call	store_mmx_instruction
      sse_cmp_nextbyte:
	mov	al,[nextbyte]
	cmp	al,-1
	jne	nextbyte_ok
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	test	[operand_size],not 1
	jnz	invalid_value
	cmp	al,'('
	jne	invalid_operand
	call	get_byte_value
	cmp	al,7
	ja	invalid_value
      nextbyte_ok:
	stos	byte [edi]
	jmp	instruction_assembled
      sse_cmp_xmmreg_xmmreg:
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	pop	ebx
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	sse_cmp_nextbyte
cvtpi2ps_instruction:
	mov	[mmx_prefix],0
	jmp	cvtpi_instruction
cvtpi2pd_instruction:
	mov	[mmx_prefix],66h
      cvtpi_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	push	eax
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	cvtpi_xmmreg_xmmreg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	cvtpi_size_ok
	cmp	[operand_size],8
	jne	invalid_operand_size
      cvtpi_size_ok:
	call	store_mmx_instruction
	jmp	instruction_assembled
      cvtpi_xmmreg_xmmreg:
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,8
	jne	invalid_operand_size
	pop	ebx
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	ah,[extended_code]
	mov	al,0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
cvtsi2ss_instruction:
	mov	[mmx_prefix],0F3h
	jmp	cvtsi_instruction
cvtsi2sd_instruction:
	mov	[mmx_prefix],0F2h
      cvtsi_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	push	eax
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	cvtsi_xmmreg_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	mov	[postbyte_register],al
	cmp	[operand_size],0
	je	cvtsi_size_ok
	cmp	[operand_size],4
	jne	invalid_operand_size
      cvtsi_size_ok:
	call	store_mmx_instruction
	jmp	instruction_assembled
      cvtsi_xmmreg_reg:
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	pop	ebx
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	al,0Fh
	mov	ah,[extended_code]
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
cvtps2pi_instruction:
	mov	[mmx_prefix],0
	mov	[mmx_size],8
	jmp	cvtpd_instruction
cvtpd2pi_instruction:
	mov	[mmx_prefix],66h
	mov	[mmx_size],16
      cvtpd_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,8
	jne	invalid_operand_size
	mov	[operand_size],0
	jmp	sse_reg
cvtss2si_instruction:
	mov	[mmx_prefix],0F3h
	mov	[mmx_size],4
	jmp	cvt2si_instruction
cvtsd2si_instruction:
	mov	[mmx_prefix],0F2h
	mov	[mmx_size],8
      cvt2si_instruction:
	mov	[extended_code],al
	mov	[base_code],0Fh
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	mov	[operand_size],0
	jmp	sse_reg
amd3dnow_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0Fh
	push	eax
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,8
	jne	invalid_operand_size
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	amd3dnow_mmreg_mmreg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	mov	[postbyte_register],al
	call	store_instruction
	jmp	amd3dnow_assembled
      amd3dnow_mmreg_mmreg:
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,8
	jne	invalid_operand_size
	pop	ebx
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	mov	ax,0F0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
      amd3dnow_assembled:
	pop	eax
	stos	byte [edi]
	jmp	instruction_assembled

fxsave_instruction:
	mov	[extended_code],0AEh
	mov	[base_code],0Fh
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	ah,[operand_size]
	or	ah,ah
	jz	fxsave_size_ok
	mov	al,[postbyte_register]
	cmp	al,10b
	jb	invalid_operand_size
	cmp	al,11b
	ja	invalid_operand_size
	cmp	ah,4
	jne	invalid_operand_size
      fxsave_size_ok:
	call	store_instruction
	jmp	instruction_assembled
prefetch_instruction:
	mov	[extended_code],18h
      prefetch_mem_8bit:
	mov	[base_code],0Fh
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	cmp	ah,0
	je	prefetch_size_ok
	cmp	ah,1
	jne	invalid_operand_size
      prefetch_size_ok:
	call	get_address
	call	store_instruction
	jmp	instruction_assembled
amd_prefetch_instruction:
	mov	[extended_code],0Dh
	jmp	prefetch_mem_8bit
fence_instruction:
	mov	bl,al
	mov	ax,0AE0Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
pause_instruction:
	mov	ax,90F3h
	stos	word [edi]
	jmp	instruction_assembled
maskmovq_instruction:
	mov	cl,8
	mov	[mmx_prefix],0
	jmp	maskmov_instruction
maskmovdqu_instruction:
	mov	cl,16
	mov	[mmx_prefix],66h
      maskmov_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,cl
	jne	invalid_operand_size
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	pop	ebx
	shl	bl,3
	or	bl,al
	or	bl,11000000b
	call	store_mmx_prefix
	mov	ax,0F70Fh
	stos	word [edi]
	mov	al,bl
	stos	byte [edi]
	jmp	instruction_assembled
movntq_instruction:
	mov	[mmx_prefix],0
	mov	[mmx_size],8
	jmp	movnt_instruction
movntps_instruction:
	mov	[mmx_prefix],0
	mov	[mmx_size],16
	jmp	movnt_instruction
movntdq_instruction:
	mov	[mmx_prefix],66h
	mov	[mmx_size],16
      movnt_instruction:
	mov	[extended_code],al
	mov	[base_code],0Fh
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,[mmx_size]
	jne	invalid_operand_size
	mov	[postbyte_register],al
	call	store_mmx_instruction
	jmp	instruction_assembled
movnti_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	mov	[postbyte_register],al
	call	store_instruction
	jmp	instruction_assembled

convert_register:
	mov	ah,al
	shr	ah,4
	and	al,111b
	cmp	ah,4
	ja	invalid_operand
      match_register_size:
	cmp	ah,[operand_size]
	je	register_size_ok
	cmp	[operand_size],0
	jne	operand_sizes_do_not_match
	mov	[operand_size],ah
      register_size_ok:
	ret
convert_fpu_register:
	mov	ah,al
	shr	ah,4
	and	al,111b
	cmp	ah,10
	jne	invalid_operand
	jmp	match_register_size
convert_mmx_register:
	mov	ah,al
	shr	ah,4
	and	al,111b
	cmp	ah,9
	je	xmm_register
	ja	invalid_operand
	cmp	ah,8
	jne	invalid_operand
	jmp	match_register_size
      xmm_register:
	and	al,0Fh
	mov	ah,16
	jmp	match_register_size
get_size_operator:
	xor	ah,ah
	cmp	al,11h
	jne	operand_size_ok
	lods	word [esi]
	xchg	al,ah
	mov	[forced_size],1
	cmp	ah,[operand_size]
	je	forced_ok
	cmp	[operand_size],0
	jne	operand_sizes_do_not_match
	mov	[operand_size],ah
      forced_ok:
	ret
      operand_size_ok:
	cmp	al,'['
	jne	forced_ok
	mov	[forced_size],0
	ret
get_jump_operator:
	mov	[jump_type],0
	cmp	al,12h
	jne	jump_operator_ok
	lods	word [esi]
	mov	[jump_type],al
	mov	al,ah
      jump_operator_ok:
	ret
operand_16bit_prefix:
	cmp	[code_type],16
	je	size_prefix_ok
	mov	al,66h
	stos	byte [edi]
	ret
operand_32bit_prefix:
	cmp	[code_type],32
	je	size_prefix_ok
	mov	al,66h
	stos	byte [edi]
      size_prefix_ok:
	ret
store_segment_prefix_if_necessary:
	mov	al,[segment_register]
	or	al,al
	jz	segment_prefix_ok
	cmp	al,3
	je	ss_prefix
	cmp	al,4
	ja	segment_prefix_386
	jb	segment_prefix_86
	cmp	bh,25h
	je	segment_prefix_86
	cmp	bh,45h
	je	segment_prefix_86
	cmp	bh,44h
	je	segment_prefix_86
	ret
      ss_prefix:
	cmp	bh,25h
	je	segment_prefix_ok
	cmp	bh,45h
	je	segment_prefix_ok
	cmp	bh,44h
	je	segment_prefix_ok
	jmp	segment_prefix_86
store_segment_prefix:
	mov	al,[segment_register]
	or	al,al
	jz	segment_prefix_ok
	cmp	al,5
	jae	segment_prefix_386
      segment_prefix_86:
	dec	al
	shl	al,3
	add	al,26h
	stos	byte [edi]
	jmp	segment_prefix_ok
      segment_prefix_386:
	add	al,64h-5
	stos	byte [edi]
      segment_prefix_ok:
	ret
store_mmx_prefix:
	mov	al,[mmx_prefix]
	or	al,al
	jz	mmx_prefix_ok
	stos	byte [edi]
      mmx_prefix_ok:
	ret
store_mmx_instruction:
	call	store_segment_prefix
	call	store_mmx_prefix
	jmp	store_instruction_main
store_instruction:
	call	store_segment_prefix_if_necessary
      store_instruction_main:
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
	cmp	ch,2
	je	address_16bit_value
	or	ch,ch
	jnz	address_sizes_do_not_agree
	or	edx,edx
	jz	address
	cmp	edx,80h
	jb	address_8bit_value
	cmp	edx,-80h
	jae	address_8bit_value
      address_16bit_value:
	or	al,10000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
	mov	eax,edx
	stos	word [edi]
	cmp	edx,10000h
	jge	value_out_of_range
	cmp	edx,-8000h
	jl	value_out_of_range
	ret
      address_8bit_value:
	or	al,01000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	cmp	edx,80h
	jge	value_out_of_range
	cmp	edx,-80h
	jl	value_out_of_range
	ret
      address:
	cmp	al,110b
	je	address_8bit_value
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
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
	test	ch,4
	jnz	sib_address_32bit_value
	cmp	ch,2
	je	address_sizes_do_not_agree
	cmp	bh,5
	je	address_value
	or	edx,edx
	jz	sib_address
      address_value:
	cmp	edx,80h
	jb	sib_address_8bit_value
	cmp	edx,-80h
	jae	sib_address_8bit_value
      sib_address_32bit_value:
	or	al,10000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	word [edi]
	jmp	store_address_32bit_value
      sib_address_8bit_value:
	or	al,01000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	word [edi]
	mov	al,dl
	stos	byte [edi]
	cmp	edx,80h
	jge	value_out_of_range
	cmp	edx,-80h
	jl	value_out_of_range
	ret
      sib_address:
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	word [edi]
	ret
      only_index_register:
	or	ah,101b
	and	bl,111b
	shl	bl,3
	or	ah,bl
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	word [edi]
	test	ch,4
	jnz	store_address_32bit_value
	or	ch,ch
	jnz	invalid_address_size
	jmp	store_address_32bit_value
      zero_index_register:
	mov	bl,4
	mov	cl,1
	jmp	base_and_index
      only_base_register:
	mov	al,bh
	and	al,111b
	cmp	al,4
	je	zero_index_register
	test	ch,4
	jnz	simple_address_32bit_value
	cmp	ch,2
	je	address_sizes_do_not_agree
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
	stos	byte [edi]
	jmp	store_address_32bit_value
      simple_address_8bit_value:
	or	al,01000000b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	cmp	edx,80h
	jge	value_out_of_range
	cmp	edx,-80h
	jl	value_out_of_range
	ret
      simple_address:
	cmp	al,5
	je	simple_address_8bit_value
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
	ret
      address_immediate:
	test	ch,4
	jnz	address_immediate_32bit
	cmp	ch,2
	je	address_immediate_16bit
	or	ch,ch
	jnz	invalid_address_size
	cmp	[code_type],16
	je	addressing_16bit
      address_immediate_32bit:
	call	address_32bit_prefix
	call	store_instruction_code
	mov	al,101b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
      store_address_32bit_value:
	test	ch,80h
	jz	address_relocation_ok
	mov	al,2
	xchg	[value_type],al
	mov	ebx,[address_symbol]
	xchg	ebx,[symbol_identifier]
	call	mark_relocation
	mov	[value_type],al
	mov	[symbol_identifier],ebx
      address_relocation_ok:
	mov	eax,edx
	stos	dword [edi]
	ret
      addressing_16bit:
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
	stos	byte [edi]
	mov	eax,edx
	stos	word [edi]
	cmp	edx,10000h
	jge	value_out_of_range
	cmp	edx,-8000h
	jl	value_out_of_range
	ret
      store_instruction_code:
	mov	al,[base_code]
	stos	byte [edi]
	cmp	al,0Fh
	jne	instruction_code_ok
      store_extended_code:
	mov	al,[extended_code]
	stos	byte [edi]
      instruction_code_ok:
	ret
      address_16bit_prefix:
	cmp	[code_type],16
	je	instruction_prefix_ok
	mov	al,67h
	stos	byte [edi]
	ret
      address_32bit_prefix:
	cmp	[code_type],32
	je	instruction_prefix_ok
	mov	al,67h
	stos	byte [edi]
      instruction_prefix_ok:
	ret

address_registers:
 db 2,'bp',0,25h
 db 2,'bx',0,23h
 db 2,'di',0,27h
 db 3,'eax',0,40h
 db 3,'ebp',0,45h
 db 3,'ebx',0,43h
 db 3,'ecx',0,41h
 db 3,'edi',0,47h
 db 3,'edx',0,42h
 db 3,'esi',0,46h
 db 3,'esp',0,44h
 db 2,'si',0,26h
 db 0

address_sizes:
 db 4,'byte',0,1
 db 5,'dword',0,4
 db 4,'word',0,2
 db 0

symbols:
 db 2,'ah',10h,14h
 db 2,'al',10h,10h
 db 2,'ax',10h,20h
 db 2,'bh',10h,17h
 db 2,'bl',10h,13h
 db 2,'bp',10h,25h
 db 2,'bx',10h,23h
 db 4,'byte',11h,1
 db 2,'ch',10h,15h
 db 2,'cl',10h,11h
 db 3,'cr0',10h,50h
 db 3,'cr2',10h,52h
 db 3,'cr3',10h,53h
 db 3,'cr4',10h,54h
 db 2,'cs',10h,62h
 db 2,'cx',10h,21h
 db 2,'dh',10h,16h
 db 2,'di',10h,27h
 db 2,'dl',10h,12h
 db 6,'dqword',11h,16
 db 3,'dr0',10h,70h
 db 3,'dr1',10h,71h
 db 3,'dr2',10h,72h
 db 3,'dr3',10h,73h
 db 3,'dr5',10h,75h
 db 3,'dr6',10h,76h
 db 3,'dr7',10h,77h
 db 2,'ds',10h,64h
 db 5,'dword',11h,4
 db 2,'dx',10h,22h
 db 3,'eax',10h,40h
 db 3,'ebp',10h,45h
 db 3,'ebx',10h,43h
 db 3,'ecx',10h,41h
 db 3,'edi',10h,47h
 db 3,'edx',10h,42h
 db 2,'es',10h,61h
 db 3,'esi',10h,46h
 db 3,'esp',10h,44h
 db 3,'far',12h,2
 db 2,'fs',10h,65h
 db 5,'fword',11h,6
 db 2,'gs',10h,66h
 db 3,'mm0',10h,80h
 db 3,'mm1',10h,81h
 db 3,'mm2',10h,82h
 db 3,'mm3',10h,83h
 db 3,'mm4',10h,84h
 db 3,'mm5',10h,85h
 db 3,'mm6',10h,86h
 db 3,'mm7',10h,87h
 db 4,'near',12h,1
 db 5,'pword',11h,6
 db 5,'qword',11h,8
 db 2,'si',10h,26h
 db 2,'sp',10h,24h
 db 2,'ss',10h,63h
 db 2,'st',10h,0A0h
 db 3,'st0',10h,0A0h
 db 3,'st1',10h,0A1h
 db 3,'st2',10h,0A2h
 db 3,'st3',10h,0A3h
 db 3,'st4',10h,0A4h
 db 3,'st5',10h,0A5h
 db 3,'st6',10h,0A6h
 db 3,'st7',10h,0A7h
 db 5,'tword',11h,0Ah
 db 5,'use16',13h,10h
 db 5,'use32',13h,20h
 db 4,'word',11h,2
 db 4,'xmm0',10h,90h
 db 4,'xmm1',10h,91h
 db 4,'xmm2',10h,92h
 db 4,'xmm3',10h,93h
 db 4,'xmm4',10h,94h
 db 4,'xmm5',10h,95h
 db 4,'xmm6',10h,96h
 db 4,'xmm7',10h,97h
 db 0

data_handlers:
 dw data_bytes-assembler
 dw data_file-assembler
 dw reserve_bytes-assembler
 dw data_words-assembler
 dw data_unicode-assembler
 dw reserve_words-assembler
 dw data_dwords-assembler
 dw reserve_dwords-assembler
 dw data_pwords-assembler
 dw reserve_pwords-assembler
 dw data_qwords-assembler
 dw reserve_qwords-assembler
 dw data_twords-assembler
 dw reserve_twords-assembler

data_directives:
 db 2,'db',1,0
 db 2,'dd',4,6
 db 2,'df',6,8
 db 2,'dp',6,8
 db 2,'dq',8,10
 db 2,'dt',10,12
 db 2,'du',2,4
 db 2,'dw',2,3
 db 4,'file',1,1
 db 2,'rb',1,2
 db 2,'rd',4,7
 db 2,'rf',6,9
 db 2,'rp',6,9
 db 2,'rq',8,11
 db 2,'rt',10,13
 db 2,'rw',2,5
 db 0

instructions:
 dw instructions_2-instructions
 dw instructions_3-instructions
 dw instructions_4-instructions
 dw instructions_5-instructions
 dw instructions_6-instructions
 dw instructions_7-instructions
 dw instructions_8-instructions
 dw instructions_9-instructions
 dw instructions_10-instructions
 dw instructions_11-instructions

instructions_2:
 db 'bt',4
 dw bt_instruction-assembler
 db 'if',0
 dw if_directive-assembler
 db 'in',0
 dw in_instruction-assembler
 db 'ja',77h
 dw conditional_jump-assembler
 db 'jb',72h
 dw conditional_jump-assembler
 db 'jc',72h
 dw conditional_jump-assembler
 db 'je',74h
 dw conditional_jump-assembler
 db 'jg',7Fh
 dw conditional_jump-assembler
 db 'jl',7Ch
 dw conditional_jump-assembler
 db 'jo',70h
 dw conditional_jump-assembler
 db 'jp',7Ah
 dw conditional_jump-assembler
 db 'js',78h
 dw conditional_jump-assembler
 db 'jz',74h
 dw conditional_jump-assembler
 db 'or',08h
 dw basic_instruction-assembler
 db 0
instructions_3:
 db 'aaa',37h
 dw simple_instruction-assembler
 db 'aad',0D5h
 dw aa_instruction-assembler
 db 'aam',0D4h
 dw aa_instruction-assembler
 db 'aas',3Fh
 dw simple_instruction-assembler
 db 'adc',10h
 dw basic_instruction-assembler
 db 'add',00h
 dw basic_instruction-assembler
 db 'and',20h
 dw basic_instruction-assembler
 db 'bsf',0BCh
 dw bs_instruction-assembler
 db 'bsr',0BDh
 dw bs_instruction-assembler
 db 'btc',7
 dw bt_instruction-assembler
 db 'btr',6
 dw bt_instruction-assembler
 db 'bts',5
 dw bt_instruction-assembler
 db 'cbw',98h
 dw simple_instruction_16bit-assembler
 db 'cdq',99h
 dw simple_instruction_32bit-assembler
 db 'clc',0F8h
 dw simple_instruction-assembler
 db 'cld',0FCh
 dw simple_instruction-assembler
 db 'cli',0FAh
 dw simple_instruction-assembler
 db 'cmc',0F5h
 dw simple_instruction-assembler
 db 'cmp',38h
 dw basic_instruction-assembler
 db 'cwd',99h
 dw simple_instruction_16bit-assembler
 db 'daa',27h
 dw simple_instruction-assembler
 db 'das',2Fh
 dw simple_instruction-assembler
 db 'dec',1
 dw inc_instruction-assembler
 db 'div',6
 dw single_operand_instruction-assembler
 db 'end',0
 dw end_directive-assembler
 db 'fld',0
 dw fld_instruction-assembler
 db 'fst',2
 dw fld_instruction-assembler
 db 'hlt',0F4h
 dw simple_instruction-assembler
 db 'inc',0
 dw inc_instruction-assembler
 db 'ins',0
 dw ins_instruction-assembler
 db 'int',0CDh
 dw int_instruction-assembler
 db 'jae',73h
 dw conditional_jump-assembler
 db 'jbe',76h
 dw conditional_jump-assembler
 db 'jge',7Dh
 dw conditional_jump-assembler
 db 'jle',7Eh
 dw conditional_jump-assembler
 db 'jmp',0
 dw jmp_instruction-assembler
 db 'jna',76h
 dw conditional_jump-assembler
 db 'jnb',73h
 dw conditional_jump-assembler
 db 'jnc',73h
 dw conditional_jump-assembler
 db 'jne',75h
 dw conditional_jump-assembler
 db 'jng',7Eh
 dw conditional_jump-assembler
 db 'jnl',7Dh
 dw conditional_jump-assembler
 db 'jno',71h
 dw conditional_jump-assembler
 db 'jnp',7Bh
 dw conditional_jump-assembler
 db 'jns',79h
 dw conditional_jump-assembler
 db 'jnz',75h
 dw conditional_jump-assembler
 db 'jpe',7Ah
 dw conditional_jump-assembler
 db 'jpo',7Bh
 dw conditional_jump-assembler
 db 'lar',2
 dw lar_instruction-assembler
 db 'lds',3
 dw ls_instruction-assembler
 db 'lea',0
 dw lea_instruction-assembler
 db 'les',0
 dw ls_instruction-assembler
 db 'lfs',4
 dw ls_instruction-assembler
 db 'lgs',5
 dw ls_instruction-assembler
 db 'lsl',3
 dw lar_instruction-assembler
 db 'lss',2
 dw ls_instruction-assembler
 db 'ltr',3
 dw pm_word_instruction-assembler
 db 'mov',0
 dw mov_instruction-assembler
 db 'mul',4
 dw single_operand_instruction-assembler
 db 'neg',3
 dw single_operand_instruction-assembler
 db 'nop',90h
 dw simple_instruction-assembler
 db 'not',2
 dw single_operand_instruction-assembler
 db 'org',0
 dw org_directive-assembler
 db 'out',0
 dw out_instruction-assembler
 db 'pop',0
 dw pop_instruction-assembler
 db 'por',0EBh
 dw mmx_instruction-assembler
 db 'rcl',2
 dw sh_instruction-assembler
 db 'rcr',3
 dw sh_instruction-assembler
 db 'rep',0F3h
 dw prefix_instruction-assembler
 db 'ret',0C2h
 dw ret_instruction-assembler
 db 'rol',0
 dw sh_instruction-assembler
 db 'ror',1
 dw sh_instruction-assembler
 db 'rsm',0AAh
 dw simple_extended_instruction-assembler
 db 'sal',6
 dw sh_instruction-assembler
 db 'sar',7
 dw sh_instruction-assembler
 db 'sbb',18h
 dw basic_instruction-assembler
 db 'shl',4
 dw sh_instruction-assembler
 db 'shr',5
 dw sh_instruction-assembler
 db 'stc',0F9h
 dw simple_instruction-assembler
 db 'std',0FDh
 dw simple_instruction-assembler
 db 'sti',0FBh
 dw simple_instruction-assembler
 db 'str',1
 dw pm_word_instruction-assembler
 db 'sub',28h
 dw basic_instruction-assembler
 db 'ud2',0Bh
 dw simple_extended_instruction-assembler
 db 'xor',30h
 dw basic_instruction-assembler
 db 0
instructions_4:
 db 'arpl',0
 dw arpl_instruction-assembler
 db 'call',0
 dw call_instruction-assembler
 db 'clts',6
 dw simple_extended_instruction-assembler
 db 'cmps',0
 dw cmps_instruction-assembler
 db 'cwde',98h
 dw simple_instruction_32bit-assembler
 db 'data',0
 dw data_directive-assembler
 db 'else',0
 dw else_directive-assembler
 db 'emms',77h
 dw simple_extended_instruction-assembler
 db 'fabs',100001b
 dw simple_fpu_instruction-assembler
 db 'fadd',0
 dw basic_fpu_instruction-assembler
 db 'fbld',4
 dw fbld_instruction-assembler
 db 'fchs',100000b
 dw simple_fpu_instruction-assembler
 db 'fcom',2
 dw basic_fpu_instruction-assembler
 db 'fcos',111111b
 dw simple_fpu_instruction-assembler
 db 'fdiv',6
 dw basic_fpu_instruction-assembler
 db 'feni',0E0h
 dw finit_instruction-assembler
 db 'fild',0
 dw fild_instruction-assembler
 db 'fist',2
 dw fild_instruction-assembler
 db 'fld1',101000b
 dw simple_fpu_instruction-assembler
 db 'fldz',101110b
 dw simple_fpu_instruction-assembler
 db 'fmul',1
 dw basic_fpu_instruction-assembler
 db 'fnop',010000b
 dw simple_fpu_instruction-assembler
 db 'fsin',111110b
 dw simple_fpu_instruction-assembler
 db 'fstp',3
 dw fld_instruction-assembler
 db 'fsub',4
 dw basic_fpu_instruction-assembler
 db 'ftst',100100b
 dw simple_fpu_instruction-assembler
 db 'fxam',100101b
 dw simple_fpu_instruction-assembler
 db 'fxch',0
 dw fxch_instruction-assembler
 db 'heap',0
 dw heap_directive-assembler
 db 'idiv',7
 dw single_operand_instruction-assembler
 db 'imul',0
 dw imul_instruction-assembler
 db 'insb',6Ch
 dw simple_instruction-assembler
 db 'insd',6Dh
 dw simple_instruction_32bit-assembler
 db 'insw',6Dh
 dw simple_instruction_16bit-assembler
 db 'int3',0CCh
 dw simple_instruction-assembler
 db 'into',0CEh
 dw simple_instruction-assembler
 db 'invd',8
 dw simple_extended_instruction-assembler
 db 'iret',0CFh
 dw simple_instruction-assembler
 db 'jcxz',0E3h
 dw loop_instruction_16bit-assembler
 db 'jnae',72h
 dw conditional_jump-assembler
 db 'jnbe',77h
 dw conditional_jump-assembler
 db 'jnge',7Ch
 dw conditional_jump-assembler
 db 'jnle',7Fh
 dw conditional_jump-assembler
 db 'lahf',9Fh
 dw simple_instruction-assembler
 db 'lgdt',2
 dw pm_pword_instruction-assembler
 db 'lidt',3
 dw pm_pword_instruction-assembler
 db 'lldt',2
 dw pm_word_instruction-assembler
 db 'lmsw',16h
 dw pm_word_instruction-assembler
 db 'load',0
 dw load_directive-assembler
 db 'lock',0F0h
 dw prefix_instruction-assembler
 db 'lods',0
 dw lods_instruction-assembler
 db 'loop',0E2h
 dw loop_instruction-assembler
 db 'movd',0
 dw movd_instruction-assembler
 db 'movq',0
 dw movq_instruction-assembler
 db 'movs',0
 dw movs_instruction-assembler
 db 'orpd',56h
 dw sse_pd_instruction-assembler
 db 'orps',56h
 dw sse_ps_instruction-assembler
 db 'outs',0
 dw outs_instruction-assembler
 db 'pand',0DBh
 dw mmx_instruction-assembler
 db 'popa',61h
 dw simple_instruction-assembler
 db 'popd',4
 dw pop_instruction-assembler
 db 'popf',9Dh
 dw simple_instruction-assembler
 db 'popw',2
 dw pop_instruction-assembler
 db 'push',0
 dw push_instruction-assembler
 db 'pxor',0EFh
 dw mmx_instruction-assembler
 db 'repe',0F3h
 dw prefix_instruction-assembler
 db 'repz',0F3h
 dw prefix_instruction-assembler
 db 'retd',0C2h
 dw ret_instruction_32bit-assembler
 db 'retf',0CAh
 dw ret_instruction-assembler
 db 'retn',0C2h
 dw ret_instruction-assembler
 db 'retw',0C2h
 dw ret_instruction_16bit-assembler
 db 'sahf',9Eh
 dw simple_instruction-assembler
 db 'salc',0D6h
 dw simple_instruction-assembler
 db 'scas',0AEh
 dw stos_instruction-assembler
 db 'seta',97h
 dw set_instruction-assembler
 db 'setb',92h
 dw set_instruction-assembler
 db 'setc',92h
 dw set_instruction-assembler
 db 'sete',94h
 dw set_instruction-assembler
 db 'setg',9Fh
 dw set_instruction-assembler
 db 'setl',9Ch
 dw set_instruction-assembler
 db 'seto',90h
 dw set_instruction-assembler
 db 'setp',9Ah
 dw set_instruction-assembler
 db 'sets',98h
 dw set_instruction-assembler
 db 'setz',94h
 dw set_instruction-assembler
 db 'sgdt',0
 dw pm_pword_instruction-assembler
 db 'shld',0A4h
 dw shd_instruction-assembler
 db 'shrd',0ACh
 dw shd_instruction-assembler
 db 'sidt',1
 dw pm_pword_instruction-assembler
 db 'sldt',0
 dw pm_word_instruction-assembler
 db 'smsw',14h
 dw pm_word_instruction-assembler
 db 'stos',0AAh
 dw stos_instruction-assembler
 db 'test',0
 dw test_instruction-assembler
 db 'verr',4
 dw pm_word_instruction-assembler
 db 'verw',5
 dw pm_word_instruction-assembler
 db 'wait',9Bh
 dw simple_instruction-assembler
 db 'xadd',0C0h
 dw basic_486_instruction-assembler
 db 'xchg',0
 dw xchg_instruction-assembler
 db 'xlat',0D7h
 dw xlat_instruction-assembler
 db 0
instructions_5:
 db 'addpd',58h
 dw sse_pd_instruction-assembler
 db 'addps',58h
 dw sse_ps_instruction-assembler
 db 'addsd',58h
 dw sse_sd_instruction-assembler
 db 'addss',58h
 dw sse_ss_instruction-assembler
 db 'andpd',54h
 dw sse_pd_instruction-assembler
 db 'andps',54h
 dw sse_ps_instruction-assembler
 db 'bound',0
 dw bound_instruction-assembler
 db 'bswap',0
 dw bswap_instruction-assembler
 db 'cmova',47h
 dw cmov_instruction-assembler
 db 'cmovb',42h
 dw cmov_instruction-assembler
 db 'cmovc',42h
 dw cmov_instruction-assembler
 db 'cmove',44h
 dw cmov_instruction-assembler
 db 'cmovg',4Fh
 dw cmov_instruction-assembler
 db 'cmovl',4Ch
 dw cmov_instruction-assembler
 db 'cmovo',40h
 dw cmov_instruction-assembler
 db 'cmovp',4Ah
 dw cmov_instruction-assembler
 db 'cmovs',48h
 dw cmov_instruction-assembler
 db 'cmovz',44h
 dw cmov_instruction-assembler
 db 'cmppd',0
 dw cmppd_instruction-assembler
 db 'cmpps',0
 dw cmpps_instruction-assembler
 db 'cmpsb',0A6h
 dw simple_instruction-assembler
 db 'cmpsd',0
 dw cmpsd_instruction-assembler
 db 'cmpss',0
 dw cmpss_instruction-assembler
 db 'cmpsw',0A7h
 dw simple_instruction_16bit-assembler
 db 'cpuid',0A2h
 dw simple_extended_instruction-assembler
 db 'divpd',5Eh
 dw sse_pd_instruction-assembler
 db 'divps',5Eh
 dw sse_ps_instruction-assembler
 db 'divsd',5Eh
 dw sse_sd_instruction-assembler
 db 'divss',5Eh
 dw sse_ss_instruction-assembler
 db 'enter',0
 dw enter_instruction-assembler
 db 'entry',0
 dw entry_directive-assembler
 db 'extrn',0
 dw extrn_directive-assembler
 db 'f2xm1',110000b
 dw simple_fpu_instruction-assembler
 db 'faddp',0
 dw faddp_instruction-assembler
 db 'fbstp',6
 dw fbld_instruction-assembler
 db 'fclex',0E2h
 dw finit_instruction-assembler
 db 'fcomi',0F0h
 dw fcomi_instruction-assembler
 db 'fcomp',3
 dw basic_fpu_instruction-assembler
 db 'fdisi',0E1h
 dw finit_instruction-assembler
 db 'fdivp',7
 dw faddp_instruction-assembler
 db 'fdivr',7
 dw basic_fpu_instruction-assembler
 db 'femms',0Eh
 dw simple_extended_instruction-assembler
 db 'ffree',0
 dw ffree_instruction-assembler
 db 'fiadd',0
 dw fi_instruction-assembler
 db 'ficom',2
 dw fi_instruction-assembler
 db 'fidiv',6
 dw fi_instruction-assembler
 db 'fimul',1
 dw fi_instruction-assembler
 db 'finit',0E3h
 dw finit_instruction-assembler
 db 'fistp',3
 dw fild_instruction-assembler
 db 'fisub',4
 dw fi_instruction-assembler
 db 'fldcw',5
 dw fldcw_instruction-assembler
 db 'fldpi',101011b
 dw simple_fpu_instruction-assembler
 db 'fmulp',1
 dw faddp_instruction-assembler
 db 'fneni',0E0h
 dw fninit_instruction-assembler
 db 'fprem',111000b
 dw simple_fpu_instruction-assembler
 db 'fptan',110010b
 dw simple_fpu_instruction-assembler
 db 'fsave',6
 dw fsave_instruction-assembler
 db 'fsqrt',111010b
 dw simple_fpu_instruction-assembler
 db 'fstcw',7
 dw fstcw_instruction-assembler
 db 'fstsw',0
 dw fstsw_instruction-assembler
 db 'fsubp',5
 dw faddp_instruction-assembler
 db 'fsubr',5
 dw basic_fpu_instruction-assembler
 db 'fucom',4
 dw ffree_instruction-assembler
 db 'fwait',9Bh
 dw simple_instruction-assembler
 db 'fyl2x',110001b
 dw simple_fpu_instruction-assembler
 db 'iretd',0CFh
 dw simple_instruction_32bit-assembler
 db 'iretw',0CFh
 dw simple_instruction_16bit-assembler
 db 'jecxz',0E3h
 dw loop_instruction_32bit-assembler
 db 'label',0
 dw label_directive-assembler
 db 'leave',0C9h
 dw simple_instruction-assembler
 db 'lodsb',0ACh
 dw simple_instruction-assembler
 db 'lodsd',0ADh
 dw simple_instruction_32bit-assembler
 db 'lodsw',0ADh
 dw simple_instruction_16bit-assembler
 db 'loopd',0E2h
 dw loop_instruction_32bit-assembler
 db 'loope',0E1h
 dw loop_instruction-assembler
 db 'loopw',0E2h
 dw loop_instruction_16bit-assembler
 db 'loopz',0E1h
 dw loop_instruction-assembler
 db 'maxpd',5Fh
 dw sse_pd_instruction-assembler
 db 'maxps',5Fh
 dw sse_ps_instruction-assembler
 db 'maxsd',5Fh
 dw sse_sd_instruction-assembler
 db 'maxss',5Fh
 dw sse_ss_instruction-assembler
 db 'minpd',5Dh
 dw sse_pd_instruction-assembler
 db 'minps',5Dh
 dw sse_ps_instruction-assembler
 db 'minsd',5Dh
 dw sse_sd_instruction-assembler
 db 'minss',5Dh
 dw sse_ss_instruction-assembler
 db 'movsb',0A4h
 dw simple_instruction-assembler
 db 'movsd',0
 dw movsd_instruction-assembler
 db 'movss',0
 dw movss_instruction-assembler
 db 'movsw',0A5h
 dw simple_instruction_16bit-assembler
 db 'movsx',0BEh
 dw movx_instruction-assembler
 db 'movzx',0B6h
 dw movx_instruction-assembler
 db 'mulpd',59h
 dw sse_pd_instruction-assembler
 db 'mulps',59h
 dw sse_ps_instruction-assembler
 db 'mulsd',59h
 dw sse_sd_instruction-assembler
 db 'mulss',59h
 dw sse_ss_instruction-assembler
 db 'outsb',6Eh
 dw simple_instruction-assembler
 db 'outsd',6Fh
 dw simple_instruction_32bit-assembler
 db 'outsw',6Fh
 dw simple_instruction_16bit-assembler
 db 'paddb',0FCh
 dw mmx_instruction-assembler
 db 'paddd',0FEh
 dw mmx_instruction-assembler
 db 'paddq',0D4h
 dw mmx_instruction-assembler
 db 'paddw',0FDh
 dw mmx_instruction-assembler
 db 'pandn',0DFh
 dw mmx_instruction-assembler
 db 'pause',0
 dw pause_instruction-assembler
 db 'pavgb',0E0h
 dw mmx_instruction-assembler
 db 'pavgw',0E3h
 dw mmx_instruction-assembler
 db 'pfacc',0AEh
 dw amd3dnow_instruction-assembler
 db 'pfadd',9Eh
 dw amd3dnow_instruction-assembler
 db 'pfmax',0A4h
 dw amd3dnow_instruction-assembler
 db 'pfmin',94h
 dw amd3dnow_instruction-assembler
 db 'pfmul',0B4h
 dw amd3dnow_instruction-assembler
 db 'pfrcp',96h
 dw amd3dnow_instruction-assembler
 db 'pfsub',9Ah
 dw amd3dnow_instruction-assembler
 db 'pi2fd',0Dh
 dw amd3dnow_instruction-assembler
 db 'pi2fw',0Ch
 dw amd3dnow_instruction-assembler
 db 'pi2id',1Dh
 dw amd3dnow_instruction-assembler
 db 'pi2iw',1Ch
 dw amd3dnow_instruction-assembler
 db 'popad',61h
 dw simple_instruction_32bit-assembler
 db 'popaw',61h
 dw simple_instruction_16bit-assembler
 db 'popfd',9Dh
 dw simple_instruction_32bit-assembler
 db 'popfw',9Dh
 dw simple_instruction_16bit-assembler
 db 'pslld',0F2h
 dw mmx_ps_instruction-assembler
 db 'psllq',0F3h
 dw mmx_ps_instruction-assembler
 db 'psllw',0F1h
 dw mmx_ps_instruction-assembler
 db 'psrad',0E2h
 dw mmx_ps_instruction-assembler
 db 'psraw',0E1h
 dw mmx_ps_instruction-assembler
 db 'psrld',0D2h
 dw mmx_ps_instruction-assembler
 db 'psrlq',0D3h
 dw mmx_ps_instruction-assembler
 db 'psrlw',0D1h
 dw mmx_ps_instruction-assembler
 db 'psubb',0F8h
 dw mmx_instruction-assembler
 db 'psubd',0FAh
 dw mmx_instruction-assembler
 db 'psubq',0FBh
 dw mmx_instruction-assembler
 db 'psubw',0F9h
 dw mmx_instruction-assembler
 db 'pusha',60h
 dw simple_instruction-assembler
 db 'pushd',4
 dw push_instruction-assembler
 db 'pushf',9Ch
 dw simple_instruction-assembler
 db 'pushw',2
 dw push_instruction-assembler
 db 'rcpps',53h
 dw sse_ps_instruction-assembler
 db 'rcpss',53h
 dw sse_ss_instruction-assembler
 db 'rdmsr',32h
 dw simple_extended_instruction-assembler
 db 'rdpmc',33h
 dw simple_extended_instruction-assembler
 db 'rdtsc',31h
 dw simple_extended_instruction-assembler
 db 'repne',0F2h
 dw prefix_instruction-assembler
 db 'repnz',0F2h
 dw prefix_instruction-assembler
 db 'retfd',0CAh
 dw ret_instruction_32bit-assembler
 db 'retfw',0CAh
 dw ret_instruction_16bit-assembler
 db 'retnd',0C2h
 dw ret_instruction_32bit-assembler
 db 'retnw',0C2h
 dw ret_instruction_16bit-assembler
 db 'scasb',0AEh
 dw simple_instruction-assembler
 db 'scasd',0AFh
 dw simple_instruction_32bit-assembler
 db 'scasw',0AFh
 dw simple_instruction_16bit-assembler
 db 'setae',93h
 dw set_instruction-assembler
 db 'setbe',96h
 dw set_instruction-assembler
 db 'setge',9Dh
 dw set_instruction-assembler
 db 'setle',9Eh
 dw set_instruction-assembler
 db 'setna',96h
 dw set_instruction-assembler
 db 'setnb',93h
 dw set_instruction-assembler
 db 'setnc',93h
 dw set_instruction-assembler
 db 'setne',95h
 dw set_instruction-assembler
 db 'setng',9Eh
 dw set_instruction-assembler
 db 'setnl',9Dh
 dw set_instruction-assembler
 db 'setno',91h
 dw set_instruction-assembler
 db 'setnp',9Bh
 dw set_instruction-assembler
 db 'setns',99h
 dw set_instruction-assembler
 db 'setnz',95h
 dw set_instruction-assembler
 db 'setpe',9Ah
 dw set_instruction-assembler
 db 'setpo',9Bh
 dw set_instruction-assembler
 db 'stack',0
 dw stack_directive-assembler
 db 'stosb',0AAh
 dw simple_instruction-assembler
 db 'stosd',0ABh
 dw simple_instruction_32bit-assembler
 db 'stosw',0ABh
 dw simple_instruction_16bit-assembler
 db 'subpd',5Ch
 dw sse_pd_instruction-assembler
 db 'subps',5Ch
 dw sse_ps_instruction-assembler
 db 'subsd',5Ch
 dw sse_sd_instruction-assembler
 db 'subss',5Ch
 dw sse_ss_instruction-assembler
 db 'times',0
 dw times_directive-assembler
 db 'wrmsr',30h
 dw simple_extended_instruction-assembler
 db 'xlatb',0D7h
 dw simple_instruction-assembler
 db 'xorpd',57h
 dw sse_pd_instruction-assembler
 db 'xorps',57h
 dw sse_ps_instruction-assembler
 db 0
instructions_6:
 db 'andnpd',55h
 dw sse_pd_instruction-assembler
 db 'andnps',55h
 dw sse_ps_instruction-assembler
 db 'cmovae',43h
 dw cmov_instruction-assembler
 db 'cmovbe',46h
 dw cmov_instruction-assembler
 db 'cmovge',4Dh
 dw cmov_instruction-assembler
 db 'cmovle',4Eh
 dw cmov_instruction-assembler
 db 'cmovna',46h
 dw cmov_instruction-assembler
 db 'cmovnb',43h
 dw cmov_instruction-assembler
 db 'cmovnc',43h
 dw cmov_instruction-assembler
 db 'cmovne',45h
 dw cmov_instruction-assembler
 db 'cmovng',4Eh
 dw cmov_instruction-assembler
 db 'cmovnl',4Dh
 dw cmov_instruction-assembler
 db 'cmovno',41h
 dw cmov_instruction-assembler
 db 'cmovnp',4Bh
 dw cmov_instruction-assembler
 db 'cmovns',49h
 dw cmov_instruction-assembler
 db 'cmovnz',45h
 dw cmov_instruction-assembler
 db 'cmovpe',4Ah
 dw cmov_instruction-assembler
 db 'cmovpo',4Bh
 dw cmov_instruction-assembler
 db 'comisd',2Fh
 dw comisd_instruction-assembler
 db 'comiss',2Fh
 dw comiss_instruction-assembler
 db 'fcmovb',0C0h
 dw fcmov_instruction-assembler
 db 'fcmove',0C8h
 dw fcmov_instruction-assembler
 db 'fcmovu',0D8h
 dw fcmov_instruction-assembler
 db 'fcomip',0F0h
 dw fcomip_instruction-assembler
 db 'fcompp',0
 dw fcompp_instruction-assembler
 db 'fdivrp',6
 dw faddp_instruction-assembler
 db 'ffreep',0
 dw ffreep_instruction-assembler
 db 'ficomp',3
 dw fi_instruction-assembler
 db 'fidivr',7
 dw fi_instruction-assembler
 db 'fisubr',5
 dw fi_instruction-assembler
 db 'fldenv',4
 dw fldenv_instruction-assembler
 db 'fldl2e',101010b
 dw simple_fpu_instruction-assembler
 db 'fldl2t',101001b
 dw simple_fpu_instruction-assembler
 db 'fldlg2',101100b
 dw simple_fpu_instruction-assembler
 db 'fldln2',101101b
 dw simple_fpu_instruction-assembler
 db 'fnclex',0E2h
 dw fninit_instruction-assembler
 db 'fndisi',0E1h
 dw fninit_instruction-assembler
 db 'fninit',0E3h
 dw fninit_instruction-assembler
 db 'fnsave',6
 dw fnsave_instruction-assembler
 db 'fnstcw',7
 dw fldcw_instruction-assembler
 db 'fnstsw',0
 dw fnstsw_instruction-assembler
 db 'format',0
 dw format_directive-assembler
 db 'fpatan',110011b
 dw simple_fpu_instruction-assembler
 db 'fprem1',110101b
 dw simple_fpu_instruction-assembler
 db 'frstor',4
 dw fnsave_instruction-assembler
 db 'fscale',111101b
 dw simple_fpu_instruction-assembler
 db 'fstenv',6
 dw fstenv_instruction-assembler
 db 'fsubrp',4
 dw faddp_instruction-assembler
 db 'fucomi',0E8h
 dw fcomi_instruction-assembler
 db 'fucomp',5
 dw ffree_instruction-assembler
 db 'fxsave',0
 dw fxsave_instruction-assembler
 db 'invlpg',0
 dw invlpg_instruction-assembler
 db 'lfence',0E8h
 dw fence_instruction-assembler
 db 'looped',0E1h
 dw loop_instruction_32bit-assembler
 db 'loopew',0E1h
 dw loop_instruction_16bit-assembler
 db 'loopne',0E0h
 dw loop_instruction-assembler
 db 'loopnz',0E0h
 dw loop_instruction-assembler
 db 'loopzd',0E1h
 dw loop_instruction_32bit-assembler
 db 'loopzw',0E1h
 dw loop_instruction_16bit-assembler
 db 'mfence',0F0h
 dw fence_instruction-assembler
 db 'movapd',28h
 dw movpd_instruction-assembler
 db 'movaps',28h
 dw movps_instruction-assembler
 db 'movdqa',66h
 dw movdq_instruction-assembler
 db 'movdqu',0F3h
 dw movdq_instruction-assembler
 db 'movhpd',16h
 dw movlpd_instruction-assembler
 db 'movhps',16h
 dw movlps_instruction-assembler
 db 'movlpd',12h
 dw movlpd_instruction-assembler
 db 'movlps',12h
 dw movlps_instruction-assembler
 db 'movnti',0C3h
 dw movnti_instruction-assembler
 db 'movntq',0E7h
 dw movntq_instruction-assembler
 db 'movupd',10h
 dw movpd_instruction-assembler
 db 'movups',10h
 dw movps_instruction-assembler
 db 'paddsb',0ECh
 dw mmx_instruction-assembler
 db 'paddsw',0EDh
 dw mmx_instruction-assembler
 db 'pextrw',0C5h
 dw pmovmskb_instruction-assembler
 db 'pfnacc',8Ah
 dw amd3dnow_instruction-assembler
 db 'pfsubr',0AAh
 dw amd3dnow_instruction-assembler
 db 'pinsrw',0C4h
 dw pinsrw_instruction-assembler
 db 'pmaxsw',0EEh
 dw mmx_instruction-assembler
 db 'pmaxub',0DEh
 dw mmx_instruction-assembler
 db 'pminsw',0EAh
 dw mmx_instruction-assembler
 db 'pminub',0DAh
 dw mmx_instruction-assembler
 db 'pmulhw',0E5h
 dw mmx_instruction-assembler
 db 'pmullw',0D5h
 dw mmx_instruction-assembler
 db 'psadbw',0F6h
 dw mmx_instruction-assembler
 db 'pshufd',66h
 dw pshufd_instruction-assembler
 db 'pshufw',0
 dw pshufw_instruction-assembler
 db 'pslldq',111b
 dw ps_dq_instruction-assembler
 db 'psrldq',011b
 dw ps_dq_instruction-assembler
 db 'psubsb',0E8h
 dw mmx_instruction-assembler
 db 'psubsw',0E9h
 dw mmx_instruction-assembler
 db 'pswapd',0BBh
 dw amd3dnow_instruction-assembler
 db 'public',0
 dw public_directive-assembler
 db 'pushad',60h
 dw simple_instruction_32bit-assembler
 db 'pushaw',60h
 dw simple_instruction_16bit-assembler
 db 'pushfd',9Ch
 dw simple_instruction_32bit-assembler
 db 'pushfw',9Ch
 dw simple_instruction_16bit-assembler
 db 'repeat',0
 dw repeat_directive-assembler
 db 'setnae',92h
 dw set_instruction-assembler
 db 'setnbe',97h
 dw set_instruction-assembler
 db 'setnge',9Ch
 dw set_instruction-assembler
 db 'setnle',9Fh
 dw set_instruction-assembler
 db 'sfence',0F8h
 dw fence_instruction-assembler
 db 'shufpd',0C6h
 dw sse_pd_instruction-assembler
 db 'shufps',0C6h
 dw sse_ps_instruction-assembler
 db 'sqrtpd',51h
 dw sse_pd_instruction-assembler
 db 'sqrtps',51h
 dw sse_ps_instruction-assembler
 db 'sqrtsd',51h
 dw sse_sd_instruction-assembler
 db 'sqrtss',51h
 dw sse_ss_instruction-assembler
 db 'wbinvd',9
 dw simple_extended_instruction-assembler
 db 0
instructions_7:
 db 'clflush',111b
 dw fxsave_instruction-assembler
 db 'cmovnae',42h
 dw cmov_instruction-assembler
 db 'cmovnbe',47h
 dw cmov_instruction-assembler
 db 'cmovnge',4Ch
 dw cmov_instruction-assembler
 db 'cmovnle',4Fh
 dw cmov_instruction-assembler
 db 'cmpeqpd',0
 dw cmp_pd_instruction-assembler
 db 'cmpeqps',0
 dw cmp_ps_instruction-assembler
 db 'cmpeqsd',0
 dw cmp_sd_instruction-assembler
 db 'cmpeqss',0
 dw cmp_ss_instruction-assembler
 db 'cmplepd',2
 dw cmp_pd_instruction-assembler
 db 'cmpleps',2
 dw cmp_ps_instruction-assembler
 db 'cmplesd',2
 dw cmp_sd_instruction-assembler
 db 'cmpless',2
 dw cmp_ss_instruction-assembler
 db 'cmpltpd',1
 dw cmp_pd_instruction-assembler
 db 'cmpltps',1
 dw cmp_ps_instruction-assembler
 db 'cmpltsd',1
 dw cmp_sd_instruction-assembler
 db 'cmpltss',1
 dw cmp_ss_instruction-assembler
 db 'cmpxchg',0B0h
 dw basic_486_instruction-assembler
 db 'display',0
 dw display_directive-assembler
 db 'fcmovbe',0D0h
 dw fcmov_instruction-assembler
 db 'fcmovnb',0C0h
 dw fcomi_instruction-assembler
 db 'fcmovne',0C8h
 dw fcomi_instruction-assembler
 db 'fcmovnu',0D8h
 dw fcomi_instruction-assembler
 db 'fdecstp',110110b
 dw simple_fpu_instruction-assembler
 db 'fincstp',110111b
 dw simple_fpu_instruction-assembler
 db 'fnstenv',6
 dw fldenv_instruction-assembler
 db 'frndint',111100b
 dw simple_fpu_instruction-assembler
 db 'fsincos',111011b
 dw simple_fpu_instruction-assembler
 db 'fucomip',0E8h
 dw fcomip_instruction-assembler
 db 'fucompp',0
 dw fucompp_instruction-assembler
 db 'fxrstor',1
 dw fxsave_instruction-assembler
 db 'fxtract',110100b
 dw simple_fpu_instruction-assembler
 db 'fyl2xp1',111001b
 dw simple_fpu_instruction-assembler
 db 'ldmxcsr',10b
 dw fxsave_instruction-assembler
 db 'loopned',0E0h
 dw loop_instruction_32bit-assembler
 db 'loopnew',0E0h
 dw loop_instruction_16bit-assembler
 db 'loopnzd',0E0h
 dw loop_instruction_32bit-assembler
 db 'loopnzw',0E0h
 dw loop_instruction_16bit-assembler
 db 'movdq2q',0
 dw movdq2q_instruction-assembler
 db 'movhlps',12h
 dw movhlps_instruction-assembler
 db 'movlhps',16h
 dw movhlps_instruction-assembler
 db 'movntdq',0E7h
 dw movntdq_instruction-assembler
 db 'movntpd',2Bh
 dw movntdq_instruction-assembler
 db 'movntps',2Bh
 dw movntps_instruction-assembler
 db 'movq2dq',0
 dw movq2dq_instruction-assembler
 db 'paddusb',0DCh
 dw mmx_instruction-assembler
 db 'paddusw',0DDh
 dw mmx_instruction-assembler
 db 'pavgusb',0BFh
 dw amd3dnow_instruction-assembler
 db 'pcmpeqb',74h
 dw mmx_instruction-assembler
 db 'pcmpeqd',76h
 dw mmx_instruction-assembler
 db 'pcmpeqw',75h
 dw mmx_instruction-assembler
 db 'pcmpgtb',64h
 dw mmx_instruction-assembler
 db 'pcmpgtd',66h
 dw mmx_instruction-assembler
 db 'pcmpgtw',65h
 dw mmx_instruction-assembler
 db 'pfcmpeq',0B0h
 dw amd3dnow_instruction-assembler
 db 'pfcmpge',90h
 dw amd3dnow_instruction-assembler
 db 'pfcmpgt',0A0h
 dw amd3dnow_instruction-assembler
 db 'pfpnacc',8Eh
 dw amd3dnow_instruction-assembler
 db 'pfrsqrt',97h
 dw amd3dnow_instruction-assembler
 db 'pmaddwd',0F5h
 dw mmx_instruction-assembler
 db 'pmulhrw',0B7h
 dw amd3dnow_instruction-assembler
 db 'pmulhuw',0E4h
 dw mmx_instruction-assembler
 db 'pmuludq',0F4h
 dw mmx_instruction-assembler
 db 'pshufhw',0F3h
 dw pshufd_instruction-assembler
 db 'pshuflw',0F2h
 dw pshufd_instruction-assembler
 db 'psubusb',0D8h
 dw mmx_instruction-assembler
 db 'psubusw',0D9h
 dw mmx_instruction-assembler
 db 'rsqrtps',52h
 dw sse_ps_instruction-assembler
 db 'rsqrtss',52h
 dw sse_ss_instruction-assembler
 db 'section',0
 dw section_directive-assembler
 db 'segment',0
 dw segment_directive-assembler
 db 'stmxcsr',11b
 dw fxsave_instruction-assembler
 db 'sysexit',35h
 dw simple_extended_instruction-assembler
 db 'ucomisd',2Eh
 dw comisd_instruction-assembler
 db 'ucomiss',2Eh
 dw comiss_instruction-assembler
 db 'virtual',0
 dw virtual_directive-assembler
 db 0
instructions_8:
 db 'cmpneqpd',4
 dw cmp_pd_instruction-assembler
 db 'cmpneqps',4
 dw cmp_ps_instruction-assembler
 db 'cmpneqsd',4
 dw cmp_sd_instruction-assembler
 db 'cmpneqss',4
 dw cmp_ss_instruction-assembler
 db 'cmpnlepd',6
 dw cmp_pd_instruction-assembler
 db 'cmpnleps',6
 dw cmp_ps_instruction-assembler
 db 'cmpnlesd',6
 dw cmp_sd_instruction-assembler
 db 'cmpnless',6
 dw cmp_ss_instruction-assembler
 db 'cmpnltpd',5
 dw cmp_pd_instruction-assembler
 db 'cmpnltps',5
 dw cmp_ps_instruction-assembler
 db 'cmpnltsd',5
 dw cmp_sd_instruction-assembler
 db 'cmpnltss',5
 dw cmp_ss_instruction-assembler
 db 'cmpordpd',7
 dw cmp_pd_instruction-assembler
 db 'cmpordps',7
 dw cmp_ps_instruction-assembler
 db 'cmpordsd',7
 dw cmp_sd_instruction-assembler
 db 'cmpordss',7
 dw cmp_ss_instruction-assembler
 db 'cvtdq2pd',0E6h
 dw cvtdq2pd_instruction-assembler
 db 'cvtdq2ps',5Bh
 dw sse_ps_instruction-assembler
 db 'cvtpd2dq',0E6h
 dw cvtpd2dq_instruction-assembler
 db 'cvtpd2pi',2Dh
 dw cvtpd2pi_instruction-assembler
 db 'cvtpd2ps',5Ah
 dw sse_pd_instruction-assembler
 db 'cvtpi2pd',2Ah
 dw cvtpi2pd_instruction-assembler
 db 'cvtpi2ps',2Ah
 dw cvtpi2ps_instruction-assembler
 db 'cvtps2dq',5Bh
 dw sse_pd_instruction-assembler
 db 'cvtps2pd',5Ah
 dw cvtps2pd_instruction-assembler
 db 'cvtps2pi',2Dh
 dw cvtps2pi_instruction-assembler
 db 'cvtsd2si',2Dh
 dw cvtsd2si_instruction-assembler
 db 'cvtsd2ss',5Ah
 dw sse_sd_instruction-assembler
 db 'cvtsi2sd',2Ah
 dw cvtsi2sd_instruction-assembler
 db 'cvtsi2ss',2Ah
 dw cvtsi2ss_instruction-assembler
 db 'cvtss2sd',5Ah
 dw sse_ss_instruction-assembler
 db 'cvtss2si',2Dh
 dw cvtss2si_instruction-assembler
 db 'fcmovnbe',0D0h
 dw fcomi_instruction-assembler
 db 'maskmovq',0
 dw maskmovq_instruction-assembler
 db 'movmskpd',0
 dw movmskpd_instruction-assembler
 db 'movmskps',0
 dw movmskps_instruction-assembler
 db 'packssdw',6Bh
 dw mmx_instruction-assembler
 db 'packsswb',63h
 dw mmx_instruction-assembler
 db 'packuswb',67h
 dw mmx_instruction-assembler
 db 'pfrcpit1',0A6h
 dw amd3dnow_instruction-assembler
 db 'pfrcpit2',0B6h
 dw amd3dnow_instruction-assembler
 db 'pfrsqit1',0A7h
 dw amd3dnow_instruction-assembler
 db 'pmovmskb',0D7h
 dw pmovmskb_instruction-assembler
 db 'prefetch',0
 dw amd_prefetch_instruction-assembler
 db 'sysenter',34h
 dw simple_extended_instruction-assembler
 db 'unpckhpd',15h
 dw sse_pd_instruction-assembler
 db 'unpckhps',15h
 dw sse_ps_instruction-assembler
 db 'unpcklpd',14h
 dw sse_pd_instruction-assembler
 db 'unpcklps',14h
 dw sse_ps_instruction-assembler
 db 0
instructions_9:
 db 'cmpxchg8b',0
 dw cmpxchg8b_instruction-assembler
 db 'cvttpd2dq',0E6h
 dw sse_pd_instruction-assembler
 db 'cvttpd2pi',2Ch
 dw cvtpd2pi_instruction-assembler
 db 'cvttps2dq',5Bh
 dw cvtdq2pd_instruction-assembler
 db 'cvttps2pi',2Ch
 dw cvtps2pi_instruction-assembler
 db 'cvttsd2si',2Ch
 dw cvtsd2si_instruction-assembler
 db 'cvttss2si',2Ch
 dw cvtss2si_instruction-assembler
 db 'prefetchw',1
 dw amd_prefetch_instruction-assembler
 db 'punpckhbw',68h
 dw mmx_instruction-assembler
 db 'punpckhdq',6Ah
 dw mmx_instruction-assembler
 db 'punpckhwd',69h
 dw mmx_instruction-assembler
 db 'punpcklbw',60h
 dw mmx_instruction-assembler
 db 'punpckldq',62h
 dw mmx_instruction-assembler
 db 'punpcklwd',61h
 dw mmx_instruction-assembler
 db 0
instructions_10:
 db 'cmpunordpd',3
 dw cmp_pd_instruction-assembler
 db 'cmpunordps',3
 dw cmp_ps_instruction-assembler
 db 'cmpunordsd',3
 dw cmp_sd_instruction-assembler
 db 'cmpunordss',3
 dw cmp_ss_instruction-assembler
 db 'maskmovdqu',0
 dw maskmovdqu_instruction-assembler
 db 'prefetcht0',1
 dw prefetch_instruction-assembler
 db 'prefetcht1',2
 dw prefetch_instruction-assembler
 db 'prefetcht2',3
 dw prefetch_instruction-assembler
 db 'punpckhqdq',6Dh
 dw sse_pd_instruction-assembler
 db 'punpcklqdq',6Ch
 dw sse_pd_instruction-assembler
 db 0
instructions_11:
 db 'prefetchnta',0
 dw prefetch_instruction-assembler
 db 0
