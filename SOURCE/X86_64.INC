
; flat assembler core
; Copyright (c) 1999-2005, Tomasz Grysztar.
; All rights reserved.

simple_instruction_except64:
	cmp	[code_type],64
	je	illegal_instruction
simple_instruction:
	stos	byte [edi]
	jmp	instruction_assembled
simple_instruction_only64:
	cmp	[code_type],64
	jne	illegal_instruction
	jmp	simple_instruction
simple_instruction_16bit_except64:
	cmp	[code_type],64
	je	illegal_instruction
simple_instruction_16bit:
	cmp	[code_type],16
	jne	size_prefix
	stos	byte [edi]
	jmp	instruction_assembled
      size_prefix:
	mov	ah,al
	mov	al,66h
	stos	word [edi]
	jmp	instruction_assembled
simple_instruction_32bit_except64:
	cmp	[code_type],64
	je	illegal_instruction
simple_instruction_32bit:
	cmp	[code_type],16
	je	size_prefix
	stos	byte [edi]
	jmp	instruction_assembled
simple_instruction_64bit:
	cmp	[code_type],64
	jne	illegal_instruction
	mov	ah,al
	mov	al,48h
	stos	word [edi]
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
iret_instruction:
	cmp	[code_type],64
	jne	simple_instruction
	call	operand_64bit
	jmp	simple_instruction
aa_instruction:
	cmp	[code_type],64
	je	illegal_instruction
	push	eax
	mov	bl,10
	cmp	byte [esi],'('
	jne	aa_store
	inc	esi
	xor	al,al
	xchg	al,[operand_size]
	cmp	al,1
	ja	invalid_operand_size
	call	get_byte_value
	mov	bl,al
      aa_store:
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
	call	operand_autodetect
	inc	[base_code]
      basic_mem_reg_8bit:
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
	cmp	al,8
	je	basic_mem_imm_64bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[error_line],0
	jne	basic_mem_imm_8bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
      basic_mem_imm_8bit:
	call	get_byte_value
	mov	byte [value],al
	mov	al,[base_code]
	shr	al,3
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	[base_code],80h
	call	store_instruction_with_imm8
	jmp	instruction_assembled
      basic_mem_imm_16bit:
	call	operand_16bit
	call	get_word_value
	mov	word [value],ax
	mov	al,[base_code]
	shr	al,3
	mov	[postbyte_register],al
	pop	cx bx edx
	cmp	[value_type],0
	jne	basic_mem_imm_16bit_store
	cmp	[size_declared],0
	jne	basic_mem_imm_16bit_store
	cmp	word [value],80h
	jb	basic_mem_simm_8bit
	cmp	word [value],-80h
	jae	basic_mem_simm_8bit
      basic_mem_imm_16bit_store:
	mov	[base_code],81h
	call	store_instruction_with_imm16
	jmp	instruction_assembled
      basic_mem_simm_8bit:
	mov	[base_code],83h
	call	store_instruction_with_imm8
	jmp	instruction_assembled
      basic_mem_imm_32bit:
	call	operand_32bit
	call	get_dword_value
      basic_mem_imm_32bit_ok:
	mov	dword [value],eax
	mov	al,[base_code]
	shr	al,3
	mov	[postbyte_register],al
	pop	cx bx edx
	cmp	[value_type],0
	jne	basic_mem_imm_32bit_store
	cmp	[size_declared],0
	jne	basic_mem_imm_32bit_store
	cmp	dword [value],80h
	jb	basic_mem_simm_8bit
	cmp	dword [value],-80h
	jae	basic_mem_simm_8bit
      basic_mem_imm_32bit_store:
	mov	[base_code],81h
	call	store_instruction_with_imm32
	jmp	instruction_assembled
      basic_mem_imm_64bit:
	cmp	[size_declared],0
	jne	long_immediate_not_encodable
	cmp	[value_type],4
	jae	long_immediate_not_encodable
	call	operand_64bit
	call	get_simm32
	jmp	basic_mem_imm_32bit_ok
      get_simm32:
	call	get_qword_value
	mov	ecx,edx
	cdq
	cmp	ecx,edx
	jne	value_out_of_range
	ret
      basic_reg:
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
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
	call	operand_autodetect
	add	[base_code],3
	call	store_instruction
	jmp	instruction_assembled
      basic_reg_mem_8bit:
	add	[base_code],2
	call	store_instruction
	jmp	instruction_assembled
      basic_reg_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,[postbyte_register]
	mov	[postbyte_register],al
	mov	al,ah
	cmp	al,1
	je	basic_reg_reg_8bit
	call	operand_autodetect
	inc	[base_code]
      basic_reg_reg_8bit:
	call	store_nomem_instruction
	jmp	instruction_assembled
      basic_reg_imm:
	mov	al,[operand_size]
	cmp	al,1
	je	basic_reg_imm_8bit
	cmp	al,2
	je	basic_reg_imm_16bit
	cmp	al,4
	je	basic_reg_imm_32bit
	cmp	al,8
	je	basic_reg_imm_64bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[error_line],0
	jne	basic_reg_imm_32bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
	jmp	basic_reg_imm_32bit
      basic_reg_imm_8bit:
	call	get_byte_value
	mov	dl,al
	mov	bl,[base_code]
	shr	bl,3
	xchg	bl,[postbyte_register]
	or	bl,bl
	jz	basic_al_imm
	mov	[base_code],80h
	call	store_nomem_instruction
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
	call	operand_16bit
	call	get_word_value
	mov	dx,ax
	mov	bl,[base_code]
	shr	bl,3
	xchg	bl,[postbyte_register]
	cmp	[value_type],0
	jne	basic_reg_imm_16bit_store
	cmp	[size_declared],0
	jne	basic_reg_imm_16bit_store
	cmp	dx,80h
	jb	basic_reg_simm_8bit
	cmp	dx,-80h
	jae	basic_reg_simm_8bit
      basic_reg_imm_16bit_store:
	or	bl,bl
	jz	basic_ax_imm
	mov	[base_code],81h
	call	store_nomem_instruction
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      basic_reg_simm_8bit:
	mov	[base_code],83h
	call	store_nomem_instruction
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      basic_ax_imm:
	add	[base_code],5
	call	store_instruction_code
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      basic_reg_imm_32bit:
	call	operand_32bit
	call	get_dword_value
      basic_reg_imm_32bit_ok:
	mov	edx,eax
	mov	bl,[base_code]
	shr	bl,3
	xchg	bl,[postbyte_register]
	cmp	[value_type],0
	jne	basic_reg_imm_32bit_store
	cmp	[size_declared],0
	jne	basic_reg_imm_32bit_store
	cmp	edx,80h
	jb	basic_reg_simm_8bit
	cmp	edx,-80h
	jae	basic_reg_simm_8bit
      basic_reg_imm_32bit_store:
	or	bl,bl
	jz	basic_eax_imm
	mov	[base_code],81h
	call	store_nomem_instruction
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      basic_eax_imm:
	add	[base_code],5
	call	store_instruction_code
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      basic_reg_imm_64bit:
	cmp	[size_declared],0
	jne	long_immediate_not_encodable
	cmp	[value_type],4
	jae	long_immediate_not_encodable
	call	operand_64bit
	call	get_simm32
	jmp	basic_reg_imm_32bit_ok
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
	jb	single_mem_nosize
	call	operand_autodetect
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      single_mem_nosize:
	cmp	[error_line],0
	jne	single_mem_8bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
      single_mem_8bit:
	call	store_instruction
	jmp	instruction_assembled
      single_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	mov	al,ah
	cmp	al,1
	je	single_reg_8bit
	call	operand_autodetect
	inc	[base_code]
      single_reg_8bit:
	call	store_nomem_instruction
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
	jb	mov_mem_general_reg
	cmp	al,70h
	jb	mov_mem_sreg
      mov_mem_general_reg:
	call	convert_register
	mov	[postbyte_register],al
	pop	cx bx edx
	cmp	ah,1
	je	mov_mem_reg_8bit
	mov	al,ah
	call	operand_autodetect
	mov	al,[postbyte_register]
	or	al,bl
	or	al,bh
	jz	mov_mem_ax
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      mov_mem_reg_8bit:
	or	al,bl
	or	al,bh
	jz	mov_mem_al
	call	store_instruction
	jmp	instruction_assembled
      mov_mem_al:
	test	ch,22h
	jnz	mov_mem_address16_al
	test	ch,44h
	jnz	mov_mem_address32_al
	test	ch,88h
	jnz	mov_mem_address64_al
	or	ch,ch
	jnz	invalid_address_size
	cmp	[code_type],64
	je	mov_mem_address64_al
	cmp	[code_type],32
	je	mov_mem_address32_al
	cmp	edx,10000h
	jb	mov_mem_address16_al
      mov_mem_address32_al:
	call	store_segment_prefix_if_necessary
	call	address_32bit_prefix
	mov	[base_code],0A2h
      store_mov_address32:
	call	store_instruction_code
	push	instruction_assembled
	jmp	store_address_32bit_value
      mov_mem_address16_al:
	call	store_segment_prefix_if_necessary
	call	address_16bit_prefix
	mov	[base_code],0A2h
      store_mov_address16:
	cmp	[code_type],64
	je	invalid_address
	call	store_instruction_code
	mov	eax,edx
	stos	word [edi]
	cmp	edx,10000h
	jge	value_out_of_range
	jmp	instruction_assembled
      mov_mem_address64_al:
	call	store_segment_prefix_if_necessary
	mov	[base_code],0A2h
      store_mov_address64:
	call	store_instruction_code
	push	instruction_assembled
	jmp	store_address_64bit_value
      mov_mem_ax:
	test	ch,22h
	jnz	mov_mem_address16_ax
	test	ch,44h
	jnz	mov_mem_address32_ax
	test	ch,88h
	jnz	mov_mem_address64_ax
	or	ch,ch
	jnz	invalid_address_size
	cmp	[code_type],64
	je	mov_mem_address64_ax
	cmp	[code_type],32
	je	mov_mem_address32_ax
	cmp	edx,10000h
	jb	mov_mem_address16_ax
      mov_mem_address32_ax:
	call	store_segment_prefix_if_necessary
	call	address_32bit_prefix
	mov	[base_code],0A3h
	jmp	store_mov_address32
      mov_mem_address16_ax:
	call	store_segment_prefix_if_necessary
	call	address_16bit_prefix
	mov	[base_code],0A3h
	jmp	store_mov_address16
      mov_mem_address64_ax:
	call	store_segment_prefix_if_necessary
	mov	[base_code],0A3h
	jmp	store_mov_address64
      mov_mem_sreg:
	sub	al,61h
	mov	[postbyte_register],al
	pop	cx bx edx
	mov	ah,[operand_size]
	or	ah,ah
	jz	mov_mem_sreg_store
	cmp	ah,2
	jne	invalid_operand_size
      mov_mem_sreg_store:
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
	cmp	al,8
	je	mov_mem_imm_64bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[error_line],0
	jne	mov_mem_imm_32bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
	jmp	mov_mem_imm_32bit
      mov_mem_imm_8bit:
	call	get_byte_value
	mov	byte [value],al
	mov	[postbyte_register],0
	mov	[base_code],0C6h
	pop	cx bx edx
	call	store_instruction_with_imm8
	jmp	instruction_assembled
      mov_mem_imm_16bit:
	call	operand_16bit
	call	get_word_value
	mov	word [value],ax
	mov	[postbyte_register],0
	mov	[base_code],0C7h
	pop	cx bx edx
	call	store_instruction_with_imm16
	jmp	instruction_assembled
      mov_mem_imm_32bit:
	call	operand_32bit
	call	get_dword_value
      mov_mem_imm_32bit_store:
	mov	dword [value],eax
	mov	[postbyte_register],0
	mov	[base_code],0C7h
	pop	cx bx edx
	call	store_instruction_with_imm32
	jmp	instruction_assembled
      mov_mem_imm_64bit:
	cmp	[size_declared],0
	jne	long_immediate_not_encodable
	cmp	[value_type],4
	jae	long_immediate_not_encodable
	call	operand_64bit
	call	get_simm32
	jmp	mov_mem_imm_32bit_store
      mov_reg:
	lods	byte [esi]
	mov	ah,al
	sub	ah,10h
	and	ah,al
	test	ah,0F0h
	jnz	mov_sreg
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
	mov	ah,al
	sub	ah,10h
	and	ah,al
	test	ah,0F0h
	jnz	mov_reg_sreg
	call	convert_register
	mov	bl,[postbyte_register]
	mov	[postbyte_register],al
	mov	al,ah
	cmp	al,1
	je	mov_reg_reg_8bit
	call	operand_autodetect
	inc	[base_code]
      mov_reg_reg_8bit:
	call	store_nomem_instruction
	jmp	instruction_assembled
      mov_reg_sreg:
	mov	bl,[postbyte_register]
	mov	ah,al
	and	al,1111b
	mov	[postbyte_register],al
	shr	ah,4
	cmp	ah,5
	je	mov_reg_creg
	cmp	ah,7
	je	mov_reg_dreg
	ja	mov_reg_treg
	dec	[postbyte_register]
	cmp	[operand_size],8
	je	mov_reg_sreg64
	cmp	[operand_size],4
	je	mov_reg_sreg32
	cmp	[operand_size],2
	jne	invalid_operand_size
	call	operand_16bit
	jmp	mov_reg_sreg_store
      mov_reg_sreg64:
	call	operand_64bit
	jmp	mov_reg_sreg_store
      mov_reg_sreg32:
	call	operand_32bit
      mov_reg_sreg_store:
	mov	[base_code],8Ch
	call	store_nomem_instruction
	jmp	instruction_assembled
      mov_reg_treg:
	cmp	ah,9
	jne	invalid_operand
	mov	[extended_code],24h
	jmp	mov_reg_xrx
      mov_reg_dreg:
	mov	[extended_code],21h
	jmp	mov_reg_xrx
      mov_reg_creg:
	mov	[extended_code],20h
      mov_reg_xrx:
	mov	[base_code],0Fh
	cmp	[code_type],64
	je	mov_reg_xrx_64bit
	cmp	[operand_size],4
	jne	invalid_operand_size
	call	store_nomem_instruction
	jmp	instruction_assembled
      mov_reg_xrx_64bit:
	cmp	[operand_size],8
	jne	invalid_operand_size
	call	store_nomem_instruction
	jmp	instruction_assembled
      mov_reg_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	mov_reg_mem_8bit
	call	operand_autodetect
	mov	al,[postbyte_register]
	or	al,bl
	or	al,bh
	jz	mov_ax_mem
	add	[base_code],3
	call	store_instruction
	jmp	instruction_assembled
      mov_reg_mem_8bit:
	mov	al,[postbyte_register]
	or	al,bl
	or	al,bh
	jz	mov_al_mem
	add	[base_code],2
	call	store_instruction
	jmp	instruction_assembled
      mov_al_mem:
	test	ch,22h
	jnz	mov_al_mem_address16
	test	ch,44h
	jnz	mov_al_mem_address32
	test	ch,88h
	jnz	mov_al_mem_address64
	or	ch,ch
	jnz	invalid_address_size
	cmp	[code_type],64
	je	mov_al_mem_address64
	cmp	[code_type],32
	je	mov_al_mem_address32
	cmp	edx,10000h
	jb	mov_al_mem_address16
      mov_al_mem_address32:
	call	store_segment_prefix_if_necessary
	call	address_32bit_prefix
	mov	[base_code],0A0h
	jmp	store_mov_address32
      mov_al_mem_address16:
	call	store_segment_prefix_if_necessary
	call	address_16bit_prefix
	mov	[base_code],0A0h
	jmp	store_mov_address16
      mov_al_mem_address64:
	call	store_segment_prefix_if_necessary
	mov	[base_code],0A0h
	jmp	store_mov_address64
      mov_ax_mem:
	test	ch,22h
	jnz	mov_ax_mem_address16
	test	ch,44h
	jnz	mov_ax_mem_address32
	test	ch,88h
	jnz	mov_ax_mem_address64
	or	ch,ch
	jnz	invalid_address_size
	cmp	[code_type],64
	je	mov_ax_mem_address64
	cmp	[code_type],32
	je	mov_ax_mem_address32
	cmp	edx,10000h
	jb	mov_ax_mem_address16
      mov_ax_mem_address32:
	call	store_segment_prefix_if_necessary
	call	address_32bit_prefix
	mov	[base_code],0A1h
	jmp	store_mov_address32
      mov_ax_mem_address16:
	call	store_segment_prefix_if_necessary
	mov	[base_code],0A1h
	jmp	store_mov_address16
      mov_ax_mem_address64:
	call	store_segment_prefix_if_necessary
	mov	[base_code],0A1h
	jmp	store_mov_address64
      mov_reg_imm:
	mov	al,[operand_size]
	cmp	al,1
	je	mov_reg_imm_8bit
	cmp	al,2
	je	mov_reg_imm_16bit
	cmp	al,4
	je	mov_reg_imm_32bit
	cmp	al,8
	je	mov_reg_imm_64bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[error_line],0
	jne	mov_reg_imm_32bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
	jmp	mov_reg_imm_32bit
      mov_reg_imm_8bit:
	call	get_byte_value
	mov	dl,al
	mov	al,0B0h
	call	store_mov_reg_imm_code
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      mov_reg_imm_16bit:
	call	get_word_value
	mov	dx,ax
	call	operand_16bit
	mov	al,0B8h
	call	store_mov_reg_imm_code
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      mov_reg_imm_32bit:
	call	operand_32bit
	call	get_dword_value
	mov	edx,eax
	mov	al,0B8h
	call	store_mov_reg_imm_code
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      mov_reg_imm_64bit:
	call	operand_64bit
	call	get_qword_value
	mov	ecx,edx
	cmp	[size_declared],0
	jne	mov_reg_imm_64bit_store
	cmp	[value_type],4
	jae	mov_reg_imm_64bit_store
	cdq
	cmp	ecx,edx
	je	mov_reg_64bit_imm_32bit
      mov_reg_imm_64bit_store:
	push	eax ecx
	mov	al,0B8h
	call	store_mov_reg_imm_code
	pop	edx eax
	call	mark_relocation
	stos	dword [edi]
	mov	eax,edx
	stos	dword [edi]
	jmp	instruction_assembled
      store_mov_reg_imm_code:
	mov	ah,[postbyte_register]
	test	ah,1000b
	jz	mov_reg_imm_prefix_ok
	or	[rex_prefix],41h
      mov_reg_imm_prefix_ok:
	and	ah,111b
	add	al,ah
	mov	[base_code],al
	call	store_instruction_code
	ret
      mov_reg_64bit_imm_32bit:
	mov	edx,eax
	mov	bl,[postbyte_register]
	mov	[postbyte_register],0
	mov	[base_code],0C7h
	call	store_nomem_instruction
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      mov_sreg:
	mov	ah,al
	and	al,111b
	mov	[postbyte_register],al
	shr	ah,4
	cmp	ah,5
	je	mov_creg
	cmp	ah,7
	je	mov_dreg
	ja	mov_treg
	cmp	al,2
	je	illegal_instruction
	dec	[postbyte_register]
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
	cmp	ah,2
	jne	invalid_operand_size
	mov	bl,al
      mov_sreg_reg_size_ok:
	mov	[base_code],8Eh
	call	store_nomem_instruction
	jmp	instruction_assembled
      mov_sreg_mem:
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	mov_sreg_mem_size_ok
	cmp	al,2
	jne	invalid_operand_size
      mov_sreg_mem_size_ok:
	mov	[base_code],8Eh
	call	store_instruction
	jmp	instruction_assembled
      mov_treg:
	cmp	ah,9
	jne	invalid_operand
	mov	[extended_code],26h
	jmp	mov_xrx
      mov_dreg:
	mov	[extended_code],23h
	jmp	mov_xrx
      mov_creg:
	mov	[extended_code],22h
      mov_xrx:
	mov	[base_code],0Fh
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	cmp	[code_type],64
	je	mov_xrx_64bit
	cmp	ah,4
	jne	invalid_operand_size
	call	store_nomem_instruction
	jmp	instruction_assembled
      mov_xrx_64bit:
	cmp	ah,8
	jne	invalid_operand_size
	call	store_nomem_instruction
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
	mov	bl,al
	mov	al,ah
	call	operand_autodetect
	call	store_nomem_instruction
	jmp	instruction_assembled
      cmov_reg_mem:
	call	get_address
	mov	al,[operand_size]
	call	operand_autodetect
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
	call	operand_autodetect
	inc	[base_code]
      test_mem_reg_8bit:
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
	cmp	al,8
	je	test_mem_imm_64bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[error_line],0
	jne	test_mem_imm_32bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
	jmp	test_mem_imm_32bit
      test_mem_imm_8bit:
	call	get_byte_value
	mov	byte [value],al
	mov	[postbyte_register],0
	mov	[base_code],0F6h
	pop	cx bx edx
	call	store_instruction_with_imm8
	jmp	instruction_assembled
      test_mem_imm_16bit:
	call	operand_16bit
	call	get_word_value
	mov	word [value],ax
	mov	[postbyte_register],0
	mov	[base_code],0F7h
	pop	cx bx edx
	call	store_instruction_with_imm16
	jmp	instruction_assembled
      test_mem_imm_32bit:
	call	operand_32bit
	call	get_dword_value
      test_mem_imm_32bit_store:
	mov	dword [value],eax
	mov	[postbyte_register],0
	mov	[base_code],0F7h
	pop	cx bx edx
	call	store_instruction_with_imm32
	jmp	instruction_assembled
      test_mem_imm_64bit:
	cmp	[size_declared],0
	jne	long_immediate_not_encodable
	cmp	[value_type],4
	jae	long_immediate_not_encodable
	call	operand_64bit
	call	get_simm32
	jmp	test_mem_imm_32bit_store
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
	mov	bl,[postbyte_register]
	mov	[postbyte_register],al
	mov	al,ah
	cmp	al,1
	je	test_reg_reg_8bit
	call	operand_autodetect
	inc	[base_code]
      test_reg_reg_8bit:
	call	store_nomem_instruction
	jmp	instruction_assembled
      test_reg_imm:
	mov	al,[operand_size]
	cmp	al,1
	je	test_reg_imm_8bit
	cmp	al,2
	je	test_reg_imm_16bit
	cmp	al,4
	je	test_reg_imm_32bit
	cmp	al,8
	je	test_reg_imm_64bit
	jmp	invalid_operand_size
      test_reg_imm_8bit:
	call	get_byte_value
	mov	dl,al
	mov	bl,[postbyte_register]
	mov	[postbyte_register],0
	mov	[base_code],0F6h
	or	bl,bl
	jz	test_al_imm
	call	store_nomem_instruction
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      test_al_imm:
	mov	[base_code],0A8h
	call	store_instruction_code
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      test_reg_imm_16bit:
	call	operand_16bit
	call	get_word_value
	mov	dx,ax
	mov	bl,[postbyte_register]
	mov	[postbyte_register],0
	mov	[base_code],0F7h
	or	bl,bl
	jz	test_ax_imm
	call	store_nomem_instruction
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      test_ax_imm:
	mov	[base_code],0A9h
	call	store_instruction_code
	mov	ax,dx
	stos	word [edi]
	jmp	instruction_assembled
      test_reg_imm_32bit:
	call	operand_32bit
	call	get_dword_value
      test_reg_imm_32bit_store:
	mov	edx,eax
	mov	bl,[postbyte_register]
	mov	[postbyte_register],0
	mov	[base_code],0F7h
	or	bl,bl
	jz	test_eax_imm
	call	store_nomem_instruction
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      test_eax_imm:
	mov	[base_code],0A9h
	call	store_instruction_code
	mov	eax,edx
	stos	dword [edi]
	jmp	instruction_assembled
      test_reg_imm_64bit:
	cmp	[size_declared],0
	jne	long_immediate_not_encodable
	cmp	[value_type],4
	jae	long_immediate_not_encodable
	call	operand_64bit
	call	get_simm32
	jmp	test_reg_imm_32bit_store
      test_reg_mem:
	call	get_address
	mov	al,[operand_size]
	cmp	al,1
	je	test_reg_mem_8bit
	call	operand_autodetect
	inc	[base_code]
      test_reg_mem_8bit:
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
	mov	bl,al
	mov	al,ah
	cmp	al,1
	je	xchg_reg_reg_8bit
	call	operand_autodetect
	cmp	[postbyte_register],0
	je	xchg_ax_reg
	or	bl,bl
	jnz	xchg_reg_reg_store
	mov	bl,[postbyte_register]
      xchg_ax_reg:
	cmp	[code_type],64
	jne	xchg_ax_reg_ok
	cmp	ah,4
	jne	xchg_ax_reg_ok
	or	bl,bl
	jz	xchg_reg_reg_store
      xchg_ax_reg_ok:
	test	bl,1000b
	jz	xchg_ax_reg_store
	or	[rex_prefix],41h
	and	bl,111b
      xchg_ax_reg_store:
	add	bl,90h
	mov	[base_code],bl
	call	store_instruction_code
	jmp	instruction_assembled
      xchg_reg_reg_store:
	inc	[base_code]
      xchg_reg_reg_8bit:
	call	store_nomem_instruction
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
	cmp	al,8
	je	push_mem_64bit
	or	al,al
	jnz	invalid_operand_size
	cmp	ah,2
	je	push_mem_16bit
	cmp	ah,4
	je	push_mem_32bit
	cmp	ah,8
	je	push_mem_64bit
	cmp	[error_line],0
	jne	push_mem_store
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
	jmp	push_mem_store
      push_mem_16bit:
	test	ah,not 2
	jnz	invalid_operand_size
	call	operand_16bit
	jmp	push_mem_store
      push_mem_32bit:
	test	ah,not 4
	jnz	invalid_operand_size
	cmp	[code_type],64
	je	illegal_instruction
	call	operand_32bit
	jmp	push_mem_store
      push_mem_64bit:
	test	ah,not 8
	jnz	invalid_operand_size
	cmp	[code_type],64
	jne	illegal_instruction
      push_mem_store:
	mov	[base_code],0FFh
	mov	[postbyte_register],110b
	call	store_instruction
	jmp	push_done
      push_reg:
	lods	byte [esi]
	mov	ah,al
	sub	ah,10h
	and	ah,al
	test	ah,0F0h
	jnz	push_sreg
	call	convert_register
	test	al,1000b
	jz	push_reg_ok
	or	[rex_prefix],41h
	and	al,111b
      push_reg_ok:
	add	al,50h
	mov	[base_code],al
	mov	al,ah
	mov	ah,[extended_code]
	cmp	al,2
	je	push_reg_16bit
	cmp	al,4
	je	push_reg_32bit
	cmp	al,8
	jne	invalid_operand_size
      push_reg_64bit:
	test	ah,not 8
	jnz	invalid_operand_size
	cmp	[code_type],64
	jne	illegal_instruction
	jmp	push_reg_store
      push_reg_32bit:
	test	ah,not 4
	jnz	invalid_operand_size
	cmp	[code_type],64
	je	illegal_instruction
	call	operand_32bit
	jmp	push_reg_store
      push_reg_16bit:
	test	ah,not 2
	jnz	invalid_operand_size
	call	operand_16bit
      push_reg_store:
	call	store_instruction_code
	jmp	push_done
      push_sreg:
	mov	bl,al
	mov	dl,[operand_size]
	mov	dh,[extended_code]
	cmp	dl,2
	je	push_sreg16
	cmp	dl,4
	je	push_sreg32
	cmp	dl,8
	je	push_sreg64
	or	dl,dl
	jnz	invalid_operand_size
	cmp	dh,2
	je	push_sreg16
	cmp	dh,4
	je	push_sreg32
	cmp	dh,8
	je	push_sreg64
	jmp	push_sreg_store
      push_sreg16:
	test	dh,not 2
	jnz	invalid_operand_size
	call	operand_16bit
	jmp	push_sreg_store
      push_sreg32:
	test	dh,not 4
	jnz	invalid_operand_size
	cmp	[code_type],64
	je	illegal_instruction
	call	operand_32bit
	jmp	push_sreg_store
      push_sreg64:
	test	dh,not 8
	jnz	invalid_operand_size
	cmp	[code_type],64
	jne	illegal_instruction
      push_sreg_store:
	mov	al,bl
	cmp	al,70h
	jae	invalid_operand
	sub	al,61h
	cmp	al,4
	jae	push_sreg_386
	shl	al,3
	add	al,6
	mov	[base_code],al
	cmp	[code_type],64
	je	illegal_instruction
	jmp	push_reg_store
      push_sreg_386:
	sub	al,4
	shl	al,3
	add	al,0A0h
	mov	[extended_code],al
	mov	[base_code],0Fh
	jmp	push_reg_store
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
	cmp	al,8
	je	push_imm_64bit
	cmp	ah,2
	je	push_imm_optimized_16bit
	cmp	ah,4
	je	push_imm_optimized_32bit
	cmp	ah,8
	je	push_imm_optimized_64bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[code_type],16
	je	push_imm_optimized_16bit
	cmp	[code_type],32
	je	push_imm_optimized_32bit
      push_imm_optimized_64bit:
	cmp	[code_type],64
	jne	illegal_instruction
	call	get_simm32
	mov	edx,eax
	cmp	[value_type],0
	jne	push_imm_32bit_store
	cmp	eax,-80h
	jl	push_imm_32bit_store
	cmp	eax,80h
	jge	push_imm_32bit_store
	jmp	push_imm_8bit
      push_imm_optimized_32bit:
	cmp	[code_type],64
	je	illegal_instruction
	call	get_dword_value
	mov	edx,eax
	cmp	[value_type],0
	jne	push_imm_32bit_store
	cmp	eax,-80h
	jl	push_imm_32bit_store
	cmp	eax,80h
	jge	push_imm_32bit_store
	call	operand_32bit
	jmp	push_imm_8bit
      push_imm_optimized_16bit:
	call	get_word_value
	mov	dx,ax
	cmp	[value_type],0
	jne	push_imm_16bit_store
	cmp	ax,-80h
	jl	push_imm_16bit_store
	cmp	ax,80h
	jge	push_imm_16bit_store
	call	operand_16bit
      push_imm_8bit:
	mov	ah,al
	mov	al,6Ah
	stos	word [edi]
	jmp	push_done
      push_imm_16bit:
	call	get_word_value
	mov	dx,ax
	call	operand_16bit
      push_imm_16bit_store:
	mov	[base_code],68h
	call	store_instruction_code
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	push_done
      push_imm_64bit:
	cmp	[code_type],64
	jne	illegal_instruction
	call	get_simm32
	mov	edx,eax
	jmp	push_imm_32bit_store
      push_imm_32bit:
	cmp	[code_type],64
	je	illegal_instruction
	call	get_dword_value
	mov	edx,eax
	call	operand_32bit
      push_imm_32bit_store:
	mov	[base_code],68h
	call	store_instruction_code
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
	mov	[size_override],0
	mov	[operand_prefix],0
	mov	[rex_prefix],0
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
	cmp	al,8
	je	pop_mem_64bit
	or	al,al
	jnz	invalid_operand_size
	cmp	ah,2
	je	pop_mem_16bit
	cmp	ah,4
	je	pop_mem_32bit
	cmp	ah,8
	je	pop_mem_64bit
	cmp	[error_line],0
	jne	pop_mem_store
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
	jmp	pop_mem_store
      pop_mem_16bit:
	test	ah,not 2
	jnz	invalid_operand_size
	call	operand_16bit
	jmp	pop_mem_store
      pop_mem_32bit:
	test	ah,not 4
	jnz	invalid_operand_size
	cmp	[code_type],64
	je	illegal_instruction
	call	operand_32bit
	jmp	pop_mem_store
      pop_mem_64bit:
	test	ah,not 8
	jnz	invalid_operand_size
	cmp	[code_type],64
	jne	illegal_instruction
      pop_mem_store:
	mov	[base_code],08Fh
	mov	[postbyte_register],0
	call	store_instruction
	jmp	pop_done
      pop_reg:
	lods	byte [esi]
	mov	ah,al
	sub	ah,10h
	and	ah,al
	test	ah,0F0h
	jnz	pop_sreg
	call	convert_register
	test	al,1000b
	jz	pop_reg_ok
	or	[rex_prefix],41h
	and	al,111b
      pop_reg_ok:
	add	al,58h
	mov	[base_code],al
	mov	al,ah
	mov	ah,[extended_code]
	cmp	al,2
	je	pop_reg_16bit
	cmp	al,4
	je	pop_reg_32bit
	cmp	al,8
	je	pop_reg_64bit
	jmp	invalid_operand_size
      pop_reg_64bit:
	test	ah,not 8
	jnz	invalid_operand_size
	cmp	[code_type],64
	jne	illegal_instruction
	jmp	pop_reg_store
      pop_reg_32bit:
	test	ah,not 4
	jnz	invalid_operand_size
	cmp	[code_type],64
	je	illegal_instruction
	call	operand_32bit
	jmp	pop_reg_store
      pop_reg_16bit:
	test	ah,not 2
	jnz	invalid_operand_size
	call	operand_16bit
      pop_reg_store:
	call	store_instruction_code
      pop_done:
	lods	byte [esi]
	dec	esi
	cmp	al,0Fh
	je	instruction_assembled
	or	al,al
	jz	instruction_assembled
	mov	[operand_size],0
	mov	[size_override],0
	mov	[operand_prefix],0
	mov	[rex_prefix],0
	jmp	pop_next
      pop_sreg:
	mov	bl,al
	mov	dl,[operand_size]
	mov	dh,[extended_code]
	cmp	dl,2
	je	pop_sreg16
	cmp	dl,4
	je	pop_sreg32
	cmp	dl,8
	je	pop_sreg64
	or	dl,dl
	jnz	invalid_operand_size
	cmp	dh,2
	je	pop_sreg16
	cmp	dh,4
	je	pop_sreg32
	cmp	dh,8
	je	pop_sreg64
	jmp	pop_sreg_store
      pop_sreg16:
	test	dh,not 2
	jnz	invalid_operand_size
	call	operand_16bit
	jmp	pop_sreg_store
      pop_sreg32:
	test	dh,not 4
	jnz	invalid_operand_size
	cmp	[code_type],64
	je	illegal_instruction
	call	operand_32bit
	jmp	pop_sreg_store
      pop_sreg64:
	test	dh,not 8
	jnz	invalid_operand_size
	cmp	[code_type],64
	jne	illegal_instruction
      pop_sreg_store:
	mov	al,bl
	cmp	al,70h
	jae	invalid_operand
	sub	al,61h
	cmp	al,4
	jae	pop_sreg_386
	shl	al,3
	add	al,7
	mov	[base_code],al
	cmp	[code_type],64
	je	illegal_instruction
	jmp	pop_reg_store
      pop_sreg_386:
	sub	al,4
	shl	al,3
	add	al,0A1h
	mov	[extended_code],al
	mov	[base_code],0Fh
	jmp	pop_reg_store
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
	jb	inc_mem_nosize
	call	operand_autodetect
	mov	al,0FFh
	xchg	al,[base_code]
	mov	[postbyte_register],al
	call	store_instruction
	jmp	instruction_assembled
      inc_mem_nosize:
	cmp	[error_line],0
	jne	inc_mem_8bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
      inc_mem_8bit:
	mov	al,0FEh
	xchg	al,[base_code]
	mov	[postbyte_register],al
	call	store_instruction
	jmp	instruction_assembled
      inc_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	mov	al,0FEh
	xchg	al,[base_code]
	mov	[postbyte_register],al
	mov	al,ah
	cmp	al,1
	je	inc_reg_8bit
	call	operand_autodetect
	cmp	[code_type],64
	je	inc_reg_long_form
	mov	al,[postbyte_register]
	shl	al,3
	add	al,bl
	add	al,40h
	mov	[base_code],al
	call	store_instruction_code
	jmp	instruction_assembled
      inc_reg_long_form:
	inc	[base_code]
      inc_reg_8bit:
	call	store_nomem_instruction
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
	cmp	ah,1
	jne	invalid_operand_size
	mov	bl,al
	mov	[postbyte_register],0
	call	store_nomem_instruction
	jmp	instruction_assembled
arpl_instruction:
	cmp	[code_type],64
	je	illegal_instruction
	mov	[base_code],63h
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
	call	store_instruction
	jmp	instruction_assembled
      arpl_reg:
	lods	byte [esi]
	call	convert_register
	cmp	ah,2
	jne	invalid_operand_size
	mov	bl,al
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
	mov	[postbyte_register],al
	call	store_nomem_instruction
	jmp	instruction_assembled
bound_instruction:
	cmp	[code_type],64
	je	illegal_instruction
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
	call	operand_32bit
	mov	[base_code],62h
	call	store_instruction
	jmp	instruction_assembled
      bound_16bit:
	call	operand_16bit
	mov	[base_code],62h
	call	store_instruction
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
ret_instruction_only64:
	cmp	[code_type],64
	jne	illegal_instruction
	jmp	ret_instruction
ret_instruction_32bit_except64:
	cmp	[code_type],64
	je	illegal_instruction
ret_instruction_32bit:
	call	operand_32bit
	jmp	ret_instruction
ret_instruction_16bit:
	call	operand_16bit
	jmp	ret_instruction
retf_instruction:
	cmp	[code_type],64
	jne	ret_instruction
ret_instruction_64bit:
	call	operand_64bit
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
	call	store_instruction_code
	mov	ax,dx
	stos	word [edi]
	jmp	instruction_assembled
      simple_ret:
	inc	[base_code]
	call	store_instruction_code
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
	xor	al,al
	xchg	al,[operand_size]
	push	eax
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	call	operand_autodetect
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
	jmp	ls_short_code
      lds_instruction:
	mov	[base_code],0C5h
      ls_short_code:
	cmp	[code_type],64
	je	illegal_instruction
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
	cmp	al,10
	je	ls_64bit
	jmp	invalid_operand_size
      ls_16bit:
	call	operand_16bit
	call	store_instruction
	jmp	instruction_assembled
      ls_32bit:
	call	operand_32bit
	call	store_instruction
	jmp	instruction_assembled
      ls_64bit:
	call	operand_64bit
	call	store_instruction
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
	jb	sh_mem_cl_nosize
	call	operand_autodetect
	mov	[base_code],0D3h
	call	store_instruction
	jmp	instruction_assembled
      sh_mem_cl_nosize:
	cmp	[error_line],0
	jne	sh_mem_cl_8bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
      sh_mem_cl_8bit:
	mov	[base_code],0D2h
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
	jb	sh_mem_imm_nosize
	call	operand_autodetect
	cmp	byte [value],1
	je	sh_mem_1
	mov	[base_code],0C1h
	call	store_instruction_with_imm8
	jmp	instruction_assembled
      sh_mem_1:
	mov	[base_code],0D1h
	call	store_instruction
	jmp	instruction_assembled
      sh_mem_imm_nosize:
	cmp	[error_line],0
	jne	sh_mem_imm_8bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
      sh_mem_imm_8bit:
	cmp	byte [value],1
	je	sh_mem_1_8bit
	mov	[base_code],0C0h
	call	store_instruction_with_imm8
	jmp	instruction_assembled
      sh_mem_1_8bit:
	mov	[base_code],0D0h
	call	store_instruction
	jmp	instruction_assembled
      sh_reg:
	lods	byte [esi]
	call	convert_register
	mov	bx,ax
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
	mov	al,bh
	cmp	al,1
	je	sh_reg_cl_8bit
	call	operand_autodetect
	mov	[base_code],0D3h
	call	store_nomem_instruction
	jmp	instruction_assembled
      sh_reg_cl_8bit:
	mov	[base_code],0D2h
	call	store_nomem_instruction
	jmp	instruction_assembled
      sh_reg_imm:
	mov	al,[operand_size]
	or	al,al
	jz	sh_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      sh_reg_imm_size_ok:
	push	ebx
	call	get_byte_value
	mov	dl,al
	pop	ebx
	mov	al,bh
	cmp	al,1
	je	sh_reg_imm_8bit
	call	operand_autodetect
	cmp	dl,1
	je	sh_reg_1
	mov	[base_code],0C1h
	call	store_nomem_instruction
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      sh_reg_1:
	mov	[base_code],0D1h
	call	store_nomem_instruction
	jmp	instruction_assembled
      sh_reg_imm_8bit:
	cmp	dl,1
	je	sh_reg_1_8bit
	mov	[base_code],0C0h
	call	store_nomem_instruction
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      sh_reg_1_8bit:
	mov	[base_code],0D0h
	call	store_nomem_instruction
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
	mov	al,ah
	mov	[operand_size],0
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
	call	operand_autodetect
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
	call	operand_autodetect
	call	store_instruction_with_imm8
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
	mov	[postbyte_register],al
	mov	al,ah
	push	eax ebx
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
	pop	ebx eax
	call	operand_autodetect
	inc	[extended_code]
	call	store_nomem_instruction
	jmp	instruction_assembled
      shd_reg_reg_imm:
	mov	al,[operand_size]
	or	al,al
	jz	shd_reg_reg_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      shd_reg_reg_imm_size_ok:
	call	get_byte_value
	mov	dl,al
	pop	ebx eax
	call	operand_autodetect
	call	store_nomem_instruction
	mov	al,dl
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
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	movx_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	mov	ah,[operand_size]
	cmp	ah,al
	jae	invalid_operand_size
	cmp	ah,1
	je	movx_mem_8bit
	cmp	ah,2
	je	movx_mem_16bit
	or	ah,ah
	jnz	invalid_operand_size
	cmp	[error_line],0
	jne	movx_mem_8bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
      movx_mem_8bit:
	call	operand_autodetect
	call	store_instruction
	jmp	instruction_assembled
      movx_mem_16bit:
	inc	[extended_code]
	call	operand_autodetect
	call	store_instruction
	jmp	instruction_assembled
      movx_reg:
	lods	byte [esi]
	call	convert_register
	pop	ebx
	xchg	bl,al
	cmp	ah,al
	jae	invalid_operand_size
	cmp	ah,1
	je	movx_reg_8bit
	cmp	ah,2
	je	movx_reg_16bit
	jmp	invalid_operand_size
      movx_reg_8bit:
	call	operand_autodetect
	call	store_nomem_instruction
	jmp	instruction_assembled
      movx_reg_16bit:
	call	operand_autodetect
	inc	[extended_code]
	call	store_nomem_instruction
	jmp	instruction_assembled
movsxd_instruction:
	mov	[base_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	cmp	ah,8
	jne	invalid_operand_size
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	movsxd_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	cmp	[operand_size],4
	je	movsxd_mem_store
	cmp	[operand_size],0
	jne	invalid_operand_size
      movsxd_mem_store:
	call	operand_64bit
	call	store_instruction
	jmp	instruction_assembled
      movsxd_reg:
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	call	operand_64bit
	call	store_nomem_instruction
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
	call	operand_autodetect
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
	call	get_byte_value
	mov	byte [value],al
	pop	eax
	or	al,al
	jz	bt_mem_imm_nosize
	call	operand_autodetect
      bt_mem_imm_store:
	pop	cx bx edx
	mov	[extended_code],0BAh
	call	store_instruction_with_imm8
	jmp	instruction_assembled
      bt_mem_imm_nosize:
	cmp	[error_line],0
	jne	bt_mem_imm_store
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
	jmp	bt_mem_imm_store
      bt_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,al
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
	mov	[postbyte_register],al
	mov	al,ah
	call	operand_autodetect
	call	store_nomem_instruction
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
	call	operand_autodetect
      bt_reg_imm_store:
	mov	[extended_code],0BAh
	call	store_nomem_instruction
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
	call	operand_autodetect
	call	store_instruction
	jmp	instruction_assembled
      bs_reg_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	mov	al,ah
	call	operand_autodetect
	call	store_nomem_instruction
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
	jb	imul_mem_nosize
	call	operand_autodetect
	inc	[base_code]
	call	store_instruction
	jmp	instruction_assembled
      imul_mem_nosize:
	cmp	[error_line],0
	jne	imul_mem_8bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
      imul_mem_8bit:
	call	store_instruction
	jmp	instruction_assembled
      imul_reg:
	lods	byte [esi]
	call	convert_register
	cmp	byte [esi],','
	je	imul_reg_
	mov	bl,al
	mov	al,ah
	cmp	al,1
	je	imul_reg_8bit
	call	operand_autodetect
	inc	[base_code]
	call	store_nomem_instruction
	jmp	instruction_assembled
      imul_reg_8bit:
	call	store_nomem_instruction
	jmp	instruction_assembled
      imul_reg_:
	mov	[postbyte_register],al
	inc	esi
	cmp	byte [esi],'('
	je	imul_reg_imm
	cmp	byte [esi],11h
	jne	imul_reg_noimm
	cmp	byte [esi+2],'('
	je	imul_reg_imm
      imul_reg_noimm:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	imul_reg_reg
	cmp	al,'['
	jne	invalid_operand
      imul_reg_mem:
	call	get_address
	push	edx bx cx
	cmp	byte [esi],','
	je	imul_reg_mem_imm
	mov	al,[operand_size]
	call	operand_autodetect
	pop	cx bx edx
	mov	[base_code],0Fh
	mov	[extended_code],0AFh
	call	store_instruction
	jmp	instruction_assembled
      imul_reg_mem_imm:
	inc	esi
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	jne	invalid_operand
	mov	al,[operand_size]
	cmp	al,2
	je	imul_reg_mem_imm_16bit
	cmp	al,4
	je	imul_reg_mem_imm_32bit
	cmp	al,8
	je	imul_reg_mem_imm_64bit
	jmp	invalid_operand_size
      imul_reg_mem_imm_16bit:
	call	operand_16bit
	call	get_word_value
	mov	word [value],ax
	cmp	[value_type],0
	jne	imul_reg_mem_imm_16bit_store
	cmp	[size_declared],0
	jne	imul_reg_mem_imm_16bit_store
	cmp	ax,-80h
	jl	imul_reg_mem_imm_16bit_store
	cmp	ax,80h
	jl	imul_reg_mem_imm_8bit_store
      imul_reg_mem_imm_16bit_store:
	pop	cx bx edx
	mov	[base_code],69h
	call	store_instruction_with_imm16
	jmp	instruction_assembled
      imul_reg_mem_imm_32bit:
	call	operand_32bit
	call	get_dword_value
      imul_reg_mem_imm_32bit_ok:
	mov	dword [value],eax
	cmp	[value_type],0
	jne	imul_reg_mem_imm_32bit_store
	cmp	[size_declared],0
	jne	imul_reg_mem_imm_32bit_store
	cmp	eax,-80h
	jl	imul_reg_mem_imm_32bit_store
	cmp	eax,80h
	jl	imul_reg_mem_imm_8bit_store
      imul_reg_mem_imm_32bit_store:
	pop	cx bx edx
	mov	[base_code],69h
	call	store_instruction_with_imm32
	jmp	instruction_assembled
      imul_reg_mem_imm_64bit:
	cmp	[size_declared],0
	jne	long_immediate_not_encodable
	cmp	[value_type],4
	jae	long_immediate_not_encodable
	call	operand_64bit
	call	get_simm32
	jmp	imul_reg_mem_imm_32bit_ok
      imul_reg_mem_imm_8bit_store:
	pop	cx bx edx
	mov	[base_code],6Bh
	call	store_instruction_with_imm8
	jmp	instruction_assembled
      imul_reg_imm:
	mov	bl,[postbyte_register]
	dec	esi
	jmp	imul_reg_reg_imm
      imul_reg_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	cmp	byte [esi],','
	je	imul_reg_reg_imm
	mov	al,ah
	call	operand_autodetect
	mov	[base_code],0Fh
	mov	[extended_code],0AFh
	call	store_nomem_instruction
	jmp	instruction_assembled
      imul_reg_reg_imm:
	inc	esi
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'('
	jne	invalid_operand
	mov	al,[operand_size]
	cmp	al,2
	je	imul_reg_reg_imm_16bit
	cmp	al,4
	je	imul_reg_reg_imm_32bit
	cmp	al,8
	je	imul_reg_reg_imm_64bit
	jmp	invalid_operand_size
      imul_reg_reg_imm_16bit:
	call	operand_16bit
	push	ebx
	call	get_word_value
	pop	ebx
	mov	dx,ax
	cmp	[value_type],0
	jne	imul_reg_reg_imm_16bit_store
	cmp	[size_declared],0
	jne	imul_reg_reg_imm_16bit_store
	cmp	ax,-80h
	jl	imul_reg_reg_imm_16bit_store
	cmp	ax,80h
	jl	imul_reg_reg_imm_8bit_store
      imul_reg_reg_imm_16bit_store:
	mov	[base_code],69h
	call	store_nomem_instruction
	mov	ax,dx
	call	mark_relocation
	stos	word [edi]
	jmp	instruction_assembled
      imul_reg_reg_imm_32bit:
	call	operand_32bit
	push	ebx
	call	get_dword_value
      imul_reg_reg_imm_32bit_ok:
	pop	ebx
	mov	edx,eax
	cmp	[value_type],0
	jne	imul_reg_reg_imm_32bit_store
	cmp	[size_declared],0
	jne	imul_reg_reg_imm_32bit_store
	cmp	eax,-80h
	jl	imul_reg_reg_imm_32bit_store
	cmp	eax,80h
	jl	imul_reg_reg_imm_8bit_store
      imul_reg_reg_imm_32bit_store:
	mov	[base_code],69h
	call	store_nomem_instruction
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      imul_reg_reg_imm_64bit:
	cmp	[size_declared],0
	jne	long_immediate_not_encodable
	cmp	[value_type],4
	jae	long_immediate_not_encodable
	call	operand_64bit
	push	ebx
	call	get_simm32
	jmp	imul_reg_reg_imm_32bit_ok
      imul_reg_reg_imm_8bit_store:
	mov	[base_code],6Bh
	call	store_nomem_instruction
	mov	al,dl
	stos	byte [edi]
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
	call	operand_16bit
	mov	[base_code],0EDh
	call	store_instruction_code
	jmp	instruction_assembled
      in_eax_dx:
	call	operand_32bit
	mov	[base_code],0EDh
	call	store_instruction_code
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
	call	operand_16bit
	mov	[base_code],0E5h
	call	store_instruction_code
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      in_eax_imm:
	call	operand_32bit
	mov	[base_code],0E5h
	call	store_instruction_code
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
	call	operand_16bit
	mov	[base_code],0EFh
	call	store_instruction_code
	jmp	instruction_assembled
      out_dx_eax:
	call	operand_32bit
	mov	[base_code],0EFh
	call	store_instruction_code
	jmp	instruction_assembled
      out_imm:
	mov	al,[operand_size]
	or	al,al
	jz	out_imm_size_ok
	cmp	al,1
	jne	invalid_operand_size
      out_imm_size_ok:
	call	get_byte_value
	mov	dl,al
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
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      out_imm_ax:
	call	operand_16bit
	mov	[base_code],0E7h
	call	store_instruction_code
	mov	al,dl
	stos	byte [edi]
	jmp	instruction_assembled
      out_imm_eax:
	call	operand_32bit
	mov	[base_code],0E7h
	call	store_instruction_code
	mov	al,dl
	stos	byte [edi]
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
	jo	jump_out_of_range
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
	sub	eax,2
	jo	jump_out_of_range
	mov	edx,eax
	mov	ecx,edi
	call	operand_32bit
	mov	al,[base_code]
	add	al,10h
	mov	[extended_code],al
	mov	[base_code],0Fh
	call	store_instruction_code
	mov	eax,edi
	sub	eax,ecx
	sub	edx,eax
	jo	jump_out_of_range
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      conditional_jump_16bit:
	mov	edx,eax
	mov	ecx,edi
	call	operand_16bit
	mov	al,[base_code]
	add	al,10h
	mov	[extended_code],al
	mov	[base_code],0Fh
	call	store_instruction_code
	mov	eax,edi
	sub	eax,ecx
	sub	edx,eax
	jo	jump_out_of_range
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
	cmp	[code_type],64
	je	illegal_instruction
	mov	dl,al
	call	address_16bit_prefix
	mov	al,dl
	jmp	loop_instruction
loop_instruction_32bit:
	mov	dl,al
	call	address_32bit_prefix
	mov	al,dl
	jmp	loop_instruction
loop_instruction_64bit:
	cmp	[code_type],64
	jne	illegal_instruction
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
	jo	jump_out_of_range
	mov	edx,eax
	mov	al,[base_code]
	stos	byte [edi]
	mov	al,dl
	stos	byte [edi]
	cmp	edx,80h
	jge	jump_out_of_range
	cmp	edx,-80h
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
	cmp	al,'('
	je	jmp_imm
	mov	[base_code],0FFh
	cmp	al,10h
	je	jmp_reg
	cmp	al,'['
	jne	invalid_operand
      jmp_mem:
	call	get_address
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
	cmp	al,8
	je	jmp_mem_64bit
	cmp	al,10
	je	jmp_mem_80bit
	jmp	invalid_operand_size
      jmp_mem_size_not_specified:
	cmp	[jump_type],2
	je	jmp_mem_far
	cmp	[jump_type],1
	je	jmp_mem_near
	cmp	[error_line],0
	jne	jmp_mem_near
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
      jmp_mem_near:
	cmp	[code_type],16
	je	jmp_mem_16bit
	cmp	[code_type],32
	je	jmp_mem_near_32bit
      jmp_mem_64bit:
	cmp	[jump_type],2
	je	invalid_operand_size
	cmp	[code_type],64
	jne	illegal_instruction
	call	store_instruction
	jmp	instruction_assembled
      jmp_mem_far:
	cmp	[code_type],16
	je	jmp_mem_far_32bit
      jmp_mem_48bit:
	call	operand_32bit
      jmp_mem_far_store:
	cmp	[jump_type],1
	je	invalid_operand_size
	inc	[postbyte_register]
	call	store_instruction
	jmp	instruction_assembled
      jmp_mem_80bit:
	call	operand_64bit
	jmp	jmp_mem_far_store
      jmp_mem_far_32bit:
	call	operand_16bit
	jmp	jmp_mem_far_store
      jmp_mem_32bit:
	cmp	[jump_type],2
	je	jmp_mem_far_32bit
	cmp	[jump_type],1
	je	jmp_mem_near_32bit
	cmp	[code_type],16
	je	jmp_mem_far_32bit
      jmp_mem_near_32bit:
	cmp	[code_type],64
	je	illegal_instruction
	call	operand_32bit
	call	store_instruction
	jmp	instruction_assembled
      jmp_mem_16bit:
	cmp	[jump_type],2
	je	invalid_operand_size
	call	operand_16bit
	call	store_instruction
	jmp	instruction_assembled
      jmp_reg:
	cmp	[jump_type],2
	je	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	mov	al,ah
	cmp	al,2
	je	jmp_reg_16bit
	cmp	al,4
	je	jmp_reg_32bit
	cmp	al,8
	jne	invalid_operand_size
      jmp_reg_64bit:
	cmp	[code_type],64
	jne	illegal_instruction
	call	store_nomem_instruction
	jmp	instruction_assembled
      jmp_reg_32bit:
	cmp	[code_type],64
	je	illegal_instruction
	call	store_nomem_instruction
	jmp	instruction_assembled
      jmp_reg_16bit:
	call	operand_16bit
	call	store_nomem_instruction
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
	jo	jump_out_of_range
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
	cmp	[code_type],16
	je	jmp_16bit
      jmp_32bit:
	test	[operand_size],not 4
	jnz	invalid_operand_size
	sub	eax,2
	jo	jump_out_of_range
	mov	edx,eax
	mov	ecx,edi
	call	operand_32bit
	call	store_instruction_code
	mov	eax,edi
	sub	eax,ecx
	sub	edx,eax
	jo	jump_out_of_range
	mov	eax,edx
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      jmp_16bit:
	mov	edx,eax
	mov	ecx,edi
	call	operand_16bit
	call	store_instruction_code
	mov	eax,edi
	sub	eax,ecx
	sub	edx,eax
	jo	jump_out_of_range
	mov	eax,edx
	stos	word [edi]
	cmp	eax,10000h
	jge	jump_out_of_range
	cmp	eax,-10000h
	jl	jump_out_of_range
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
	cmp	[code_type],64
	je	illegal_instruction
	mov	al,[extended_code]
	mov	[base_code],al
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
	mov	al,[operand_size]
	cmp	al,4
	je	jmp_far_16bit
	cmp	al,6
	je	jmp_far_32bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[code_type],16
	jne	jmp_far_32bit
      jmp_far_16bit:
	call	get_word_value
	mov	ebx,eax
	call	operand_16bit
	call	store_instruction_code
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
	call	operand_32bit
	call	store_instruction_code
	mov	eax,ebx
	call	mark_relocation
	stos	dword [edi]
	jmp	jmp_far_segment

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
	je	movs_address_16bit
	cmp	al,4
	je	movs_address_32bit
	cmp	[code_type],64
	jne	invalid_address_size
	jmp	movs_store
      movs_address_32bit:
	call	address_32bit_prefix
	jmp	movs_store
      movs_address_16bit:
	cmp	[code_type],64
	je	invalid_address_size
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
	cmp	bl,8
	je	simple_instruction_64bit
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
	je	lods_address_16bit
	cmp	bh,46h
	je	lods_address_32bit
	cmp	bh,86h
	jne	invalid_address
	cmp	[code_type],64
	jne	invalid_address_size
	jmp	lods_store
      lods_address_32bit:
	call	address_32bit_prefix
	jmp	lods_store
      lods_address_16bit:
	cmp	[code_type],64
	je	invalid_address_size
	call	address_16bit_prefix
      lods_store:
	cmp	[segment_register],4
	je	lods_segment_ok
	call	store_segment_prefix
      lods_segment_ok:
	mov	al,0ACh
	mov	bl,[operand_size]
	cmp	bl,1
	je	simple_instruction
	inc	al
	cmp	bl,2
	je	simple_instruction_16bit
	cmp	bl,4
	je	simple_instruction_32bit
	cmp	bl,8
	je	simple_instruction_64bit
	or	bl,bl
	jz	operand_size_not_specified
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
	je	stos_address_16bit
	cmp	bh,47h
	je	stos_address_32bit
	cmp	bh,87h
	jne	invalid_address
	cmp	[code_type],64
	jne	invalid_address_size
	jmp	stos_store
      stos_address_32bit:
	call	address_32bit_prefix
	jmp	stos_store
      stos_address_16bit:
	cmp	[code_type],64
	je	invalid_address_size
	call	address_16bit_prefix
      stos_store:
	cmp	[segment_register],1
	ja	invalid_address
	mov	al,[base_code]
	mov	bl,[operand_size]
	cmp	bl,1
	je	simple_instruction
	inc	al
	cmp	bl,2
	je	simple_instruction_16bit
	cmp	bl,4
	je	simple_instruction_32bit
	cmp	bl,8
	je	simple_instruction_64bit
	or	bl,bl
	jz	operand_size_not_specified
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
	je	cmps_address_16bit
	cmp	al,4
	je	cmps_address_32bit
	cmp	[code_type],64
	jne	invalid_address_size
	jmp	cmps_store
      cmps_address_32bit:
	call	address_32bit_prefix
	jmp	cmps_store
      cmps_address_16bit:
	cmp	[code_type],64
	je	invalid_address_size
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
	cmp	bl,8
	je	simple_instruction_64bit
	or	bl,bl
	jz	operand_size_not_specified
	jmp	invalid_operand_size
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
	je	ins_address_16bit
	cmp	bh,47h
	je	ins_address_32bit
	cmp	bh,87h
	jne	invalid_address
	cmp	[code_type],64
	jne	invalid_address_size
	jmp	ins_store
      ins_address_32bit:
	call	address_32bit_prefix
	jmp	ins_store
      ins_address_16bit:
	cmp	[code_type],64
	je	invalid_address_size
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
	je	outs_address_16bit
	cmp	bh,46h
	je	outs_address_32bit
	cmp	bh,86h
	jne	invalid_address
	cmp	[code_type],64
	jne	invalid_address_size
	jmp	outs_store
      outs_address_32bit:
	call	address_32bit_prefix
	jmp	outs_store
      outs_address_16bit:
	cmp	[code_type],64
	je	invalid_address_size
	call	address_16bit_prefix
      outs_store:
	cmp	[segment_register],4
	je	outs_segment_ok
	call	store_segment_prefix
      outs_segment_ok:
	mov	al,6Eh
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
	je	xlat_address_16bit
	cmp	bh,43h
	je	xlat_address_32bit
	cmp	bh,83h
	jne	invalid_address
	cmp	[code_type],64
	jne	invalid_address_size
	jmp	xlat_store
      xlat_address_32bit:
	call	address_32bit_prefix
	jmp	xlat_store
      xlat_address_16bit:
	cmp	[code_type],64
	je	invalid_address_size
	call	address_16bit_prefix
      xlat_store:
	call	store_segment_prefix_if_necessary
	mov	al,0D7h
	cmp	[operand_size],1
	jbe	simple_instruction
	jmp	invalid_operand_size

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
	je	pm_mem_store
	or	al,al
	jnz	invalid_operand_size
      pm_mem_store:
	call	store_instruction
	jmp	instruction_assembled
      pm_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	mov	al,ah
	call	operand_autodetect
	call	store_nomem_instruction
	jmp	instruction_assembled
lgdt_instruction:
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
	je	lgdt_mem_48bit
	cmp	al,10
	je	lgdt_mem_80bit
	or	al,al
	jnz	invalid_operand_size
	cmp	[code_type],64
	je	lgdt_mem_80bit
      lgdt_mem_48bit:
	cmp	[code_type],64
	je	illegal_instruction
	call	store_instruction
	jmp	instruction_assembled
      lgdt_mem_80bit:
	cmp	[code_type],64
	jne	illegal_instruction
	call	store_instruction
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
	xor	al,al
	xchg	al,[operand_size]
	call	operand_autodetect
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	lar_reg_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	lar_reg_mem
	cmp	al,2
	jne	invalid_operand_size
      lar_reg_mem:
	call	store_instruction
	jmp	instruction_assembled
      lar_reg_reg:
	lods	byte [esi]
	call	convert_register
	cmp	ah,2
	jne	invalid_operand_size
	mov	bl,al
	call	store_nomem_instruction
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
swapgs_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],1
	mov	[postbyte_register],7
	xor	bl,bl
	call	store_nomem_instruction
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
	call	operand_autodetect
	inc	[extended_code]
      basic_486_mem_reg_8bit:
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
	mov	[postbyte_register],al
	mov	al,ah
	cmp	al,1
	je	basic_486_reg_reg_8bit
	call	operand_autodetect
	inc	[extended_code]
      basic_486_reg_reg_8bit:
	call	store_nomem_instruction
	jmp	instruction_assembled
bswap_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	add	al,0C8h
	mov	[extended_code],al
	mov	[base_code],0Fh
	cmp	ah,8
	je	bswap_reg64
	cmp	ah,4
	jne	invalid_operand_size
	call	operand_32bit
	call	store_instruction_code
	jmp	instruction_assembled
      bswap_reg64:
	call	operand_64bit
	call	store_instruction_code
	jmp	instruction_assembled
cmpxchgx_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0C7h
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	ah,1
	xchg	[postbyte_register],ah
	mov	al,[operand_size]
	or	al,al
	jz	cmpxchgx_size_ok
	cmp	al,ah
	jne	invalid_operand_size
      cmpxchgx_size_ok:
	cmp	ah,16
	jne	cmpxchgx_store
	call	operand_64bit
      cmpxchgx_store:
	call	store_instruction
	jmp	instruction_assembled

basic_fpu_instruction:
	mov	[postbyte_register],al
	mov	[base_code],0D8h
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
	mov	bl,1
	call	store_nomem_instruction
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
	cmp	[error_line],0
	jne	basic_fpu_mem_32bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
      basic_fpu_mem_32bit:
	call	store_instruction
	jmp	instruction_assembled
      basic_fpu_mem_64bit:
	mov	[base_code],0DCh
	call	store_instruction
	jmp	instruction_assembled
      basic_fpu_streg:
	lods	byte [esi]
	call	convert_fpu_register
	mov	bl,al
	mov	ah,[postbyte_register]
	cmp	ah,2
	je	basic_fpu_single_streg
	cmp	ah,3
	je	basic_fpu_single_streg
	or	al,al
	jz	basic_fpu_st0
	test	ah,110b
	jz	basic_fpu_streg_st0
	xor	[postbyte_register],1
      basic_fpu_streg_st0:
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
	mov	[base_code],0DCh
	call	store_nomem_instruction
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
	mov	bl,al
      basic_fpu_single_streg:
	mov	[base_code],0D8h
	call	store_nomem_instruction
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
	cmp	[error_line],0
	jne	fi_mem_32bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
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
	cmp	[error_line],0
	jne	fld_mem_32bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
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
	je	fld_mem_80bit_store
	dec	[postbyte_register]
	cmp	al,3
	je	fld_mem_80bit_store
	jmp	invalid_operand_size
      fld_mem_80bit_store:
	add	[postbyte_register],5
	mov	[base_code],0DBh
	call	store_instruction
	jmp	instruction_assembled
      fld_streg:
	lods	byte [esi]
	call	convert_fpu_register
	mov	bl,al
	cmp	[postbyte_register],2
	jae	fst_streg
	mov	[base_code],0D9h
	call	store_nomem_instruction
	jmp	instruction_assembled
      fst_streg:
	mov	[base_code],0DDh
	call	store_nomem_instruction
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
	cmp	[error_line],0
	jne	fild_mem_32bit
	mov	eax,[current_line]
	mov	[error_line],eax
	mov	[error],operand_size_not_specified
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
	cmp	al,1
	je	fisttp_64bit_store
	jb	fild_mem_64bit_store
	dec	[postbyte_register]
	cmp	al,3
	je	fild_mem_64bit_store
	jmp	invalid_operand_size
      fild_mem_64bit_store:
	add	[postbyte_register],5
	mov	[base_code],0DFh
	call	store_instruction
	jmp	instruction_assembled
      fisttp_64bit_store:
	mov	[base_code],0DDh
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
	mov	[base_code],0DEh
	mov	edx,esi
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	faddp_streg
	mov	esi,edx
	mov	bl,1
	call	store_nomem_instruction
	jmp	instruction_assembled
      faddp_streg:
	lods	byte [esi]
	call	convert_fpu_register
	mov	bl,al
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
	call	store_nomem_instruction
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
	mov	[postbyte_register],al
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
	call	store_instruction
	jmp	instruction_assembled
      mmx_mmreg_mmreg:
	lods	byte [esi]
	call	convert_mmx_register
	mov	bl,al
	call	store_nomem_instruction
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
	mov	[postbyte_register],al
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
	mov	bl,[extended_code]
	mov	al,bl
	shr	bl,4
	and	al,1111b
	add	al,70h
	mov	[extended_code],al
	sub	bl,0Ch
	shl	bl,1
	xchg	bl,[postbyte_register]
	call	store_nomem_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
pextrw_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jnz	invalid_operand_size
	mov	[postbyte_register],al
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
	mov	bl,al
	call	make_mmx_prefix
	cmp	[extended_code],0C5h
	je	mmx_nomem_imm8
	call	store_nomem_instruction
	jmp	instruction_assembled
      mmx_imm8:
	push	bx cx edx
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
	mov	byte [value],al
	pop	edx cx bx
	call	store_instruction_with_imm8
	jmp	instruction_assembled
      mmx_nomem_imm8:
	call	store_nomem_instruction
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
	stosb
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
	je	mmx_imm8
	cmp	[operand_size],2
	jne	invalid_operand_size
	jmp	mmx_imm8
      pinsrw_mmreg_reg:
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	mov	bl,al
	jmp	mmx_nomem_imm8
pshufw_instruction:
	mov	[mmx_size],8
	mov	[operand_prefix],al
	jmp	pshuf_instruction
pshufd_instruction:
	mov	[mmx_size],16
	mov	[operand_prefix],al
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
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	pshuf_mmreg_mmreg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	jmp	mmx_imm8
      pshuf_mmreg_mmreg:
	lods	byte [esi]
	call	convert_mmx_register
	mov	bl,al
	jmp	mmx_nomem_imm8
movd_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],7Eh
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
	call	store_instruction
	jmp	instruction_assembled
      movd_reg:
	lods	byte [esi]
	cmp	al,0B0h
	jae	movd_mmreg
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
	mov	[postbyte_register],al
	call	make_mmx_prefix
	call	store_nomem_instruction
	jmp	instruction_assembled
      movd_mmreg:
	mov	[extended_code],6Eh
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
	je	movd_mmreg_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	test	[operand_size],not 4
	jnz	invalid_operand_size
	call	store_instruction
	jmp	instruction_assembled
      movd_mmreg_reg:
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	mov	bl,al
	call	store_nomem_instruction
	jmp	instruction_assembled
      make_mmx_prefix:
	cmp	[operand_size],16
	jne	no_mmx_prefix
	mov	[operand_prefix],66h
      no_mmx_prefix:
	ret
movq_instruction:
	mov	[base_code],0Fh
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	movq_reg
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
	cmp	ah,16
	je	movq_mem_xmmreg
	mov	[extended_code],7Fh
	call	store_instruction
	jmp	instruction_assembled
     movq_mem_xmmreg:
	mov	[extended_code],0D6h
	mov	[operand_prefix],66h
	call	store_instruction
	jmp	instruction_assembled
     movq_reg:
	lods	byte [esi]
	cmp	al,0B0h
	jae	movq_mmreg
	call	convert_register
	cmp	ah,8
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
	mov	[postbyte_register],al
	call	make_mmx_prefix
	mov	[extended_code],7Eh
	call	operand_64bit
	call	store_nomem_instruction
	jmp	instruction_assembled
     movq_mmreg:
	call	convert_mmx_register
	mov	[postbyte_register],al
	mov	[extended_code],6Fh
	cmp	ah,16
	jne	movq_mmreg_
	mov	[extended_code],7Eh
	mov	[operand_prefix],0F3h
      movq_mmreg_:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	movq_mmreg_reg
	call	get_address
	test	[operand_size],not 8
	jnz	invalid_operand_size
	call	store_instruction
	jmp	instruction_assembled
      movq_mmreg_reg:
	lods	byte [esi]
	cmp	al,0B0h
	jae	movq_mmreg_mmreg
	mov	[operand_size],0
	call	convert_register
	cmp	ah,8
	jne	invalid_operand_size
	mov	[extended_code],6Eh
	mov	[operand_prefix],0
	mov	bl,al
	call	make_mmx_prefix
	call	operand_64bit
	call	store_nomem_instruction
	jmp	instruction_assembled
      movq_mmreg_mmreg:
	call	convert_mmx_register
	mov	bl,al
	call	store_nomem_instruction
	jmp	instruction_assembled
movdq_instruction:
	mov	[operand_prefix],al
	mov	[base_code],0Fh
	mov	[extended_code],6Fh
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
	mov	[extended_code],7Fh
	call	store_instruction
	jmp	instruction_assembled
      movdq_mmreg:
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	[postbyte_register],al
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
	call	store_instruction
	jmp	instruction_assembled
      movdq_mmreg_mmreg:
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	bl,al
	call	store_nomem_instruction
	jmp	instruction_assembled
lddqu_instruction:
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
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	mov	[postbyte_register],al
	mov	[operand_prefix],0F2h
	mov	[base_code],0Fh
	mov	[extended_code],0F0h
	call	store_instruction
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
	mov	[postbyte_register],al
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
	mov	bl,al
	mov	[operand_prefix],0F3h
	mov	[base_code],0Fh
	mov	[extended_code],0D6h
	call	store_nomem_instruction
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
	mov	[postbyte_register],al
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
	mov	bl,al
	mov	[operand_prefix],0F2h
	mov	[base_code],0Fh
	mov	[extended_code],0D6h
	call	store_nomem_instruction
	jmp	instruction_assembled

sse_ps_instruction:
	mov	[mmx_size],16
	jmp	sse_instruction
sse_pd_instruction:
	mov	[mmx_size],16
	mov	[operand_prefix],66h
	jmp	sse_instruction
sse_ss_instruction:
	mov	[mmx_size],4
	mov	[operand_prefix],0F3h
	jmp	sse_instruction
sse_sd_instruction:
	mov	[mmx_size],8
	mov	[operand_prefix],0F2h
	jmp	sse_instruction
comiss_instruction:
	mov	[mmx_size],4
	jmp	sse_instruction
comisd_instruction:
	mov	[mmx_size],8
	mov	[operand_prefix],66h
	jmp	sse_instruction
cvtps2pd_instruction:
	mov	[mmx_size],8
	jmp	sse_instruction
cvtpd2dq_instruction:
	mov	[mmx_size],16
	mov	[operand_prefix],0F2h
	jmp	sse_instruction
cvtdq2pd_instruction:
	mov	[mmx_size],16
	mov	[operand_prefix],0F3h
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
	mov	[postbyte_register],al
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	sse_xmmreg_xmmreg
      sse_reg_mem:
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	cmp	[operand_size],0
	je	sse_mem_size_ok
	mov	al,[mmx_size]
	cmp	[operand_size],al
	jne	invalid_operand_size
      sse_mem_size_ok:
	cmp	[extended_code],0C6h
	je	mmx_imm8
	call	store_instruction
	jmp	instruction_assembled
      sse_xmmreg_xmmreg:
	cmp	[operand_prefix],66h
	jne	sse_xmmreg_xmmreg_ok
	cmp	[extended_code],12h
	je	invalid_operand
	cmp	[extended_code],16h
	je	invalid_operand
      sse_xmmreg_xmmreg_ok:
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	bl,al
	cmp	[extended_code],0C6h
	je	mmx_nomem_imm8
	call	store_nomem_instruction
	jmp	instruction_assembled
ps_dq_instruction:
	mov	[postbyte_register],al
	mov	[operand_prefix],66h
	mov	[base_code],0Fh
	mov	[extended_code],73h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	bl,al
	jmp	mmx_nomem_imm8
movpd_instruction:
	mov	[operand_prefix],66h
movps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	mov	[mmx_size],16
	jmp	sse_mov_instruction
movss_instruction:
	mov	[mmx_size],4
	mov	[operand_prefix],0F3h
	jmp	sse_movs
movsd_instruction:
	mov	al,0A5h
	mov	ah,[esi]
	or	ah,ah
	jz	simple_instruction_32bit
	cmp	ah,0Fh
	je	simple_instruction_32bit
	mov	[mmx_size],8
	mov	[operand_prefix],0F2h
      sse_movs:
	mov	[base_code],0Fh
	mov	[extended_code],10h
	jmp	sse_mov_instruction
sse_mov_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	sse_xmmreg
      sse_mem:
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
	call	store_instruction
	jmp	instruction_assembled
movlpd_instruction:
	mov	[operand_prefix],66h
movlps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	mov	[mmx_size],8
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	sse_mem
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	[postbyte_register],al
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	jmp	sse_reg_mem
movhlps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	mov	[mmx_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	sse_xmmreg_xmmreg_ok
	jmp	invalid_operand
maskmovq_instruction:
	mov	cl,8
	jmp	maskmov_instruction
maskmovdqu_instruction:
	mov	cl,16
	mov	[operand_prefix],66h
      maskmov_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0F7h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,cl
	jne	invalid_operand_size
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	mov	bl,al
	call	store_nomem_instruction
	jmp	instruction_assembled
movmskpd_instruction:
	mov	[operand_prefix],66h
movmskps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],50h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	mov	[operand_size],0
	mov	[postbyte_register],al
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
	mov	bl,al
	call	store_nomem_instruction
	jmp	instruction_assembled
cmppd_instruction:
	mov	[operand_prefix],66h
cmpps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0C2h
	mov	[mmx_size],16
	mov	byte [value],-1
	jmp	sse_cmp_instruction
cmp_pd_instruction:
	mov	[operand_prefix],66h
cmp_ps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0C2h
	mov	[mmx_size],16
	mov	byte [value],al
	jmp	sse_cmp_instruction
cmpss_instruction:
	mov	[mmx_size],4
	mov	[operand_prefix],0F3h
	jmp	cmpsx_instruction
cmpsd_instruction:
	mov	al,0A7h
	mov	ah,[esi]
	or	ah,ah
	jz	simple_instruction_32bit
	cmp	ah,0Fh
	je	simple_instruction_32bit
	mov	[mmx_size],8
	mov	[operand_prefix],0F2h
      cmpsx_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0C2h
	mov	byte [value],-1
	jmp	sse_cmp_instruction
cmp_ss_instruction:
	mov	[mmx_size],4
	mov	[operand_prefix],0F3h
	jmp	cmp_sx_instruction
cmp_sd_instruction:
	mov	[mmx_size],8
	mov	[operand_prefix],0F2h
      cmp_sx_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0C2h
	mov	byte [value],al
sse_cmp_instruction:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	sse_cmp_xmmreg_xmmreg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	sse_cmp_size_ok
	cmp	al,[mmx_size]
	jne	invalid_operand_size
      sse_cmp_size_ok:
	push	bx cx edx
	call	get_nextbyte
	pop	edx cx bx
	call	store_instruction_with_imm8
	jmp	instruction_assembled
      sse_cmp_xmmreg_xmmreg:
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	bl,al
	call	store_nomem_instruction
	call	get_nextbyte
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      get_nextbyte:
	cmp	byte [value],-1
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
	mov	byte [value],al
      nextbyte_ok:
	ret
cvtpi2pd_instruction:
	mov	[operand_prefix],66h
cvtpi2ps_instruction:
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
	mov	[postbyte_register],al
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
	cmp	[operand_size],0
	je	cvtpi_size_ok
	cmp	[operand_size],8
	jne	invalid_operand_size
      cvtpi_size_ok:
	call	store_instruction
	jmp	instruction_assembled
      cvtpi_xmmreg_xmmreg:
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,8
	jne	invalid_operand_size
	mov	bl,al
	call	store_nomem_instruction
	jmp	instruction_assembled
cvtsi2ss_instruction:
	mov	[operand_prefix],0F3h
	jmp	cvtsi_instruction
cvtsi2sd_instruction:
	mov	[operand_prefix],0F2h
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
	mov	[postbyte_register],al
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
	cmp	[operand_size],0
	je	cvtsi_size_ok
	cmp	[operand_size],4
	jne	invalid_operand_size
      cvtsi_size_ok:
	call	store_instruction
	jmp	instruction_assembled
      cvtsi_xmmreg_reg:
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	je	cvtsi_xmmreg_reg_store
	cmp	ah,8
	jne	invalid_operand_size
	call	operand_64bit
      cvtsi_xmmreg_reg_store:
	mov	bl,al
	call	store_nomem_instruction
	jmp	instruction_assembled
cvtps2pi_instruction:
	mov	[mmx_size],8
	jmp	cvtpd_instruction
cvtpd2pi_instruction:
	mov	[operand_prefix],66h
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
	mov	[operand_prefix],0F3h
	mov	[mmx_size],4
	jmp	cvt2si_instruction
cvtsd2si_instruction:
	mov	[operand_prefix],0F2h
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
	mov	[operand_size],0
	cmp	ah,4
	je	sse_reg
	cmp	ah,8
	jne	invalid_operand_size
	call	operand_64bit
	jmp	sse_reg
amd3dnow_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],0Fh
	mov	byte [value],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,8
	jne	invalid_operand_size
	mov	[postbyte_register],al
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
	call	store_instruction_with_imm8
	jmp	instruction_assembled
      amd3dnow_mmreg_mmreg:
	lods	byte [esi]
	call	convert_mmx_register
	cmp	ah,8
	jne	invalid_operand_size
	mov	bl,al
	call	store_nomem_instruction
	mov	al,byte [value]
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
	cmp	al,111b
	je	clflush_size_check
	cmp	al,10b
	jb	invalid_operand_size
	cmp	al,11b
	ja	invalid_operand_size
	cmp	ah,4
	jne	invalid_operand_size
	jmp	fxsave_size_ok
      clflush_size_check:
	cmp	ah,1
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
	or	ah,ah
	jz	prefetch_size_ok
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
movntq_instruction:
	mov	[mmx_size],8
	jmp	movnt_instruction
movntps_instruction:
	mov	[mmx_size],16
	jmp	movnt_instruction
movntdq_instruction:
	mov	[operand_prefix],66h
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
	call	store_instruction
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
	je	movnti_store
	cmp	ah,8
	jne	invalid_operand_size
	call	operand_64bit
      movnti_store:
	mov	[postbyte_register],al
	call	store_instruction
	jmp	instruction_assembled
monitor_instruction:
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	ax,0400h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	ax,0401h
	jne	invalid_operand
	cmp	[postbyte_register],0C8h
	jne	monitor_instruction_store
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	ax,0402h
	jne	invalid_operand
      monitor_instruction_store:
	mov	ax,010Fh
	stos	word [edi]
	mov	al,[postbyte_register]
	stos	byte [edi]
	jmp	instruction_assembled

simple_vmx_instruction:
	mov	ah,al
	mov	al,0Fh
	stos	byte [edi]
	mov	al,1
	stos	word [edi]
	jmp	instruction_assembled
vmclear_instruction:
	mov	[operand_prefix],66h
	jmp	vmx_instruction
vmxon_instruction:
	mov	[operand_prefix],0F3h
vmx_instruction:
	mov	[postbyte_register],al
	mov	[extended_code],0C7h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	vmx_size_ok
	cmp	al,8
	jne	invalid_operand_size
      vmx_size_ok:
	mov	[base_code],0Fh
	call	store_instruction
	jmp	instruction_assembled
vmread_instruction:
	mov	[extended_code],78h
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
	mov	[postbyte_register],al
      vmread_check_size:
	cmp	[code_type],64
	je	vmread_long
	cmp	[operand_size],4
	je	vmx_size_ok
	jmp	invalid_operand_size
      vmread_long:
	cmp	[operand_size],8
	je	vmx_size_ok
	jmp	invalid_operand_size
vmwrite_instruction:
	mov	[extended_code],79h
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
	jmp	vmread_check_size

convert_register:
	mov	ah,al
	shr	ah,4
	and	al,0Fh
	cmp	ah,8
	je	match_register_size
	cmp	ah,4
	ja	invalid_operand
	cmp	ah,1
	ja	match_register_size
	cmp	al,4
	jb	match_register_size
	or	ah,ah
	jz	high_byte_register
	or	[rex_prefix],40h
      match_register_size:
	cmp	ah,[operand_size]
	je	register_size_ok
	cmp	[operand_size],0
	jne	operand_sizes_do_not_match
	mov	[operand_size],ah
      register_size_ok:
	ret
      high_byte_register:
	mov	ah,1
	or	[rex_prefix],80h
	jmp	match_register_size
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
	cmp	ah,0Ch
	je	xmm_register
	ja	invalid_operand
	and	al,111b
	cmp	ah,0Bh
	jne	invalid_operand
	mov	ah,8
	jmp	match_register_size
      xmm_register:
	and	al,0Fh
	mov	ah,16
	cmp	al,8
	jb	match_register_size
	cmp	[code_type],64
	jne	invalid_operand
	jmp	match_register_size
get_size_operator:
	xor	ah,ah
	cmp	al,11h
	jne	no_size_operator
	mov	[size_declared],1
	lods	word [esi]
	xchg	al,ah
	mov	[size_override],1
	cmp	ah,[operand_size]
	je	size_operator_ok
	cmp	[operand_size],0
	jne	operand_sizes_do_not_match
	mov	[operand_size],ah
      size_operator_ok:
	ret
      no_size_operator:
	mov	[size_declared],0
	cmp	al,'['
	jne	size_operator_ok
	mov	[size_override],0
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
get_address:
	mov	[segment_register],0
	mov	[address_size],0
	mov	al,[code_type]
	shr	al,3
	mov	[value_size],al
	mov	al,[esi]
	and	al,11110000b
	cmp	al,60h
	jne	get_size_prefix
	lods	byte [esi]
	sub	al,60h
	mov	[segment_register],al
	mov	al,[esi]
	and	al,11110000b
      get_size_prefix:
	cmp	al,70h
	jne	address_size_prefix_ok
	lods	byte [esi]
	sub	al,70h
	cmp	al,2
	jb	invalid_address_size
	cmp	al,8
	ja	invalid_address_size
	mov	[address_size],al
	mov	[value_size],al
      address_size_prefix_ok:
	call	calculate_address
	mov	[address_high],edx
	mov	edx,eax
	cmp	[code_type],64
	jne	address_ok
	or	bx,bx
	jnz	address_ok
	test	ch,0Fh
	jnz	address_ok
      calculate_relative_address:
	call	calculate_relative_offset
	mov	[address_high],edx
	mov	edx,[symbol_identifier]
	mov	[address_symbol],edx
	mov	edx,eax
	mov	ch,[value_type]
	mov	bx,0FF00h
	xor	cl,cl
      address_ok:
	ret

operand_16bit:
	cmp	[code_type],16
	je	size_prefix_ok
	mov	[operand_prefix],66h
	ret
operand_32bit:
	cmp	[code_type],16
	jne	size_prefix_ok
	mov	[operand_prefix],66h
      size_prefix_ok:
	ret
operand_64bit:
	cmp	[code_type],64
	jne	invalid_operand_size
	or	[rex_prefix],48h
	ret
operand_autodetect:
	cmp	al,2
	je	operand_16bit
	cmp	al,4
	je	operand_32bit
	cmp	al,8
	je	operand_64bit
	jmp	invalid_operand_size
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
store_instruction_code:
	mov	al,[operand_prefix]
	or	al,al
	jz	operand_prefix_ok
	stos	byte [edi]
      operand_prefix_ok:
	mov	al,[rex_prefix]
	test	al,40h
	jz	rex_prefix_ok
	cmp	[code_type],64
	jne	invalid_operand
	test	al,0B0h
	jnz	prefix_conflict
	stos	byte [edi]
      rex_prefix_ok:
	mov	al,[base_code]
	stos	byte [edi]
	cmp	al,0Fh
	jne	instruction_code_ok
      store_extended_code:
	mov	al,[extended_code]
	stos	byte [edi]
      instruction_code_ok:
	ret
store_nomem_instruction:
	test	[postbyte_register],1000b
	jz	nomem_reg_code_ok
	or	[rex_prefix],44h
	and	[postbyte_register],111b
      nomem_reg_code_ok:
	test	bl,1000b
	jz	nomem_rm_code_ok
	or	[rex_prefix],41h
	and	bl,111b
      nomem_rm_code_ok:
	call	store_instruction_code
	mov	al,[postbyte_register]
	shl	al,3
	or	al,bl
	or	al,11000000b
	stos	byte [edi]
	ret
store_instruction:
	mov	[current_offset],edi
	test	[postbyte_register],1000b
	jz	reg_code_ok
	or	[rex_prefix],44h
	and	[postbyte_register],111b
      reg_code_ok:
	call	store_segment_prefix_if_necessary
	or	bx,bx
	jz	address_immediate
	cmp	bx,0F000h
	je	address_rip_based
	cmp	bx,0FF00h
	je	address_relative
	mov	al,bl
	or	al,bh
	and	al,11110000b
	cmp	al,80h
	je	postbyte_64bit
	cmp	al,40h
	je	postbyte_32bit
	cmp	al,20h
	jne	invalid_address
	cmp	[code_type],64
	je	invalid_address_size
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
	test	ch,22h
	jnz	address_16bit_value
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
	jmp	base_and_index
      postbyte_64bit:
	cmp	[code_type],64
	jne	invalid_address_size
	test	bh,1000b
	jz	base_code_ok
	or	[rex_prefix],41h
      base_code_ok:
	test	bl,1000b
	jz	index_code_ok
	or	[rex_prefix],42h
      index_code_ok:
	call	store_instruction_code
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
	test	ch,44h
	jnz	sib_address_32bit_value
	or	ch,ch
	jnz	address_sizes_do_not_agree
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
	test	ch,44h
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
	test	ch,44h
	jnz	simple_address_32bit_value
	or	ch,ch
	jnz	address_sizes_do_not_agree
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
	cmp	[code_type],64
	je	address_immediate_sib
	test	ch,44h
	jnz	address_immediate_32bit
	test	ch,22h
	jnz	address_immediate_16bit
	or	ch,ch
	jnz	invalid_address_size
	cmp	[code_type],16
	je	addressing_16bit
      address_immediate_32bit:
	call	address_32bit_prefix
	call	store_instruction_code
      store_immediate_address:
	mov	al,101b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
      store_address_32bit_value:
	test	ch,0F0h
	jz	address_32bit_relocation_ok
	mov	al,2
	xchg	[value_type],al
	mov	ebx,[address_symbol]
	xchg	ebx,[symbol_identifier]
	call	mark_relocation
	mov	[value_type],al
	mov	[symbol_identifier],ebx
      address_32bit_relocation_ok:
	mov	eax,edx
	stos	dword [edi]
	ret
      store_address_64bit_value:
	test	ch,0F0h
	jz	address_64bit_relocation_ok
	mov	al,4
	xchg	[value_type],al
	mov	ebx,[address_symbol]
	xchg	ebx,[symbol_identifier]
	call	mark_relocation
	mov	[value_type],al
	mov	[symbol_identifier],ebx
      address_64bit_relocation_ok:
	mov	eax,edx
	stos	dword [edi]
	mov	eax,[address_high]
	stos	dword [edi]
	ret
      address_immediate_sib:
	test	ch,not 44h
	jnz	invalid_address_size
	call	address_32bit_prefix
	call	store_instruction_code
	mov	al,100b
	mov	ah,100101b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	word [edi]
	jmp	store_address_32bit_value
      address_rip_based:
	cmp	[code_type],64
	jne	invalid_address
	call	store_instruction_code
	jmp	store_immediate_address
      address_relative:
	call	store_instruction_code
	movzx	eax,[immediate_size]
	add	eax,edi
	sub	eax,[current_offset]
	add	eax,5
	sub	edx,eax
	jo	value_out_of_range
	mov	al,101b
	mov	cl,[postbyte_register]
	shl	cl,3
	or	al,cl
	stos	byte [edi]
	xchg	[value_type],ch
	mov	ebx,[address_symbol]
	xchg	ebx,[symbol_identifier]
	mov	eax,edx
	call	mark_relocation
	mov	[value_type],ch
	mov	[symbol_identifier],ebx
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
store_instruction_with_imm8:
	mov	[immediate_size],1
	call	store_instruction
	mov	al,byte [value]
	stos	byte [edi]
	ret
store_instruction_with_imm16:
	mov	[immediate_size],2
	call	store_instruction
	mov	ax,word [value]
	call	mark_relocation
	stos	word [edi]
	ret
store_instruction_with_imm32:
	mov	[immediate_size],4
	call	store_instruction
	mov	eax,dword [value]
	call	mark_relocation
	stos	dword [edi]
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
 db 3,'r10',10h,8Ah
 db 3,'r11',10h,8Bh
 db 3,'r12',10h,8Ch
 db 3,'r13',10h,8Dh
 db 3,'r14',10h,8Eh
 db 3,'r15',10h,8Fh
 db 2,'r8',10h,88h
 db 2,'r9',10h,89h
 db 3,'rax',0,80h
 db 3,'rbp',0,85h
 db 3,'rbx',0,83h
 db 3,'rcx',0,81h
 db 3,'rdi',0,87h
 db 3,'rdx',0,82h
 db 3,'rip',0,0F0h
 db 3,'rsi',0,86h
 db 3,'rsp',0,84h
 db 2,'si',0,26h
 db 0

address_sizes:
 db 4,'byte',0,1
 db 5,'dword',0,4
 db 5,'qword',0,8
 db 4,'word',0,2
 db 0

symbols:
 db 2,'ah',10h,04h
 db 2,'al',10h,10h
 db 2,'ax',10h,20h
 db 2,'bh',10h,07h
 db 2,'bl',10h,13h
 db 2,'bp',10h,25h
 db 3,'bpl',10h,15h
 db 2,'bx',10h,23h
 db 4,'byte',11h,1
 db 2,'ch',10h,05h
 db 2,'cl',10h,11h
 db 3,'cr0',10h,50h
 db 3,'cr1',10h,51h
 db 3,'cr2',10h,52h
 db 3,'cr3',10h,53h
 db 3,'cr4',10h,54h
 db 3,'cr5',10h,55h
 db 3,'cr6',10h,56h
 db 3,'cr7',10h,57h
 db 2,'cs',10h,62h
 db 2,'cx',10h,21h
 db 2,'dh',10h,06h
 db 2,'di',10h,27h
 db 3,'dil',10h,17h
 db 2,'dl',10h,12h
 db 6,'dqword',11h,16
 db 3,'dr0',10h,70h
 db 3,'dr1',10h,71h
 db 3,'dr2',10h,72h
 db 3,'dr3',10h,73h
 db 3,'dr4',10h,74h
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
 db 3,'mm0',10h,0B0h
 db 3,'mm1',10h,0B1h
 db 3,'mm2',10h,0B2h
 db 3,'mm3',10h,0B3h
 db 3,'mm4',10h,0B4h
 db 3,'mm5',10h,0B5h
 db 3,'mm6',10h,0B6h
 db 3,'mm7',10h,0B7h
 db 4,'near',12h,1
 db 5,'pword',11h,6
 db 5,'qword',11h,8
 db 3,'r10',10h,8Ah
 db 4,'r10b',10h,1Ah
 db 4,'r10d',10h,4Ah
 db 4,'r10w',10h,2Ah
 db 3,'r11',10h,8Bh
 db 4,'r11b',10h,1Bh
 db 4,'r11d',10h,4Bh
 db 4,'r11w',10h,2Bh
 db 3,'r12',10h,8Ch
 db 4,'r12b',10h,1Ch
 db 4,'r12d',10h,4Ch
 db 4,'r12w',10h,2Ch
 db 3,'r13',10h,8Dh
 db 4,'r13b',10h,1Dh
 db 4,'r13d',10h,4Dh
 db 4,'r13w',10h,2Dh
 db 3,'r14',10h,8Eh
 db 4,'r14b',10h,1Eh
 db 4,'r14d',10h,4Eh
 db 4,'r14w',10h,2Eh
 db 3,'r15',10h,8Fh
 db 4,'r15b',10h,1Fh
 db 4,'r15d',10h,4Fh
 db 4,'r15w',10h,2Fh
 db 2,'r8',10h,88h
 db 3,'r8b',10h,18h
 db 3,'r8d',10h,48h
 db 3,'r8w',10h,28h
 db 2,'r9',10h,89h
 db 3,'r9b',10h,19h
 db 3,'r9d',10h,49h
 db 3,'r9w',10h,29h
 db 3,'rax',10h,80h
 db 3,'rbp',10h,85h
 db 3,'rbx',10h,83h
 db 3,'rcx',10h,81h
 db 3,'rdi',10h,87h
 db 3,'rdx',10h,82h
 db 3,'rsi',10h,86h
 db 3,'rsp',10h,84h
 db 2,'si',10h,26h
 db 3,'sil',10h,16h
 db 2,'sp',10h,24h
 db 3,'spl',10h,14h
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
 db 5,'tbyte',11h,0Ah
 db 3,'tr0',10h,90h
 db 3,'tr1',10h,91h
 db 3,'tr2',10h,92h
 db 3,'tr3',10h,93h
 db 3,'tr4',10h,94h
 db 3,'tr5',10h,95h
 db 3,'tr6',10h,96h
 db 3,'tr7',10h,97h
 db 5,'tword',11h,0Ah
 db 5,'use16',13h,16
 db 5,'use32',13h,32
 db 5,'use64',13h,64
 db 4,'word',11h,2
 db 4,'xmm0',10h,0C0h
 db 4,'xmm1',10h,0C1h
 db 5,'xmm10',10h,0CAh
 db 5,'xmm11',10h,0CBh
 db 5,'xmm12',10h,0CCh
 db 5,'xmm13',10h,0CDh
 db 5,'xmm14',10h,0CEh
 db 5,'xmm15',10h,0CFh
 db 4,'xmm2',10h,0C2h
 db 4,'xmm3',10h,0C3h
 db 4,'xmm4',10h,0C4h
 db 4,'xmm5',10h,0C5h
 db 4,'xmm6',10h,0C6h
 db 4,'xmm7',10h,0C7h
 db 4,'xmm8',10h,0C8h
 db 4,'xmm9',10h,0C9h
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
 dw simple_instruction_except64-assembler
 db 'aad',0D5h
 dw aa_instruction-assembler
 db 'aam',0D4h
 dw aa_instruction-assembler
 db 'aas',3Fh
 dw simple_instruction_except64-assembler
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
 db 'cqo',99h
 dw simple_instruction_64bit-assembler
 db 'cwd',99h
 dw simple_instruction_16bit-assembler
 db 'daa',27h
 dw simple_instruction_except64-assembler
 db 'das',2Fh
 dw simple_instruction_except64-assembler
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
 db 'ins',6Ch
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
 db 'sal',4
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
 db 'cdqe',98h
 dw simple_instruction_64bit-assembler
 db 'clts',6
 dw simple_extended_instruction-assembler
 db 'cmps',0A6h
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
 db 'int1',0F1h
 dw simple_instruction-assembler
 db 'int3',0CCh
 dw simple_instruction-assembler
 db 'into',0CEh
 dw simple_instruction_except64-assembler
 db 'invd',8
 dw simple_extended_instruction-assembler
 db 'iret',0CFh
 dw iret_instruction-assembler
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
 dw simple_instruction_except64-assembler
 db 'lgdt',2
 dw lgdt_instruction-assembler
 db 'lidt',3
 dw lgdt_instruction-assembler
 db 'lldt',2
 dw pm_word_instruction-assembler
 db 'lmsw',16h
 dw pm_word_instruction-assembler
 db 'load',0
 dw load_directive-assembler
 db 'lock',0F0h
 dw prefix_instruction-assembler
 db 'lods',0ACh
 dw lods_instruction-assembler
 db 'loop',0E2h
 dw loop_instruction-assembler
 db 'movd',0
 dw movd_instruction-assembler
 db 'movq',0
 dw movq_instruction-assembler
 db 'movs',0A4h
 dw movs_instruction-assembler
 db 'orpd',56h
 dw sse_pd_instruction-assembler
 db 'orps',56h
 dw sse_ps_instruction-assembler
 db 'outs',6Eh
 dw outs_instruction-assembler
 db 'pand',0DBh
 dw mmx_instruction-assembler
 db 'popa',61h
 dw simple_instruction_except64-assembler
 db 'popd',4
 dw pop_instruction-assembler
 db 'popf',9Dh
 dw simple_instruction-assembler
 db 'popq',8
 dw pop_instruction-assembler
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
 dw ret_instruction_32bit_except64-assembler
 db 'retf',0CAh
 dw retf_instruction-assembler
 db 'retn',0C2h
 dw ret_instruction-assembler
 db 'retq',0C2h
 dw ret_instruction_only64-assembler
 db 'retw',0C2h
 dw ret_instruction_16bit-assembler
 db 'sahf',9Eh
 dw simple_instruction_except64-assembler
 db 'salc',0D6h
 dw simple_instruction_except64-assembler
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
 dw lgdt_instruction-assembler
 db 'shld',0A4h
 dw shd_instruction-assembler
 db 'shrd',0ACh
 dw shd_instruction-assembler
 db 'sidt',1
 dw lgdt_instruction-assembler
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
 db 'align',0
 dw align_directive-assembler
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
 db 'cmpsq',0
 dw simple_instruction_64bit-assembler
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
 db 'iretq',0CFh
 dw simple_instruction_64bit-assembler
 db 'iretw',0CFh
 dw simple_instruction_16bit-assembler
 db 'jecxz',0E3h
 dw loop_instruction_32bit-assembler
 db 'jrcxz',0E3h
 dw loop_instruction_64bit-assembler
 db 'label',0
 dw label_directive-assembler
 db 'lddqu',0
 dw lddqu_instruction-assembler
 db 'leave',0C9h
 dw simple_instruction-assembler
 db 'lodsb',0ACh
 dw simple_instruction-assembler
 db 'lodsd',0ADh
 dw simple_instruction_32bit-assembler
 db 'lodsq',0ADh
 dw simple_instruction_64bit-assembler
 db 'lodsw',0ADh
 dw simple_instruction_16bit-assembler
 db 'loopd',0E2h
 dw loop_instruction_32bit-assembler
 db 'loope',0E1h
 dw loop_instruction-assembler
 db 'loopq',0E2h
 dw loop_instruction_64bit-assembler
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
 db 'movsq',0A5h
 dw simple_instruction_64bit-assembler
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
 db 'mwait',0C9h
 dw monitor_instruction-assembler
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
 db 'pf2id',1Dh
 dw amd3dnow_instruction-assembler
 db 'pf2iw',1Ch
 dw amd3dnow_instruction-assembler
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
 db 'popad',61h
 dw simple_instruction_32bit_except64-assembler
 db 'popaw',61h
 dw simple_instruction_16bit_except64-assembler
 db 'popfd',9Dh
 dw simple_instruction_32bit_except64-assembler
 db 'popfw',9Dh
 dw simple_instruction_16bit-assembler
 db 'popfq',9Dh
 dw simple_instruction_only64-assembler
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
 dw simple_instruction_except64-assembler
 db 'pushd',4
 dw push_instruction-assembler
 db 'pushf',9Ch
 dw simple_instruction-assembler
 db 'pushq',8
 dw push_instruction-assembler
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
 db 'retfq',0CAh
 dw ret_instruction_64bit-assembler
 db 'retfw',0CAh
 dw ret_instruction_16bit-assembler
 db 'retnd',0C2h
 dw ret_instruction_32bit_except64-assembler
 db 'retnq',0C2h
 dw ret_instruction_only64-assembler
 db 'retnw',0C2h
 dw ret_instruction_16bit-assembler
 db 'scasb',0AEh
 dw simple_instruction-assembler
 db 'scasd',0AFh
 dw simple_instruction_32bit-assembler
 db 'scasq',0AFh
 dw simple_instruction_64bit-assembler
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
 db 'store',0
 dw store_directive-assembler
 db 'stosb',0AAh
 dw simple_instruction-assembler
 db 'stosd',0ABh
 dw simple_instruction_32bit-assembler
 db 'stosq',0ABh
 dw simple_instruction_64bit-assembler
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
 db 'vmxon',6
 dw vmxon_instruction-assembler
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
 db 'fisttp',1
 dw fild_instruction-assembler
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
 db 'frstpm',0E5h
 dw fninit_instruction-assembler
 db 'fscale',111101b
 dw simple_fpu_instruction-assembler
 db 'fsetpm',0E4h
 dw fninit_instruction-assembler
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
 db 'haddpd',07Ch
 dw sse_pd_instruction-assembler
 db 'haddps',07Ch
 dw cvtpd2dq_instruction-assembler
 db 'hsubpd',07Dh
 dw sse_pd_instruction-assembler
 db 'hsubps',07Dh
 dw cvtpd2dq_instruction-assembler
 db 'invlpg',0
 dw invlpg_instruction-assembler
 db 'lfence',0E8h
 dw fence_instruction-assembler
 db 'looped',0E1h
 dw loop_instruction_32bit-assembler
 db 'loopeq',0E1h
 dw loop_instruction_64bit-assembler
 db 'loopew',0E1h
 dw loop_instruction_16bit-assembler
 db 'loopne',0E0h
 dw loop_instruction-assembler
 db 'loopnz',0E0h
 dw loop_instruction-assembler
 db 'loopzd',0E1h
 dw loop_instruction_32bit-assembler
 db 'loopzq',0E1h
 dw loop_instruction_64bit-assembler
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
 db 'movsxd',63h
 dw movsxd_instruction-assembler
 db 'movupd',10h
 dw movpd_instruction-assembler
 db 'movups',10h
 dw movps_instruction-assembler
 db 'paddsb',0ECh
 dw mmx_instruction-assembler
 db 'paddsw',0EDh
 dw mmx_instruction-assembler
 db 'pextrw',0C5h
 dw pextrw_instruction-assembler
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
 dw simple_instruction_32bit_except64-assembler
 db 'pushaw',60h
 dw simple_instruction_16bit_except64-assembler
 db 'pushfd',9Ch
 dw simple_instruction_32bit_except64-assembler
 db 'pushfq',9Ch
 dw simple_instruction_only64-assembler
 db 'pushfw',9Ch
 dw simple_instruction_16bit-assembler
 db 'repeat',0
 dw repeat_directive-assembler
 db 'setalc',0D6h
 dw simple_instruction_except64-assembler
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
 db 'sysret',07h
 dw simple_extended_instruction-assembler
 db 'swapgs',0
 dw swapgs_instruction-assembler
 db 'vmcall',0C1h
 dw simple_vmx_instruction-assembler
 db 'vmread',0
 dw vmread_instruction-assembler
 db 'vmxoff',0C4h
 dw simple_vmx_instruction-assembler
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
 db 'loopneq',0E0h
 dw loop_instruction_64bit-assembler
 db 'loopnew',0E0h
 dw loop_instruction_16bit-assembler
 db 'loopnzd',0E0h
 dw loop_instruction_32bit-assembler
 db 'loopnzq',0E0h
 dw loop_instruction_64bit-assembler
 db 'loopnzw',0E0h
 dw loop_instruction_16bit-assembler
 db 'monitor',0C8h
 dw monitor_instruction-assembler
 db 'movddup',12h
 dw sse_sd_instruction-assembler
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
 db 'syscall',05h
 dw simple_extended_instruction-assembler
 db 'sysexit',35h
 dw simple_extended_instruction-assembler
 db 'ucomisd',2Eh
 dw comisd_instruction-assembler
 db 'ucomiss',2Eh
 dw comiss_instruction-assembler
 db 'virtual',0
 dw virtual_directive-assembler
 db 'vmclear',6
 dw vmclear_instruction-assembler
 db 'vmptrld',6
 dw vmx_instruction-assembler
 db 'vmptrst',7
 dw vmx_instruction-assembler
 db 'vmwrite',0
 dw vmwrite_instruction-assembler
 db 0
instructions_8:
 db 'addsubpd',0D0h
 dw sse_pd_instruction-assembler
 db 'addsubps',0D0h
 dw cvtpd2dq_instruction-assembler
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
 db 'movshdup',16h
 dw cvtdq2pd_instruction-assembler
 db 'movsldup',12h
 dw cvtdq2pd_instruction-assembler
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
 dw pextrw_instruction-assembler
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
 db 'vmlaunch',0C2h
 dw simple_vmx_instruction-assembler
 db 'vmresume',0C3h
 dw simple_vmx_instruction-assembler
 db 0
instructions_9:
 db 'cmpxchg8b',8
 dw cmpxchgx_instruction-assembler
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
 db 'cmpxchg16b',16
 dw cmpxchgx_instruction-assembler
 db 'loadall286',5
 dw simple_extended_instruction-assembler
 db 'loadall386',7
 dw simple_extended_instruction-assembler
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
