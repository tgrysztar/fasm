
; flat assembler core
; Copyright (c) 1999-2019, Tomasz Grysztar.
; All rights reserved.

avx_single_source_pd_instruction_er_evex:
	or	[vex_required],8
avx_single_source_pd_instruction_er:
	or	[operand_flags],2+4+8
	jmp	avx_pd_instruction
avx_single_source_pd_instruction_sae_evex:
	or	[vex_required],8
	or	[operand_flags],2+4
	jmp	avx_pd_instruction
avx_pd_instruction_imm8:
	mov	[immediate_size],1
	jmp	avx_pd_instruction
avx_pd_instruction_er:
	or	[operand_flags],8
avx_pd_instruction_sae:
	or	[operand_flags],4
avx_pd_instruction:
	mov	[opcode_prefix],66h
	or	[rex_prefix],80h
	mov	cx,0800h
	jmp	avx_instruction_with_broadcast
avx_pd_instruction_38_evex:
	or	[vex_required],8
	mov	[supplemental_code],al
	mov	al,38h
	jmp	avx_pd_instruction
avx_cvtps2dq_instruction:
	mov	[opcode_prefix],66h
	jmp	avx_single_source_ps_instruction_er
avx_cvtudq2ps_instruction:
	mov	[opcode_prefix],0F2h
avx_single_source_ps_instruction_er_evex:
	or	[vex_required],8
avx_single_source_ps_instruction_er:
	or	[operand_flags],2+4+8
	jmp	avx_ps_instruction
avx_single_source_ps_instruction_noevex:
	or	[operand_flags],2
	or	[vex_required],2
	jmp	avx_ps_instruction
avx_ps_instruction_imm8:
	mov	[immediate_size],1
	jmp	avx_ps_instruction
avx_ps_instruction_er:
	or	[operand_flags],8
avx_ps_instruction_sae:
	or	[operand_flags],4
avx_ps_instruction:
	mov	cx,0400h
	jmp	avx_instruction_with_broadcast
avx_ps_instruction_66_38_evex:
	or	[vex_required],8
	mov	[opcode_prefix],66h
	mov	[supplemental_code],al
	mov	al,38h
	jmp	avx_ps_instruction
avx_sd_instruction_er:
	or	[operand_flags],8
avx_sd_instruction_sae:
	or	[operand_flags],4
avx_sd_instruction:
	mov	[opcode_prefix],0F2h
	or	[rex_prefix],80h
	mov	cl,8
	jmp	avx_instruction
avx_ss_instruction_er:
	or	[operand_flags],8
avx_ss_instruction_sae:
	or	[operand_flags],4
avx_ss_instruction:
	mov	[opcode_prefix],0F3h
	mov	cl,4
	jmp	avx_instruction
avx_ss_instruction_noevex:
	or	[vex_required],2
	jmp	avx_ss_instruction
avx_single_source_q_instruction_38_evex:
	or	[operand_flags],2
avx_q_instruction_38_evex:
	or	[vex_required],8
avx_q_instruction_38:
	mov	[supplemental_code],al
	mov	al,38h
	jmp	avx_q_instruction
avx_q_instruction_38_w1_evex:
	or	[vex_required],8
avx_q_instruction_38_w1:
	or	[rex_prefix],8
	jmp	avx_q_instruction_38
avx_q_instruction_3a_imm8_evex:
	mov	[immediate_size],1
	or	[vex_required],8
	mov	[supplemental_code],al
	mov	al,3Ah
	jmp	avx_q_instruction
avx_q_instruction_evex:
	or	[vex_required],8
avx_q_instruction:
	or	[rex_prefix],80h
	mov	ch,8
	jmp	avx_pi_instruction
avx_single_source_d_instruction_38_evex:
	or	[vex_required],8
avx_single_source_d_instruction_38:
	or	[operand_flags],2
	jmp	avx_d_instruction_38
avx_d_instruction_38_evex:
	or	[vex_required],8
avx_d_instruction_38:
	mov	[supplemental_code],al
	mov	al,38h
	jmp	avx_d_instruction
avx_d_instruction_3a_imm8_evex:
	mov	[immediate_size],1
	or	[vex_required],8
	mov	[supplemental_code],al
	mov	al,3Ah
	jmp	avx_d_instruction
avx_single_source_d_instruction_imm8:
	or	[operand_flags],2
	mov	[immediate_size],1
	jmp	avx_d_instruction
avx_d_instruction_evex:
	or	[vex_required],8
avx_d_instruction:
	mov	ch,4
	jmp	avx_pi_instruction
avx_single_source_bw_instruction_38:
	or	[operand_flags],2
avx_bw_instruction_38:
	mov	[supplemental_code],al
	mov	al,38h
avx_bw_instruction:
	xor	ch,ch
      avx_pi_instruction:
	mov	[opcode_prefix],66h
	xor	cl,cl
	jmp	avx_instruction_with_broadcast
avx_bw_instruction_38_w1_evex:
	or	[rex_prefix],8
avx_bw_instruction_38_evex:
	or	[vex_required],8
	jmp	avx_bw_instruction_38
avx_pd_instruction_noevex:
	xor	cl,cl
	or	[vex_required],2
	mov	[opcode_prefix],66h
	jmp	avx_instruction
avx_ps_instruction_noevex:
	or	[vex_required],2
	mov	[opcode_prefix],0F2h
	xor	cl,cl
	jmp	avx_instruction
avx_instruction:
	xor	ch,ch
      avx_instruction_with_broadcast:
	mov	[mmx_size],cl
	mov	[broadcast_size],ch
	mov	[base_code],0Fh
	mov	[extended_code],al
      avx_xop_common:
	or	[vex_required],1
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
      avx_reg:
	lods	byte [esi]
	call	convert_avx_register
	mov	[postbyte_register],al
	call	take_avx512_mask
      avx_vex_reg:
	test	[operand_flags],2
	jnz	avx_vex_reg_ok
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[vex_register],al
      avx_vex_reg_ok:
	mov	al,[mmx_size]
	or	al,al
	jz	avx_regs_size_ok
	mov	ah,[operand_size]
	or	ah,ah
	jz	avx_regs_size_ok
	cmp	al,ah
	je	avx_regs_size_ok
	ja	invalid_operand_size
	cmp	ah,16
	jne	invalid_operand_size
      avx_regs_size_ok:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
      avx_regs_rm:
	call	take_avx_rm
	jc	avx_regs_reg
	mov	al,[immediate_size]
	cmp	al,1
	je	mmx_imm8
	jb	instruction_ready
	cmp	al,-4
	je	sse_cmp_mem_ok
	cmp	byte [esi],','
	jne	invalid_operand
	inc	esi
	call	take_avx_register
	shl	al,4
	jc	invalid_operand
	or	byte [value],al
	test	al,80h
	jz	avx_regs_mem_reg_store
	cmp	[code_type],64
	jne	invalid_operand
      avx_regs_mem_reg_store:
	call	take_imm4_if_needed
	call	store_instruction_with_imm8
	jmp	instruction_assembled
      avx_regs_reg:
	mov	bl,al
	call	take_avx512_rounding
	mov	al,[immediate_size]
	cmp	al,1
	je	mmx_nomem_imm8
	jb	nomem_instruction_ready
	cmp	al,-4
	je	sse_cmp_nomem_ok
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	al,bl
	shl	al,4
	jc	invalid_operand
	or	byte [value],al
	test	al,80h
	jz	avx_regs_reg_
	cmp	[code_type],64
	jne	invalid_operand
      avx_regs_reg_:
	call	take_avx_rm
	jc	avx_regs_reg_reg
	cmp	[immediate_size],-2
	jg	invalid_operand
	or	[rex_prefix],8
	call	take_imm4_if_needed
	call	store_instruction_with_imm8
	jmp	instruction_assembled
      avx_regs_reg_reg:
	shl	al,4
	jc	invalid_operand
	and	byte [value],1111b
	or	byte [value],al
	call	take_imm4_if_needed
	call	store_nomem_instruction
	mov	al,byte [value]
	stos	byte [edi]
	jmp	instruction_assembled
      take_avx_rm:
	xor	cl,cl
	xchg	cl,[operand_size]
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	je	take_avx_mem
	cmp	al,10h
	jne	invalid_operand
	mov	[operand_size],cl
	lods	byte [esi]
	call	convert_avx_register
	or	cl,cl
	jnz	avx_reg_ok
	or	cl,[mmx_size]
	jz	avx_reg_ok
	cmp	ah,cl
	je	avx_reg_ok
	jb	invalid_operand_size
	cmp	ah,16
	jne	invalid_operand_size
      avx_reg_ok:
	stc
	ret
      take_avx_mem:
	push	ecx
	call	get_address
	cmp	byte [esi],'{'
	jne	avx_mem_ok
	inc	esi
	lods	byte [esi]
	cmp	al,1Fh
	jne	invalid_operand
	mov	al,[esi]
	shr	al,4
	cmp	al,1
	jne	invalid_operand
	mov	al,[mmx_size]
	or	al,al
	jnz	avx_mem_broadcast_check
	mov	eax,[esp]
	or	al,al
	jnz	avx_mem_broadcast_check
	mov	al,[broadcast_size]
	mov	[mmx_size],al
	mov	ah,cl
	lods	byte [esi]
	and	al,1111b
	mov	cl,al
	mov	al,[broadcast_size]
	shl	al,cl
	mov	[esp],al
	mov	cl,ah
	jmp	avx_mem_broadcast_ok
      avx_mem_broadcast_check:
	bsf	eax,eax
	xchg	al,[broadcast_size]
	mov	[mmx_size],al
	bsf	eax,eax
	jz	invalid_operand
	mov	ah,[broadcast_size]
	sub	ah,al
	lods	byte [esi]
	and	al,1111b
	cmp	al,ah
	jne	invalid_operand_size
      avx_mem_broadcast_ok:
	or	[vex_required],40h
	lods	byte [esi]
	cmp	al,'}'
	jne	invalid_operand
      avx_mem_ok:
	pop	eax
	or	al,al
	jz	avx_mem_size_deciding
	xchg	al,[operand_size]
	cmp	[mmx_size],0
	jne	avx_mem_size_enforced
	or	al,al
	jz	avx_mem_size_ok
	cmp	al,[operand_size]
	jne	operand_sizes_do_not_match
      avx_mem_size_ok:
	clc
	ret
      avx_mem_size_deciding:
	mov	al,[operand_size]
	cmp	[mmx_size],0
	jne	avx_mem_size_enforced
	cmp	al,16
	je	avx_mem_size_ok
	cmp	al,32
	je	avx_mem_size_ok
	cmp	al,64
	je	avx_mem_size_ok
	or	al,al
	jnz	invalid_operand_size
	call	recoverable_unknown_size
      avx_mem_size_enforced:
	or	al,al
	jz	avx_mem_size_ok
	cmp	al,[mmx_size]
	je	avx_mem_size_ok
	jmp	invalid_operand_size
      take_imm4_if_needed:
	cmp	[immediate_size],-3
	jne	imm4_ok
	push	ebx ecx edx
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,'('
	jne	invalid_operand
	call	get_byte_value
	test	al,11110000b
	jnz	value_out_of_range
	or	byte [value],al
	pop	edx ecx ebx
      imm4_ok:
	ret
      take_avx512_mask:
	cmp	byte [esi],'{'
	jne	avx512_masking_ok
	test	[operand_flags],10h
	jnz	invalid_operand
	inc	esi
	lods	byte [esi]
	cmp	al,14h
	jne	invalid_operand
	lods	byte [esi]
	mov	ah,al
	shr	ah,4
	cmp	ah,5
	jne	invalid_operand
	or	al,al
	jz	invalid_operand
	and	al,111b
	mov	[mask_register],al
	or	[vex_required],20h
	lods	byte [esi]
	cmp	al,'}'
	jne	invalid_operand
	cmp	byte [esi],'{'
	jne	avx512_masking_ok
	test	[operand_flags],20h
	jnz	invalid_operand
	inc	esi
	lods	byte [esi]
	cmp	al,1Fh
	jne	invalid_operand
	lods	byte [esi]
	or	al,al
	jnz	invalid_operand
	or	[mask_register],80h
	lods	byte [esi]
	cmp	al,'}'
	jne	invalid_operand
      avx512_masking_ok:
	retn
      take_avx512_rounding:
	test	[operand_flags],4+8
	jz	avx512_rounding_done
	test	[operand_flags],8
	jz	avx512_rounding_allowed
	cmp	[mmx_size],0
	jne	avx512_rounding_allowed
	cmp	[operand_size],64
	jne	avx512_rounding_done
      avx512_rounding_allowed:
	cmp	byte [esi],','
	jne	avx512_rounding_done
	cmp	byte [esi+1],'{'
	jne	avx512_rounding_done
	add	esi,2
	mov	[rounding_mode],0
	or	[vex_required],40h
	test	[operand_flags],8
	jz	take_sae
	or	[vex_required],80h
	lods	byte [esi]
	cmp	al,1Fh
	jne	invalid_operand
	lods	byte [esi]
	mov	ah,al
	shr	ah,4
	cmp	ah,2
	jne	invalid_operand
	and	al,11b
	mov	[rounding_mode],al
	lods	byte [esi]
	cmp	al,'-'
	jne	invalid_operand
      take_sae:
	lods	byte [esi]
	cmp	al,1Fh
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,30h
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,'}'
	jne	invalid_operand
      avx512_rounding_done:
	retn

avx_movdqu_instruction:
	mov	ah,0F3h
	jmp	avx_movdq_instruction
avx_movdqa_instruction:
	mov	ah,66h
      avx_movdq_instruction:
	mov	[opcode_prefix],ah
	or	[vex_required],2
	jmp	avx_movps_instruction
avx512_movdqu16_instruction:
	or	[rex_prefix],8
avx512_movdqu8_instruction:
	mov	ah,0F2h
	jmp	avx_movdq_instruction_evex
avx512_movdqu64_instruction:
	or	[rex_prefix],8
avx512_movdqu32_instruction:
	mov	ah,0F3h
	jmp	avx_movdq_instruction_evex
avx512_movdqa64_instruction:
	or	[rex_prefix],8
avx512_movdqa32_instruction:
	mov	ah,66h
      avx_movdq_instruction_evex:
	mov	[opcode_prefix],ah
	or	[vex_required],8
	jmp	avx_movps_instruction
avx_movpd_instruction:
	mov	[opcode_prefix],66h
	or	[rex_prefix],80h
avx_movps_instruction:
	or	[operand_flags],2
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	xor	al,al
	mov	[mmx_size],al
	mov	[broadcast_size],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	avx_reg
	inc	[extended_code]
	test	[extended_code],1
	jnz	avx_mem
	add	[extended_code],-1+10h
      avx_mem:
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	[operand_flags],20h
	call	take_avx512_mask
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[postbyte_register],al
	jmp	instruction_ready
avx_movntpd_instruction:
	or	[rex_prefix],80h
avx_movntdq_instruction:
	mov	[opcode_prefix],66h
avx_movntps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	or	[operand_flags],10h
	mov	[mmx_size],0
	lods	byte [esi]
	call	get_size_operator
	jmp	avx_mem
avx_compress_q_instruction:
	or	[rex_prefix],8
avx_compress_d_instruction:
	or	[vex_required],8
	mov	[mmx_size],0
	call	setup_66_0f_38
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	avx_mem
	lods	byte [esi]
	call	convert_avx_register
	mov	bl,al
	call	take_avx512_mask
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[postbyte_register],al
	jmp	nomem_instruction_ready
avx_lddqu_instruction:
	mov	ah,0F2h
	or	[vex_required],2
      avx_load_instruction:
	mov	[opcode_prefix],ah
	mov	[base_code],0Fh
	mov	[extended_code],al
	mov	[mmx_size],0
	or	[vex_required],1
	call	take_avx_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	jmp	instruction_ready
avx_movntdqa_instruction:
	mov	[supplemental_code],al
	mov	al,38h
	mov	ah,66h
	jmp	avx_load_instruction
avx_movq_instruction:
	or	[rex_prefix],8
	mov	[mmx_size],8
	jmp	avx_mov_instruction
avx_movd_instruction:
	mov	[mmx_size],4
      avx_mov_instruction:
	or	[vex_required],1
	mov	[opcode_prefix],66h
	mov	[base_code],0Fh
	mov	[extended_code],7Eh
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	avx_movd_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[mmx_size]
	not	al
	and	[operand_size],al
	jnz	invalid_operand_size
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_avx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	[postbyte_register],al
	cmp	[mmx_size],8
	jne	instruction_ready
	and	[rex_prefix],not 8
	or	[rex_prefix],80h
	mov	[extended_code],0D6h
	jmp	instruction_ready
      avx_movd_reg:
	lods	byte [esi]
	cmp	al,0C0h
	jae	avx_movd_xmmreg
	call	convert_register
	cmp	ah,[mmx_size]
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
	call	convert_avx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	[postbyte_register],al
      avx_movd_reg_ready:
	test	[rex_prefix],8
	jz	nomem_instruction_ready
	cmp	[code_type],64
	jne	illegal_instruction
	jmp	nomem_instruction_ready
      avx_movd_xmmreg:
	sub	[extended_code],10h
	call	convert_avx_register
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
	je	avx_movd_xmmreg_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[mmx_size]
	cmp	al,8
	jne	avx_movd_xmmreg_mem_ready
	call	avx_movq_xmmreg_xmmreg_opcode
      avx_movd_xmmreg_mem_ready:
	not	al
	test	[operand_size],al
	jnz	invalid_operand_size
	jmp	instruction_ready
      avx_movd_xmmreg_reg:
	lods	byte [esi]
	cmp	al,0C0h
	jae	avx_movq_xmmreg_xmmreg
	call	convert_register
	cmp	ah,[mmx_size]
	jne	invalid_operand_size
	mov	bl,al
	jmp	avx_movd_reg_ready
      avx_movq_xmmreg_xmmreg:
	cmp	[mmx_size],8
	jne	invalid_operand
	call	avx_movq_xmmreg_xmmreg_opcode
	call	convert_avx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	bl,al
	jmp	nomem_instruction_ready
      avx_movq_xmmreg_xmmreg_opcode:
	and	[rex_prefix],not 8
	or	[rex_prefix],80h
	add	[extended_code],10h
	mov	[opcode_prefix],0F3h
	ret
avx_movddup_instruction:
	or	[vex_required],1
	mov	[opcode_prefix],0F2h
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[rex_prefix],80h
	xor	al,al
	mov	[mmx_size],al
	mov	[broadcast_size],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_avx_register
	mov	[postbyte_register],al
	cmp	ah,16
	ja	avx_movddup_size_ok
	mov	[mmx_size],8
      avx_movddup_size_ok:
	call	take_avx512_mask
	jmp	avx_vex_reg_ok
avx_movlpd_instruction:
	mov	[opcode_prefix],66h
	or	[rex_prefix],80h
avx_movlps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	mov	[mmx_size],8
	mov	[broadcast_size],0
	or	[vex_required],1
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	avx_movlps_mem
	lods	byte [esi]
	call	convert_avx_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[vex_register],al
	cmp	[operand_size],16
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_rm
	jc	invalid_operand
	jmp	instruction_ready
      avx_movlps_mem:
	cmp	al,'['
	jne	invalid_operand
	call	get_address
      avx_movlps_mem_:
	mov	al,[operand_size]
	or	al,al
	jz	avx_movlps_mem_size_ok
	cmp	al,[mmx_size]
	jne	invalid_operand_size
	mov	[operand_size],0
      avx_movlps_mem_size_ok:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	cmp	ah,16
	jne	invalid_operand
	mov	[postbyte_register],al
	inc	[extended_code]
	jmp	instruction_ready
avx_movhlps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	call	take_avx_register
	cmp	ah,16
	jne	invalid_operand
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[vex_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	bl,al
	jmp	nomem_instruction_ready
avx_movsd_instruction:
	mov	al,0F2h
	mov	cl,8
	or	[rex_prefix],80h
	jmp	avx_movs_instruction
avx_movss_instruction:
	mov	al,0F3h
	mov	cl,4
      avx_movs_instruction:
	mov	[opcode_prefix],al
	mov	[mmx_size],cl
	or	[vex_required],1
	mov	[base_code],0Fh
	mov	[extended_code],10h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	avx_movs_mem
	lods	byte [esi]
	call	convert_avx_register
	cmp	ah,16
	jne	invalid_operand
	mov	[postbyte_register],al
	call	take_avx512_mask
	xor	cl,cl
	xchg	cl,[operand_size]
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	avx_movs_reg_mem
	mov	[operand_size],cl
	lods	byte [esi]
	call	convert_avx_register
	mov	[vex_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	bl,al
	cmp	bl,8
	jb	nomem_instruction_ready
	inc	[extended_code]
	xchg	bl,[postbyte_register]
	jmp	nomem_instruction_ready
      avx_movs_reg_mem:
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	avx_movs_reg_mem_ok
	cmp	al,[mmx_size]
	jne	invalid_operand_size
      avx_movs_reg_mem_ok:
	jmp	instruction_ready
      avx_movs_mem:
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	[operand_flags],20h
	call	take_avx512_mask
	jmp	avx_movlps_mem_

avx_comiss_instruction:
	or	[operand_flags],2+4+10h
	mov	cl,4
	jmp	avx_instruction
avx_comisd_instruction:
	or	[operand_flags],2+4+10h
	mov	[opcode_prefix],66h
	or	[rex_prefix],80h
	mov	cl,8
	jmp	avx_instruction
avx_movshdup_instruction:
	or	[operand_flags],2
	mov	[opcode_prefix],0F3h
	xor	cl,cl
	jmp	avx_instruction
avx_cvtqq2pd_instruction:
	mov	[opcode_prefix],0F3h
	or	[vex_required],8
	or	[operand_flags],2+4+8
	or	[rex_prefix],8
	mov	cx,0800h
	jmp	avx_instruction_with_broadcast
avx_pshuf_w_instruction:
	mov	[opcode_prefix],al
	or	[operand_flags],2
	mov	[immediate_size],1
	mov	al,70h
	xor	cl,cl
	jmp	avx_instruction
avx_single_source_128bit_instruction_38_noevex:
	or	[operand_flags],2
avx_128bit_instruction_38_noevex:
	mov	cl,16
	jmp	avx_instruction_38_noevex
avx_single_source_instruction_38_noevex:
	or	[operand_flags],2
	jmp	avx_pi_instruction_38_noevex
avx_pi_instruction_38_noevex:
	xor	cl,cl
      avx_instruction_38_noevex:
	or	[vex_required],2
      avx_instruction_38:
	mov	[opcode_prefix],66h
	mov	[supplemental_code],al
	mov	al,38h
	jmp	avx_instruction
avx_ss_instruction_3a_imm8_noevex:
	mov	cl,4
	jmp	avx_instruction_3a_imm8_noevex
avx_sd_instruction_3a_imm8_noevex:
	mov	cl,8
	jmp	avx_instruction_3a_imm8_noevex
avx_single_source_128bit_instruction_3a_imm8_noevex:
	or	[operand_flags],2
avx_128bit_instruction_3a_imm8_noevex:
	mov	cl,16
	jmp	avx_instruction_3a_imm8_noevex
avx_triple_source_instruction_3a_noevex:
	xor	cl,cl
	mov	[immediate_size],-1
	mov	byte [value],0
	jmp	avx_instruction_3a_noevex
avx_single_source_instruction_3a_imm8_noevex:
	or	[operand_flags],2
avx_pi_instruction_3a_imm8_noevex:
	xor	cl,cl
      avx_instruction_3a_imm8_noevex:
	mov	[immediate_size],1
      avx_instruction_3a_noevex:
	or	[vex_required],2
      avx_instruction_3a:
	mov	[opcode_prefix],66h
	mov	[supplemental_code],al
	mov	al,3Ah
	jmp	avx_instruction
avx_pi_instruction_3a_imm8:
	xor	cl,cl
	mov	[immediate_size],1
	jmp	avx_instruction_3a
avx_pclmulqdq_instruction:
	mov	byte [value],al
	mov	[immediate_size],-4
	or	[vex_required],2
	mov	cl,16
	mov	al,44h
	jmp	avx_instruction_3a

avx512_single_source_pd_instruction_sae_imm8:
	or	[operand_flags],2
avx512_pd_instruction_sae_imm8:
	or	[rex_prefix],8
	mov	cx,0800h
	jmp	avx512_instruction_sae_imm8
avx512_single_source_ps_instruction_sae_imm8:
	or	[operand_flags],2
avx512_ps_instruction_sae_imm8:
	mov	cx,0400h
	jmp	avx512_instruction_sae_imm8
avx512_sd_instruction_sae_imm8:
	or	[rex_prefix],8
	mov	cx,0008h
	jmp	avx512_instruction_sae_imm8
avx512_ss_instruction_sae_imm8:
	mov	cx,0004h
      avx512_instruction_sae_imm8:
	or	[operand_flags],4
      avx512_instruction_imm8:
	or	[vex_required],8
	mov	[opcode_prefix],66h
	mov	[immediate_size],1
	mov	[supplemental_code],al
	mov	al,3Ah
	jmp	avx_instruction_with_broadcast
avx512_pd_instruction_er:
	or	[operand_flags],4+8
	jmp	avx512_pd_instruction
avx512_single_source_pd_instruction_sae:
	or	[operand_flags],4
avx512_single_source_pd_instruction:
	or	[operand_flags],2
avx512_pd_instruction:
	or	[rex_prefix],8
	mov	cx,0800h
	jmp	avx512_instruction
avx512_ps_instruction_er:
	or	[operand_flags],4+8
	jmp	avx512_ps_instruction
avx512_single_source_ps_instruction_sae:
	or	[operand_flags],4
avx512_single_source_ps_instruction:
	or	[operand_flags],2
avx512_ps_instruction:
	mov	cx,0400h
	jmp	avx512_instruction
avx512_sd_instruction_er:
	or	[operand_flags],8
avx512_sd_instruction_sae:
	or	[operand_flags],4
avx512_sd_instruction:
	or	[rex_prefix],8
	mov	cx,0008h
	jmp	avx512_instruction
avx512_ss_instruction_er:
	or	[operand_flags],8
avx512_ss_instruction_sae:
	or	[operand_flags],4
avx512_ss_instruction:
	mov	cx,0004h
      avx512_instruction:
	or	[vex_required],8
	mov	[opcode_prefix],66h
	mov	[supplemental_code],al
	mov	al,38h
	jmp	avx_instruction_with_broadcast
avx512_exp2pd_instruction:
	or	[rex_prefix],8
	or	[operand_flags],2+4
	mov	cx,0840h
	jmp	avx512_instruction
avx512_exp2ps_instruction:
	or	[operand_flags],2+4
	mov	cx,0440h
	jmp	avx512_instruction

fma_instruction_pd:
	or	[rex_prefix],8
	mov	cx,0800h
	jmp	fma_instruction
fma_instruction_ps:
	mov	cx,0400h
	jmp	fma_instruction
fma_instruction_sd:
	or	[rex_prefix],8
	mov	cx,0008h
	jmp	fma_instruction
fma_instruction_ss:
	mov	cx,0004h
      fma_instruction:
	or	[operand_flags],4+8
	mov	[opcode_prefix],66h
	mov	[supplemental_code],al
	mov	al,38h
	jmp	avx_instruction_with_broadcast

fma4_instruction_p:
	xor	cl,cl
	jmp	fma4_instruction
fma4_instruction_sd:
	mov	cl,8
	jmp	fma4_instruction
fma4_instruction_ss:
	mov	cl,4
      fma4_instruction:
	mov	[immediate_size],-2
	mov	byte [value],0
	jmp	avx_instruction_3a_noevex

avx_cmp_pd_instruction:
	mov	[opcode_prefix],66h
	or	[rex_prefix],80h
	mov	cx,0800h
	jmp	avx_cmp_instruction
avx_cmp_ps_instruction:
	mov	cx,0400h
	jmp	avx_cmp_instruction
avx_cmp_sd_instruction:
	mov	[opcode_prefix],0F2h
	or	[rex_prefix],80h
	mov	cx,0008h
	jmp	avx_cmp_instruction
avx_cmp_ss_instruction:
	mov	[opcode_prefix],0F3h
	mov	cx,0004h
      avx_cmp_instruction:
	mov	byte [value],al
	mov	[immediate_size],-4
	or	[operand_flags],4+20h
	mov	al,0C2h
	jmp	avx_cmp_common
avx_cmpeqq_instruction:
	or	[rex_prefix],80h
	mov	ch,8
	mov	[supplemental_code],al
	mov	al,38h
	jmp	avx_cmp_pi_instruction
avx_cmpeqd_instruction:
	mov	ch,4
	jmp	avx_cmp_pi_instruction
avx_cmpeqb_instruction:
	xor	ch,ch
	jmp	avx_cmp_pi_instruction
avx512_cmp_uq_instruction:
	or	[rex_prefix],8
	mov	ch,8
	mov	ah,1Eh
	jmp	avx_cmp_pi_instruction_evex
avx512_cmp_ud_instruction:
	mov	ch,4
	mov	ah,1Eh
	jmp	avx_cmp_pi_instruction_evex
avx512_cmp_q_instruction:
	or	[rex_prefix],8
	mov	ch,8
	mov	ah,1Fh
	jmp	avx_cmp_pi_instruction_evex
avx512_cmp_d_instruction:
	mov	ch,4
	mov	ah,1Fh
	jmp	avx_cmp_pi_instruction_evex
avx512_cmp_uw_instruction:
	or	[rex_prefix],8
avx512_cmp_ub_instruction:
	xor	ch,ch
	mov	ah,3Eh
	jmp	avx_cmp_pi_instruction_evex
avx512_cmp_w_instruction:
	or	[rex_prefix],8
avx512_cmp_b_instruction:
	xor	ch,ch
	mov	ah,3Fh
      avx_cmp_pi_instruction_evex:
	mov	byte [value],al
	mov	[immediate_size],-4
	mov	[supplemental_code],ah
	mov	al,3Ah
	or	[vex_required],8
      avx_cmp_pi_instruction:
	xor	cl,cl
	or	[operand_flags],20h
	mov	[opcode_prefix],66h
      avx_cmp_common:
	mov	[mmx_size],cl
	mov	[broadcast_size],ch
	mov	[extended_code],al
	mov	[base_code],0Fh
	lods	byte [esi]
	call	get_size_operator
	cmp	al,14h
	je	avx_maskreg
	cmp	al,10h
	jne	invalid_operand
	or	[vex_required],2
	jmp	avx_reg
      avx_maskreg:
	cmp	[operand_size],0
	jne	invalid_operand_size
	or	[vex_required],8
	lods	byte [esi]
	call	convert_mask_register
	mov	[postbyte_register],al
	call	take_avx512_mask
	jmp	avx_vex_reg
avx512_fpclasspd_instruction:
	or	[rex_prefix],8
	mov	cx,0800h
	jmp	avx_fpclass_instruction
avx512_fpclassps_instruction:
	mov	cx,0400h
	jmp	avx_fpclass_instruction
avx512_fpclasssd_instruction:
	or	[rex_prefix],8
	mov	cx,0008h
	jmp	avx_fpclass_instruction
avx512_fpclassss_instruction:
	mov	cx,0004h
      avx_fpclass_instruction:
	mov	[broadcast_size],ch
	mov	[mmx_size],cl
	or	[operand_flags],2
	call	setup_66_0f_3a
	mov	[immediate_size],1
	lods	byte [esi]
	cmp	al,14h
	je	avx_maskreg
	jmp	invalid_operand
avx512_ptestnmd_instruction:
	mov	ch,4
	jmp	avx512_ptestnm_instruction
avx512_ptestnmq_instruction:
	or	[rex_prefix],8
	mov	ch,8
	jmp	avx512_ptestnm_instruction
avx512_ptestnmw_instruction:
	or	[rex_prefix],8
avx512_ptestnmb_instruction:
	xor	ch,ch
      avx512_ptestnm_instruction:
	mov	ah,0F3h
	jmp	avx512_ptest_instruction
avx512_ptestmd_instruction:
	mov	ch,4
	jmp	avx512_ptestm_instruction
avx512_ptestmq_instruction:
	or	[rex_prefix],8
	mov	ch,8
	jmp	avx512_ptestm_instruction
avx512_ptestmw_instruction:
	or	[rex_prefix],8
avx512_ptestmb_instruction:
	xor	ch,ch
      avx512_ptestm_instruction:
	mov	ah,66h
      avx512_ptest_instruction:
	xor	cl,cl
	mov	[opcode_prefix],ah
	mov	[supplemental_code],al
	mov	al,38h
	or	[vex_required],8
	jmp	avx_cmp_common

mask_shift_instruction_q:
	or	[rex_prefix],8
mask_shift_instruction_d:
	or	[operand_flags],2
	or	[immediate_size],1
	mov	[opcode_prefix],66h
	mov	[supplemental_code],al
	mov	al,3Ah
	jmp	mask_instruction
mask_instruction_single_source_b:
	mov	[opcode_prefix],66h
	jmp	mask_instruction_single_source_w
mask_instruction_single_source_d:
	mov	[opcode_prefix],66h
mask_instruction_single_source_q:
	or	[rex_prefix],8
mask_instruction_single_source_w:
	or	[operand_flags],2
	jmp	mask_instruction
mask_instruction_b:
	mov	[opcode_prefix],66h
	jmp	mask_instruction_w
mask_instruction_d:
	mov	[opcode_prefix],66h
mask_instruction_q:
	or	[rex_prefix],8
mask_instruction_w:
	mov	[operand_size],32
mask_instruction:
	or	[vex_required],1
	mov	[base_code],0Fh
	mov	[extended_code],al
	call	take_mask_register
	mov	[postbyte_register],al
	test	[operand_flags],2
	jnz	mask_instruction_nds_ok
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_mask_register
	mov	[vex_register],al
      mask_instruction_nds_ok:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_mask_register
	mov	bl,al
	cmp	[immediate_size],0
	jne	mmx_nomem_imm8
	jmp	nomem_instruction_ready
take_mask_register:
	lods	byte [esi]
	cmp	al,14h
	jne	invalid_operand
	lods	byte [esi]
convert_mask_register:
	mov	ah,al
	shr	ah,4
	cmp	ah,5
	jne	invalid_operand
	and	al,1111b
	ret
kmov_instruction:
	mov	[mmx_size],al
	or	[vex_required],1
	mov	[base_code],0Fh
	mov	[extended_code],90h
	lods	byte [esi]
	cmp	al,14h
	je	kmov_maskreg
	cmp	al,10h
	je	kmov_reg
	call	get_size_operator
	inc	[extended_code]
	cmp	al,'['
	jne	invalid_argument
	call	get_address
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_mask_register
	mov	[postbyte_register],al
      kmov_with_mem:
	mov	ah,[mmx_size]
	mov	al,[operand_size]
	or	al,al
	jz	kmov_mem_size_ok
	cmp	al,ah
	jne	invalid_operand_size
      kmov_mem_size_ok:
	call	setup_kmov_prefix
	jmp	instruction_ready
      setup_kmov_prefix:
	cmp	ah,4
	jb	kmov_w_ok
	or	[rex_prefix],8
      kmov_w_ok:
	test	ah,1 or 4
	jz	kmov_prefix_ok
	mov	[opcode_prefix],66h
      kmov_prefix_ok:
	ret
      kmov_maskreg:
	lods	byte [esi]
	call	convert_mask_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,14h
	je	kmov_maskreg_maskreg
	cmp	al,10h
	je	kmov_maskreg_reg
	call	get_size_operator
	cmp	al,'['
	jne	invalid_argument
	call	get_address
	jmp	kmov_with_mem
      kmov_maskreg_maskreg:
	lods	byte [esi]
	call	convert_mask_register
	mov	bl,al
	mov	ah,[mmx_size]
	call	setup_kmov_prefix
	jmp	nomem_instruction_ready
      kmov_maskreg_reg:
	add	[extended_code],2
	lods	byte [esi]
	call	convert_register
      kmov_with_reg:
	mov	bl,al
	mov	al,[mmx_size]
	mov	ah,4
	cmp	al,ah
	jbe	kmov_reg_size_check
	mov	ah,al
      kmov_reg_size_check:
	cmp	ah,[operand_size]
	jne	invalid_operand_size
	cmp	al,8
	je	kmov_f2_w1
	cmp	al,2
	ja	kmov_f2
	je	nomem_instruction_ready
	mov	[opcode_prefix],66h
	jmp	nomem_instruction_ready
      kmov_f2_w1:
	or	[rex_prefix],8
	cmp	[code_type],64
	jne	illegal_instruction
      kmov_f2:
	mov	[opcode_prefix],0F2h
	jmp	nomem_instruction_ready
      kmov_reg:
	add	[extended_code],3
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_mask_register
	jmp	kmov_with_reg
avx512_pmov_m2_instruction_w1:
	or	[rex_prefix],8
avx512_pmov_m2_instruction:
	or	[vex_required],8
	call	setup_f3_0f_38
	call	take_avx_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_mask_register
	mov	bl,al
	jmp	nomem_instruction_ready
avx512_pmov_2m_instruction_w1:
	or	[rex_prefix],8
avx512_pmov_2m_instruction:
	or	[vex_required],8
	call	setup_f3_0f_38
	call	take_mask_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	bl,al
	jmp	nomem_instruction_ready
      setup_f3_0f_38:
	mov	[extended_code],38h
	mov	[supplemental_code],al
	mov	[base_code],0Fh
	mov	[opcode_prefix],0F3h
	ret

vzeroall_instruction:
	mov	[operand_size],32
vzeroupper_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	and	[displacement_compression],0
	call	store_vex_instruction_code
	jmp	instruction_assembled
vldmxcsr_instruction:
	or	[vex_required],2
	jmp	fxsave_instruction

avx_perm2f128_instruction:
	or	[vex_required],2
	xor	ch,ch
      avx_instruction_imm8_without_128bit:
	mov	[immediate_size],1
	mov	ah,3Ah
	jmp	avx_instruction_without_128bit
avx512_shuf_q_instruction:
	or	[rex_prefix],8
	or	[vex_required],8
	mov	ch,8
	jmp	avx_instruction_imm8_without_128bit
avx512_shuf_d_instruction:
	or	[vex_required],8
	mov	ch,4
	jmp	avx_instruction_imm8_without_128bit
avx_permd_instruction:
	mov	ah,38h
	mov	ch,4
      avx_instruction_without_128bit:
	xor	cl,cl
	call	setup_avx_66_supplemental
	call	take_avx_register
	cmp	ah,32
	jb	invalid_operand_size
	mov	[postbyte_register],al
	call	take_avx512_mask
	jmp	avx_vex_reg
      setup_avx_66_supplemental:
	mov	[opcode_prefix],66h
	mov	[broadcast_size],ch
	mov	[mmx_size],cl
	mov	[base_code],0Fh
	mov	[extended_code],ah
	mov	[supplemental_code],al
	or	[vex_required],1
	ret
avx_permq_instruction:
	or	[rex_prefix],8
	mov	ch,8
	jmp	avx_permil_instruction
avx_permilpd_instruction:
	or	[rex_prefix],80h
	mov	ch,8
	jmp	avx_permil_instruction
avx_permilps_instruction:
	mov	ch,4
      avx_permil_instruction:
	or	[operand_flags],2
	xor	cl,cl
	mov	ah,3Ah
	call	setup_avx_66_supplemental
	call	take_avx_register
	cmp	[supplemental_code],4
	jae	avx_permil_size_ok
	cmp	ah,32
	jb	invalid_operand_size
      avx_permil_size_ok:
	mov	[postbyte_register],al
	call	take_avx512_mask
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_rm
	jnc	mmx_imm8
	mov	bl,al
	cmp	byte [esi],','
	jne	invalid_operand
	mov	al,[esi+1]
	cmp	al,11h
	jne	avx_permil_rm_or_imm8
	mov	al,[esi+3]
      avx_permil_rm_or_imm8:
	cmp	al,'('
	je	mmx_nomem_imm8
	mov	[vex_register],bl
	inc	esi
	mov	[extended_code],38h
	mov	al,[supplemental_code]
	cmp	al,4
	jb	avx_permq_rm
	add	[supplemental_code],8
	jmp	avx_regs_rm
      avx_permq_rm:
	or	[vex_required],8
	shl	al,5
	neg	al
	add	al,36h
	mov	[supplemental_code],al
	jmp	avx_regs_rm
vpermil_2pd_instruction:
	mov	[immediate_size],-2
	mov	byte [value],al
	mov	al,49h
	jmp	vpermil2_instruction_setup
vpermil_2ps_instruction:
	mov	[immediate_size],-2
	mov	byte [value],al
	mov	al,48h
	jmp	vpermil2_instruction_setup
vpermil2_instruction:
	mov	[immediate_size],-3
	mov	byte [value],0
      vpermil2_instruction_setup:
	or	[vex_required],2
	mov	[base_code],0Fh
	mov	[supplemental_code],al
	mov	al,3Ah
	xor	cl,cl
	jmp	avx_instruction

avx_shift_q_instruction_evex:
	or	[vex_required],8
avx_shift_q_instruction:
	or	[rex_prefix],80h
	mov	cl,8
	jmp	avx_shift_instruction
avx_shift_d_instruction:
	mov	cl,4
	jmp	avx_shift_instruction
avx_shift_bw_instruction:
	xor	cl,cl
       avx_shift_instruction:
	mov	[broadcast_size],cl
	mov	[mmx_size],0
	mov	[opcode_prefix],66h
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	call	take_avx_register
	mov	[postbyte_register],al
	call	take_avx512_mask
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	xor	cl,cl
	xchg	cl,[operand_size]
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	je	avx_shift_reg_mem
	mov	[operand_size],cl
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_avx_register
	mov	[vex_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	push	esi
	xor	cl,cl
	xchg	cl,[operand_size]
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	avx_shift_reg_reg_reg
	pop	esi
	cmp	al,'['
	je	avx_shift_reg_reg_mem
	xchg	cl,[operand_size]
	test	cl,not 1
	jnz	invalid_operand_size
	dec	esi
	call	convert_avx_shift_opcode
	mov	bl,al
	jmp	mmx_nomem_imm8
      convert_avx_shift_opcode:
	mov	al,[extended_code]
	mov	ah,al
	and	ah,1111b
	add	ah,70h
	mov	[extended_code],ah
	shr	al,4
	sub	al,0Ch
	shl	al,1
	xchg	al,[postbyte_register]
	xchg	al,[vex_register]
	ret
      avx_shift_reg_reg_reg:
	pop	eax
	lods	byte [esi]
	call	convert_xmm_register
	xchg	cl,[operand_size]
	mov	bl,al
	jmp	nomem_instruction_ready
      avx_shift_reg_reg_mem:
	mov	[mmx_size],16
	push	ecx
	lods	byte [esi]
	call	get_size_operator
	call	get_address
	pop	eax
	xchg	al,[operand_size]
	test	al,al
	jz	instruction_ready
	cmp	al,16
	jne	invalid_operand_size
	jmp	instruction_ready
      avx_shift_reg_mem:
	or	[vex_required],8
	call	take_avx_mem
	call	convert_avx_shift_opcode
	jmp	mmx_imm8
avx_shift_dq_instruction:
	mov	[postbyte_register],al
	mov	[opcode_prefix],66h
	mov	[base_code],0Fh
	mov	[extended_code],73h
	or	[vex_required],1
	mov	[mmx_size],0
	call	take_avx_register
	mov	[vex_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	je	avx_shift_dq_reg_mem
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_avx_register
	mov	bl,al
	jmp	mmx_nomem_imm8
      avx_shift_dq_reg_mem:
	or	[vex_required],8
	call	get_address
	jmp	mmx_imm8
avx512_rotate_q_instruction:
	mov	cl,8
	or	[rex_prefix],cl
	jmp	avx512_rotate_instruction
avx512_rotate_d_instruction:
	mov	cl,4
      avx512_rotate_instruction:
	mov	[broadcast_size],cl
	mov	[postbyte_register],al
	mov	[opcode_prefix],66h
	mov	[base_code],0Fh
	mov	[extended_code],72h
	or	[vex_required],8
	mov	[mmx_size],0
	mov	[immediate_size],1
	call	take_avx_register
	mov	[vex_register],al
	call	take_avx512_mask
	jmp	avx_vex_reg_ok

avx_pmovsxbq_instruction:
	mov	cl,2
	jmp	avx_pmovsx_instruction
avx_pmovsxbd_instruction:
	mov	cl,4
	jmp	avx_pmovsx_instruction
avx_pmovsxbw_instruction:
	mov	cl,8
      avx_pmovsx_instruction:
	mov	[mmx_size],cl
	or	[vex_required],1
	call	setup_66_0f_38
	call	take_avx_register
	mov	[postbyte_register],al
	call	take_avx512_mask
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	xor	al,al
	xchg	al,[operand_size]
	bsf	ecx,eax
	sub	cl,4
	shl	[mmx_size],cl
	push	eax
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	avx_pmovsx_reg_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	xchg	al,[operand_size]
	or	al,al
	jz	instruction_ready
	cmp	al,[mmx_size]
	jne	invalid_operand_size
	jmp	instruction_ready
      avx_pmovsx_reg_reg:
	lods	byte [esi]
	call	convert_avx_register
	mov	bl,al
	cmp	ah,[mmx_size]
	je	avx_pmovsx_xmmreg_reg_size_ok
	jb	invalid_operand_size
	cmp	ah,16
	jne	invalid_operand_size
      avx_pmovsx_xmmreg_reg_size_ok:
	pop	eax
	mov	[operand_size],al
	jmp	nomem_instruction_ready
avx512_pmovqb_instruction:
	mov	cl,2
	jmp	avx512_pmov_instruction
avx512_pmovdb_instruction:
	mov	cl,4
	jmp	avx512_pmov_instruction
avx512_pmovwb_instruction:
	mov	cl,8
      avx512_pmov_instruction:
	mov	[mmx_size],cl
	or	[vex_required],8
	mov	[extended_code],38h
	mov	[supplemental_code],al
	mov	[base_code],0Fh
	mov	[opcode_prefix],0F3h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	avx512_pmov_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	or	[operand_flags],20h
	call	avx512_pmov_common
	or	al,al
	jz	instruction_ready
	cmp	al,[mmx_size]
	jne	invalid_operand_size
	jmp	instruction_ready
      avx512_pmov_common:
	call	take_avx512_mask
	xor	al,al
	xchg	al,[operand_size]
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[postbyte_register],al
	mov	al,ah
	mov	ah,cl
	bsf	ecx,eax
	sub	cl,4
	shl	[mmx_size],cl
	mov	cl,ah
	pop	eax
	ret
      avx512_pmov_reg:
	lods	byte [esi]
	call	convert_avx_register
	mov	bl,al
	call	avx512_pmov_common
	cmp	al,[mmx_size]
	je	nomem_instruction_ready
	jb	invalid_operand_size
	cmp	al,16
	jne	invalid_operand_size
	jmp	nomem_instruction_ready

avx_broadcast_128_instruction_noevex:
	or	[vex_required],2
	mov	cl,10h
	jmp	avx_broadcast_instruction
avx512_broadcast_32x2_instruction:
	mov	cl,08h
	jmp	avx_broadcast_instruction_evex
avx512_broadcast_32x4_instruction:
	mov	cl,10h
	jmp	avx_broadcast_instruction_evex
avx512_broadcast_32x8_instruction:
	mov	cl,20h
	jmp	avx_broadcast_instruction_evex
avx512_broadcast_64x2_instruction:
	mov	cl,10h
	jmp	avx_broadcast_instruction_w1_evex
avx512_broadcast_64x4_instruction:
	mov	cl,20h
      avx_broadcast_instruction_w1_evex:
	or	[rex_prefix],8
      avx_broadcast_instruction_evex:
	or	[vex_required],8
	jmp	avx_broadcast_instruction
avx_broadcastss_instruction:
	mov	cl,4
	jmp	avx_broadcast_instruction
avx_broadcastsd_instruction:
	or	[rex_prefix],80h
	mov	cl,8
	jmp	avx_broadcast_instruction
avx_pbroadcastb_instruction:
	mov	cl,1
	jmp	avx_broadcast_pi_instruction
avx_pbroadcastw_instruction:
	mov	cl,2
	jmp	avx_broadcast_pi_instruction
avx_pbroadcastd_instruction:
	mov	cl,4
	jmp	avx_broadcast_pi_instruction
avx_pbroadcastq_instruction:
	mov	cl,8
	or	[rex_prefix],80h
      avx_broadcast_pi_instruction:
	or	[operand_flags],40h
      avx_broadcast_instruction:
	mov	[opcode_prefix],66h
	mov	[supplemental_code],al
	mov	al,38h
	mov	[mmx_size],cl
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	call	take_avx_register
	cmp	ah,[mmx_size]
	je	invalid_operand_size
	test	[operand_flags],40h
	jnz	avx_broadcast_destination_size_ok
	cmp	[mmx_size],4
	je	avx_broadcast_destination_size_ok
	cmp	[supplemental_code],59h
	je	avx_broadcast_destination_size_ok
	cmp	ah,16
	je	invalid_operand_size
      avx_broadcast_destination_size_ok:
	xor	ah,ah
	xchg	ah,[operand_size]
	push	eax
	call	take_avx512_mask
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	avx_broadcast_reg_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	xchg	ah,[operand_size]
	mov	[postbyte_register],al
	mov	al,[broadcast_size]
	mov	al,[mmx_size]
	cmp	al,ah
	je	instruction_ready
	or	al,al
	jz	instruction_ready
	or	ah,ah
	jz	instruction_ready
	jmp	invalid_operand_size
      avx_broadcast_reg_reg:
	lods	byte [esi]
	test	[operand_flags],40h
	jz	avx_broadcast_reg_avx_reg
	cmp	al,60h
	jb	avx_broadcast_reg_general_reg
	cmp	al,80h
	jb	avx_broadcast_reg_avx_reg
	cmp	al,0C0h
	jb	avx_broadcast_reg_general_reg
      avx_broadcast_reg_avx_reg:
	call	convert_avx_register
	mov	bl,al
	mov	al,[mmx_size]
	or	al,al
	jz	avx_broadcast_reg_avx_reg_size_ok
	cmp	ah,16
	jne	invalid_operand_size
	cmp	al,ah
	jae	invalid_operand
      avx_broadcast_reg_avx_reg_size_ok:
	pop	eax
	xchg	ah,[operand_size]
	mov	[postbyte_register],al
	test	[vex_required],2
	jnz	invalid_operand
	jmp	nomem_instruction_ready
      avx_broadcast_reg_general_reg:
	call	convert_register
	mov	bl,al
	mov	al,[mmx_size]
	or	al,al
	jz	avx_broadcast_reg_general_reg_size_ok
	cmp	al,ah
	je	avx_broadcast_reg_general_reg_size_ok
	ja	invalid_operand_size
	cmp	ah,4
	jne	invalid_operand_size
      avx_broadcast_reg_general_reg_size_ok:
	cmp	al,4
	jb	avx_broadcast_reg_general_reg_ready
	cmp	al,8
	mov	al,3
	jne	avx_broadcast_reg_general_reg_ready
	or	[rex_prefix],8
      avx_broadcast_reg_general_reg_ready:
	add	al,7Ah-1
	mov	[supplemental_code],al
	or	[vex_required],8
	pop	eax
	xchg	ah,[operand_size]
	mov	[postbyte_register],al
	jmp	nomem_instruction_ready

avx512_extract_64x4_instruction:
	or	[rex_prefix],8
avx512_extract_32x8_instruction:
	or	[vex_required],8
	mov	cl,32
	jmp	avx_extractf_instruction
avx512_extract_64x2_instruction:
	or	[rex_prefix],8
avx512_extract_32x4_instruction:
	or	[vex_required],8
	mov	cl,16
	jmp	avx_extractf_instruction
avx_extractf128_instruction:
	or	[vex_required],2
	mov	cl,16
      avx_extractf_instruction:
	mov	[mmx_size],cl
	call	setup_66_0f_3a
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	avx_extractf_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	xor	al,al
	xchg	al,[operand_size]
	or	al,al
	jz	avx_extractf_mem_size_ok
	cmp	al,[mmx_size]
	jne	invalid_operand_size
      avx_extractf_mem_size_ok:
	call	take_avx512_mask
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	cmp	ah,[mmx_size]
	jbe	invalid_operand_size
	mov	[postbyte_register],al
	jmp	mmx_imm8
      avx_extractf_reg:
	lods	byte [esi]
	call	convert_avx_register
	cmp	ah,[mmx_size]
	jne	invalid_operand_size
	push	eax
	call	take_avx512_mask
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	cmp	ah,[mmx_size]
	jbe	invalid_operand_size
	mov	[postbyte_register],al
	pop	ebx
	jmp	mmx_nomem_imm8
avx512_insert_64x4_instruction:
	or	[rex_prefix],8
avx512_insert_32x8_instruction:
	or	[vex_required],8
	mov	cl,32
	jmp	avx_insertf_instruction
avx512_insert_64x2_instruction:
	or	[rex_prefix],8
avx512_insert_32x4_instruction:
	or	[vex_required],8
	mov	cl,16
	jmp	avx_insertf_instruction
avx_insertf128_instruction:
	or	[vex_required],2
	mov	cl,16
      avx_insertf_instruction:
	mov	[mmx_size],cl
	mov	[broadcast_size],0
	call	setup_66_0f_3a
	call	take_avx_register
	cmp	ah,[mmx_size]
	jbe	invalid_operand
	mov	[postbyte_register],al
	call	take_avx512_mask
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[vex_register],al
	mov	al,[mmx_size]
	xchg	al,[operand_size]
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	avx_insertf_reg_reg_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	mov	[operand_size],al
	jmp	mmx_imm8
      avx_insertf_reg_reg_reg:
	lods	byte [esi]
	call	convert_avx_register
	mov	bl,al
	pop	eax
	mov	[operand_size],al
	jmp	mmx_nomem_imm8
avx_extract_b_instruction:
	mov	cl,1
	jmp	avx_extract_instruction
avx_extract_w_instruction:
	mov	cl,2
	jmp	avx_extract_instruction
avx_extract_q_instruction:
	or	[rex_prefix],8
	mov	cl,8
	jmp	avx_extract_instruction
avx_extract_d_instruction:
	mov	cl,4
      avx_extract_instruction:
	mov	[mmx_size],cl
	call	setup_66_0f_3a
	or	[vex_required],1
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	avx_extractps_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[mmx_size]
	not	al
	and	[operand_size],al
	jnz	invalid_operand_size
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_avx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	[postbyte_register],al
	jmp	mmx_imm8
      avx_extractps_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	mov	al,[mmx_size]
	cmp	ah,al
	jb	invalid_operand_size
	cmp	ah,4
	je	avx_extractps_reg_size_ok
	cmp	ah,8
	jne	invalid_operand_size
	cmp	[code_type],64
	jne	invalid_operand
	cmp	al,4
	jae	avx_extractps_reg_size_ok
	or	[rex_prefix],8
      avx_extractps_reg_size_ok:
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_avx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	[postbyte_register],al
	cmp	[supplemental_code],15h
	jne	mmx_nomem_imm8
	mov	[extended_code],0C5h
	xchg	bl,[postbyte_register]
	jmp	mmx_nomem_imm8
avx_insertps_instruction:
	mov	[immediate_size],1
	or	[operand_flags],10h
	mov	[opcode_prefix],66h
	mov	[supplemental_code],al
	mov	al,3Ah
	mov	cl,4
	jmp	avx_instruction
avx_pinsrb_instruction:
	mov	cl,1
	jmp	avx_pinsr_instruction_3a
avx_pinsrw_instruction:
	mov	cl,2
	jmp	avx_pinsr_instruction
avx_pinsrd_instruction:
	mov	cl,4
	jmp	avx_pinsr_instruction_3a
avx_pinsrq_instruction:
	cmp	[code_type],64
	jne	illegal_instruction
	mov	cl,8
	or	[rex_prefix],8
      avx_pinsr_instruction_3a:
	mov	[supplemental_code],al
	mov	al,3Ah
      avx_pinsr_instruction:
	mov	[opcode_prefix],66h
	mov	[base_code],0Fh
	mov	[extended_code],al
	mov	[mmx_size],cl
	or	[vex_required],1
	call	take_avx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[vex_register],al
	jmp	pinsr_xmmreg

avx_cvtudq2pd_instruction:
	or	[vex_required],8
avx_cvtdq2pd_instruction:
	mov	[opcode_prefix],0F3h
	mov	cl,4
	jmp	avx_cvt_d_instruction
avx_cvtps2qq_instruction:
	or	[operand_flags],8
avx_cvttps2qq_instruction:
	or	[operand_flags],4
	or	[vex_required],8
	mov	[opcode_prefix],66h
	mov	cl,4
	jmp	avx_cvt_d_instruction
avx_cvtps2pd_instruction:
	or	[operand_flags],4
	mov	cl,4
      avx_cvt_d_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	mov	[broadcast_size],cl
	call	take_avx_register
	mov	[postbyte_register],al
	call	take_avx512_mask
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	xor	ecx,ecx
	xchg	cl,[operand_size]
	mov	al,cl
	shr	al,1
	mov	[mmx_size],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	je	avx_cvt_d_reg_mem
	cmp	al,10h
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	convert_avx_register
	cmp	ah,[mmx_size]
	je	avx_cvt_d_reg_reg_size_ok
	jb	invalid_operand_size
	cmp	ah,16
	jne	invalid_operand_size
      avx_cvt_d_reg_reg_size_ok:
	mov	bl,al
	mov	[operand_size],cl
	call	take_avx512_rounding
	jmp	nomem_instruction_ready
      avx_cvt_d_reg_mem:
	call	take_avx_mem
	jmp	instruction_ready
avx_cvtpd2dq_instruction:
	or	[operand_flags],4+8
	mov	[opcode_prefix],0F2h
	jmp	avx_cvt_q_instruction
avx_cvtuqq2ps_instruction:
	mov	[opcode_prefix],0F2h
avx_cvtpd2udq_instruction:
	or	[operand_flags],8
avx_cvttpd2udq_instruction:
	or	[operand_flags],4
	or	[vex_required],8
	jmp	avx_cvt_q_instruction
avx_cvtpd2ps_instruction:
	or	[operand_flags],8
avx_cvttpd2dq_instruction:
	or	[operand_flags],4
	mov	[opcode_prefix],66h
      avx_cvt_q_instruction:
	mov	[broadcast_size],8
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	or	[rex_prefix],80h
	call	take_avx_register
	mov	[postbyte_register],al
	push	eax
	call	take_avx512_mask
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	xor	al,al
	mov	[operand_size],al
	mov	[mmx_size],al
	call	take_avx_rm
	jnc	avx_cvt_q_reg_mem
	mov	bl,al
	pop	eax
	call	avx_cvt_q_check_size
	call	take_avx512_rounding
	jmp	nomem_instruction_ready
      avx_cvt_q_reg_mem:
	pop	eax
	call	avx_cvt_q_check_size
	jmp	instruction_ready
      avx_cvt_q_check_size:
	mov	al,[operand_size]
	or	al,al
	jz	avx_cvt_q_size_not_specified
	cmp	al,64
	ja	invalid_operand_size
	shr	al,1
	cmp	al,ah
	je	avx_cvt_q_size_ok
	ja	invalid_operand_size
	cmp	ah,16
	jne	invalid_operand_size
      avx_cvt_q_size_ok:
	ret
      avx_cvt_q_size_not_specified:
	cmp	ah,64 shr 1
	jne	recoverable_unknown_size
	mov	[operand_size],64
	ret
avx_cvttps2udq_instruction:
	or	[vex_required],8
	or	[operand_flags],2+4
	mov	cx,0400h
	jmp	avx_instruction_with_broadcast
avx_cvttps2dq_instruction:
	mov	[opcode_prefix],0F3h
	or	[operand_flags],2+4
	mov	cx,0400h
	jmp	avx_instruction_with_broadcast
avx_cvtph2ps_instruction:
	mov	[opcode_prefix],66h
	mov	[supplemental_code],al
	or	[operand_flags],4
	mov	al,38h
	xor	cl,cl
	jmp	avx_cvt_d_instruction
avx_cvtps2ph_instruction:
	call	setup_66_0f_3a
	or	[vex_required],1
	or	[operand_flags],4
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	vcvtps2ph_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	call	take_avx512_mask
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	shl	[operand_size],1
	call	take_avx_register
	mov	[postbyte_register],al
	shr	ah,1
	mov	[mmx_size],ah
	jmp	mmx_imm8
      vcvtps2ph_reg:
	lods	byte [esi]
	call	convert_avx_register
	mov	bl,al
	call	take_avx512_mask
	xor	cl,cl
	xchg	cl,[operand_size]
	shl	cl,1
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[postbyte_register],al
	or	cl,cl
	jz	vcvtps2ph_reg_size_ok
	cmp	cl,ah
	je	vcvtps2ph_reg_size_ok
	jb	invalid_operand_size
	cmp	ah,16
	jne	invalid_operand_size
      vcvtps2ph_reg_size_ok:
	call	take_avx512_rounding
	jmp	mmx_nomem_imm8

avx_cvtsd2usi_instruction:
	or	[operand_flags],8
avx_cvttsd2usi_instruction:
	or	[vex_required],8
	jmp	avx_cvttsd2si_instruction
avx_cvtsd2si_instruction:
	or	[operand_flags],8
avx_cvttsd2si_instruction:
	mov	ah,0F2h
	mov	cl,8
	jmp	avx_cvt_2si_instruction
avx_cvtss2usi_instruction:
	or	[operand_flags],8
avx_cvttss2usi_instruction:
	or	[vex_required],8
	jmp	avx_cvttss2si_instruction
avx_cvtss2si_instruction:
	or	[operand_flags],8
avx_cvttss2si_instruction:
	mov	ah,0F3h
	mov	cl,4
      avx_cvt_2si_instruction:
	or	[operand_flags],2+4
	mov	[mmx_size],cl
	mov	[broadcast_size],0
	mov	[opcode_prefix],ah
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	mov	[operand_size],0
	cmp	ah,4
	je	avx_cvt_2si_reg
	cmp	ah,8
	jne	invalid_operand_size
	call	operand_64bit
      avx_cvt_2si_reg:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_rm
	jnc	instruction_ready
	mov	bl,al
	call	take_avx512_rounding
	jmp	nomem_instruction_ready
avx_cvtusi2sd_instruction:
	or	[vex_required],8
avx_cvtsi2sd_instruction:
	mov	ah,0F2h
	mov	cl,8
	jmp	avx_cvtsi_instruction
avx_cvtusi2ss_instruction:
	or	[vex_required],8
avx_cvtsi2ss_instruction:
	mov	ah,0F3h
	mov	cl,4
      avx_cvtsi_instruction:
	or	[operand_flags],2+4+8
	mov	[mmx_size],cl
	mov	[opcode_prefix],ah
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	call	take_avx_register
	cmp	ah,16
	jne	invalid_operand_size
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[vex_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	je	avx_cvtsi_reg_reg_mem
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	cmp	ah,4
	je	avx_cvtsi_reg_reg_reg32
	cmp	ah,8
	jne	invalid_operand_size
	call	operand_64bit
      avx_cvtsi_rounding:
	call	take_avx512_rounding
	jmp	nomem_instruction_ready
      avx_cvtsi_reg_reg_reg32:
	cmp	[mmx_size],8
	jne	avx_cvtsi_rounding
	jmp	nomem_instruction_ready
      avx_cvtsi_reg_reg_mem:
	call	get_address
	mov	al,[operand_size]
	mov	[mmx_size],al
	or	al,al
	jz	single_mem_nosize
	cmp	al,4
	je	instruction_ready
	cmp	al,8
	jne	invalid_operand_size
	call	operand_64bit
	jmp	instruction_ready

avx_maskmov_w1_instruction:
	or	[rex_prefix],8
avx_maskmov_instruction:
	call	setup_66_0f_38
	mov	[mmx_size],0
	or	[vex_required],2
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	avx_maskmov_mem
	lods	byte [esi]
	call	convert_avx_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[vex_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	jmp	instruction_ready
      avx_maskmov_mem:
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[vex_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[postbyte_register],al
	add	[supplemental_code],2
	jmp	instruction_ready
avx_movmskpd_instruction:
	mov	[opcode_prefix],66h
avx_movmskps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],50h
	or	[vex_required],2
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[postbyte_register],al
	cmp	ah,4
	je	avx_movmskps_reg_ok
	cmp	ah,8
	jne	invalid_operand_size
	cmp	[code_type],64
	jne	invalid_operand
      avx_movmskps_reg_ok:
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	bl,al
	jmp	nomem_instruction_ready
avx_maskmovdqu_instruction:
	or	[vex_required],2
	jmp	maskmovdqu_instruction
avx_pmovmskb_instruction:
	or	[vex_required],2
	mov	[opcode_prefix],66h
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	je	avx_pmovmskb_reg_size_ok
	cmp	[code_type],64
	jne	invalid_operand_size
	cmp	ah,8
	jnz	invalid_operand_size
      avx_pmovmskb_reg_size_ok:
	mov	[postbyte_register],al
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	bl,al
	jmp	nomem_instruction_ready

gather_pd_instruction:
	or	[rex_prefix],8
gather_ps_instruction:
	call	setup_66_0f_38
	or	[vex_required],4
	or	[operand_flags],20h
	call	take_avx_register
	mov	[postbyte_register],al
	call	take_avx512_mask
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	xor	cl,cl
	xchg	cl,[operand_size]
	push	ecx
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_argument
	call	get_address
	pop	eax
	xchg	al,[operand_size]
      gather_mem_size_check:
	mov	ah,4
	test	[rex_prefix],8
	jz	gather_elements_size_ok
	add	ah,ah
      gather_elements_size_ok:
	mov	[mmx_size],ah
	test	al,al
	jz	gather_mem_size_ok
	cmp	al,ah
	jne	invalid_operand_size
      gather_mem_size_ok:
	cmp	byte [esi],','
	je	gather_reg_mem_reg
	test	[vex_required],20h
	jz	invalid_operand
	mov	ah,[operand_size]
	mov	al,80h
	jmp	gather_arguments_ok
      gather_reg_mem_reg:
	or	[vex_required],2
	inc	esi
	call	take_avx_register
      gather_arguments_ok:
	mov	[vex_register],al
	cmp	al,[postbyte_register]
	je	disallowed_combination_of_registers
	mov	al,bl
	and	al,11111b
	cmp	al,[postbyte_register]
	je	disallowed_combination_of_registers
	cmp	al,[vex_register]
	je	disallowed_combination_of_registers
	mov	al,bl
	shr	al,5
	cmp	al,0Ch shr 1
	je	gather_vr128
	mov	ah,32
	cmp	al,6 shr 1
	jne	gather_regular
	add	ah,ah
      gather_regular:
	mov	al,[rex_prefix]
	shr	al,3
	xor	al,[supplemental_code]
	test	al,1
	jz	gather_uniform
	test	[supplemental_code],1
	jz	gather_double
	mov	al,ah
	xchg	al,[operand_size]
	add	al,al
	cmp	al,ah
	jne	invalid_operand_size
	jmp	instruction_ready
      gather_double:
	add	ah,ah
      gather_uniform:
	cmp	ah,[operand_size]
	jne	invalid_operand_size
	jmp	instruction_ready
      gather_vr128:
	cmp	ah,16
	je	instruction_ready
	cmp	ah,32
	jne	invalid_operand_size
	test	[supplemental_code],1
	jnz	invalid_operand_size
	test	[rex_prefix],8
	jz	invalid_operand_size
	jmp	instruction_ready
scatter_pd_instruction:
	or	[rex_prefix],8
scatter_ps_instruction:
	call	setup_66_0f_38
	or	[vex_required],4+8
	or	[operand_flags],20h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_argument
	call	get_address
	call	take_avx512_mask
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	xor	al,al
	xchg	al,[operand_size]
	push	eax
	call	take_avx_register
	mov	[postbyte_register],al
	pop	eax
	jmp	gather_mem_size_check
gatherpf_qpd_instruction:
	mov	ah,0C7h
	jmp	gatherpf_pd_instruction
gatherpf_dpd_instruction:
	mov	ah,0C6h
      gatherpf_pd_instruction:
	or	[rex_prefix],8
	mov	cl,8
	jmp	gatherpf_instruction
gatherpf_qps_instruction:
	mov	ah,0C7h
	jmp	gatherpf_ps_instruction
gatherpf_dps_instruction:
	mov	ah,0C6h
      gatherpf_ps_instruction:
	mov	cl,4
      gatherpf_instruction:
	mov	[mmx_size],cl
	mov	[postbyte_register],al
	mov	al,ah
	call	setup_66_0f_38
	or	[vex_required],4+8
	or	[operand_flags],20h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	jne	invalid_argument
	call	get_address
	call	take_avx512_mask
	mov	ah,[mmx_size]
	mov	al,[operand_size]
	or	al,al
	jz	gatherpf_mem_size_ok
	cmp	al,ah
	jne	invalid_operand_size
      gatherpf_mem_size_ok:
	mov	[operand_size],64
	mov	al,6 shr 1
	cmp	ah,4
	je	gatherpf_check_vsib
	cmp	[supplemental_code],0C6h
	jne	gatherpf_check_vsib
	mov	al,0Eh shr 1
      gatherpf_check_vsib:
	mov	ah,bl
	shr	ah,5
	cmp	al,ah
	jne	invalid_operand
	jmp	instruction_ready

bmi_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],38h
	mov	[supplemental_code],0F3h
	mov	[postbyte_register],al
      bmi_reg:
	or	[vex_required],2
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	[vex_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	bmi_reg_reg
	cmp	al,'['
	jne	invalid_argument
	call	get_address
	call	operand_32or64
	jmp	instruction_ready
      bmi_reg_reg:
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	call	operand_32or64
	jmp	nomem_instruction_ready
      operand_32or64:
	mov	al,[operand_size]
	cmp	al,4
	je	operand_32or64_ok
	cmp	al,8
	jne	invalid_operand_size
	cmp	[code_type],64
	jne	invalid_operand
	or	[rex_prefix],8
      operand_32or64_ok:
	ret
pdep_instruction:
	mov	[opcode_prefix],0F2h
	jmp	andn_instruction
pext_instruction:
	mov	[opcode_prefix],0F3h
andn_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],38h
	mov	[supplemental_code],al
	or	[vex_required],2
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
	jmp	bmi_reg
sarx_instruction:
	mov	[opcode_prefix],0F3h
	jmp	bzhi_instruction
shrx_instruction:
	mov	[opcode_prefix],0F2h
	jmp	bzhi_instruction
shlx_instruction:
	mov	[opcode_prefix],66h
bzhi_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],38h
	mov	[supplemental_code],al
	or	[vex_required],2
	call	get_reg_mem
	jc	bzhi_reg_reg
	call	get_vex_source_register
	jc	invalid_operand
	call	operand_32or64
	jmp	instruction_ready
      bzhi_reg_reg:
	call	get_vex_source_register
	jc	invalid_operand
	call	operand_32or64
	jmp	nomem_instruction_ready
      get_vex_source_register:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	no_vex_source_register
	lods	byte [esi]
	call	convert_register
	mov	[vex_register],al
	clc
	ret
      no_vex_source_register:
	stc
	ret
bextr_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],38h
	mov	[supplemental_code],al
	or	[vex_required],2
	call	get_reg_mem
	jc	bextr_reg_reg
	call	get_vex_source_register
	jc	bextr_reg_mem_imm32
	call	operand_32or64
	jmp	instruction_ready
      bextr_reg_reg:
	call	get_vex_source_register
	jc	bextr_reg_reg_imm32
	call	operand_32or64
	jmp	nomem_instruction_ready
      setup_bextr_imm_opcode:
	mov	[xop_opcode_map],0Ah
	mov	[base_code],10h
	call	operand_32or64
	ret
      bextr_reg_mem_imm32:
	call	get_imm32
	call	setup_bextr_imm_opcode
	jmp	store_instruction_with_imm32
      bextr_reg_reg_imm32:
	call	get_imm32
	call	setup_bextr_imm_opcode
      store_nomem_instruction_with_imm32:
	call	store_nomem_instruction
	mov	eax,dword [value]
	call	mark_relocation
	stos	dword [edi]
	jmp	instruction_assembled
      get_imm32:
	cmp	al,'('
	jne	invalid_operand
	push	edx ebx ecx
	call	get_dword_value
	mov	dword [value],eax
	pop	ecx ebx edx
	ret
rorx_instruction:
	mov	[opcode_prefix],0F2h
	mov	[base_code],0Fh
	mov	[extended_code],3Ah
	mov	[supplemental_code],al
	or	[vex_required],2
	call	get_reg_mem
	jc	rorx_reg_reg
	call	operand_32or64
	jmp	mmx_imm8
      rorx_reg_reg:
	call	operand_32or64
	jmp	mmx_nomem_imm8

tbm_instruction:
	mov	[xop_opcode_map],9
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[base_code],ah
	mov	[postbyte_register],al
	jmp	bmi_reg

llwpcb_instruction:
	or	[vex_required],2
	mov	[xop_opcode_map],9
	mov	[base_code],12h
	mov	[postbyte_register],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_register
	mov	bl,al
	call	operand_32or64
	jmp	nomem_instruction_ready
lwpins_instruction:
	or	[vex_required],2
	mov	[xop_opcode_map],0Ah
	mov	[base_code],12h
	mov	[vex_register],al
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
	xor	cl,cl
	xchg	cl,[operand_size]
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	lwpins_reg_reg
	cmp	al,'['
	jne	invalid_argument
	push	ecx
	call	get_address
	pop	eax
	xchg	al,[operand_size]
	test	al,al
	jz	lwpins_reg_mem_size_ok
	cmp	al,4
	jne	invalid_operand_size
      lwpins_reg_mem_size_ok:
	call	prepare_lwpins
	jmp	store_instruction_with_imm32
      lwpins_reg_reg:
	lods	byte [esi]
	call	convert_register
	cmp	ah,4
	jne	invalid_operand_size
	mov	[operand_size],cl
	mov	bl,al
	call	prepare_lwpins
	jmp	store_nomem_instruction_with_imm32
      prepare_lwpins:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_imm32
	call	operand_32or64
	mov	al,[vex_register]
	xchg	al,[postbyte_register]
	mov	[vex_register],al
	ret

xop_single_source_sd_instruction:
	or	[operand_flags],2
	mov	[mmx_size],8
	jmp	xop_instruction_9
xop_single_source_ss_instruction:
	or	[operand_flags],2
	mov	[mmx_size],4
	jmp	xop_instruction_9
xop_single_source_instruction:
	or	[operand_flags],2
	mov	[mmx_size],0
      xop_instruction_9:
	mov	[base_code],al
	mov	[xop_opcode_map],9
	jmp	avx_xop_common
xop_single_source_128bit_instruction:
	or	[operand_flags],2
	mov	[mmx_size],16
	jmp	xop_instruction_9
xop_triple_source_128bit_instruction:
	mov	[immediate_size],-1
	mov	byte [value],0
	mov	[mmx_size],16
	jmp	xop_instruction_8
xop_128bit_instruction:
	mov	[immediate_size],-2
	mov	byte [value],0
	mov	[mmx_size],16
      xop_instruction_8:
	mov	[base_code],al
	mov	[xop_opcode_map],8
	jmp	avx_xop_common
xop_pcom_b_instruction:
	mov	ah,0CCh
	jmp	xop_pcom_instruction
xop_pcom_d_instruction:
	mov	ah,0CEh
	jmp	xop_pcom_instruction
xop_pcom_q_instruction:
	mov	ah,0CFh
	jmp	xop_pcom_instruction
xop_pcom_w_instruction:
	mov	ah,0CDh
	jmp	xop_pcom_instruction
xop_pcom_ub_instruction:
	mov	ah,0ECh
	jmp	xop_pcom_instruction
xop_pcom_ud_instruction:
	mov	ah,0EEh
	jmp	xop_pcom_instruction
xop_pcom_uq_instruction:
	mov	ah,0EFh
	jmp	xop_pcom_instruction
xop_pcom_uw_instruction:
	mov	ah,0EDh
      xop_pcom_instruction:
	mov	byte [value],al
	mov	[immediate_size],-4
	mov	[mmx_size],16
	mov	[base_code],ah
	mov	[xop_opcode_map],8
	jmp	avx_xop_common
vpcmov_instruction:
	or	[vex_required],2
	mov	[immediate_size],-2
	mov	byte [value],0
	mov	[mmx_size],0
	mov	[base_code],al
	mov	[xop_opcode_map],8
	jmp	avx_xop_common
xop_shift_instruction:
	mov	[base_code],al
	or	[vex_required],2
	mov	[xop_opcode_map],9
	call	take_avx_register
	cmp	ah,16
	jne	invalid_operand
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	je	xop_shift_reg_mem
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_xmm_register
	mov	[vex_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	push	esi
	xor	cl,cl
	xchg	cl,[operand_size]
	lods	byte [esi]
	call	get_size_operator
	pop	esi
	xchg	cl,[operand_size]
	cmp	al,'['
	je	xop_shift_reg_reg_mem
	cmp	al,10h
	jne	xop_shift_reg_reg_imm
	call	take_avx_register
	mov	bl,al
	xchg	bl,[vex_register]
	jmp	nomem_instruction_ready
      xop_shift_reg_reg_mem:
	or	[rex_prefix],8
	lods	byte [esi]
	call	get_size_operator
	call	get_address
	jmp	instruction_ready
      xop_shift_reg_reg_imm:
	xor	bl,bl
	xchg	bl,[vex_register]
	cmp	[base_code],94h
	jae	invalid_operand
	add	[base_code],30h
	mov	[xop_opcode_map],8
	dec	esi
	jmp	mmx_nomem_imm8
      xop_shift_reg_mem:
	call	get_address
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	push	esi
	xor	cl,cl
	xchg	cl,[operand_size]
	lods	byte [esi]
	call	get_size_operator
	pop	esi
	xchg	cl,[operand_size]
	cmp	al,10h
	jne	xop_shift_reg_mem_imm
	call	take_avx_register
	mov	[vex_register],al
	jmp	instruction_ready
      xop_shift_reg_mem_imm:
	cmp	[base_code],94h
	jae	invalid_operand
	add	[base_code],30h
	mov	[xop_opcode_map],8
	dec	esi
	jmp	mmx_imm8

set_evex_mode:
	mov	[evex_mode],al
	jmp	instruction_assembled

take_avx_register:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
convert_avx_register:
	mov	ah,al
	and	al,1Fh
	and	ah,0E0h
	sub	ah,60h
	jb	invalid_operand
	jz	avx512_register_size
	sub	ah,60h
	jb	invalid_operand
	jnz	avx_register_size_ok
	mov	ah,16
	jmp	avx_register_size_ok
      avx512_register_size:
	mov	ah,64
      avx_register_size_ok:
	cmp	al,8
	jb	match_register_size
	cmp	[code_type],64
	jne	invalid_operand
	jmp	match_register_size
store_vex_instruction_code:
	test	[rex_prefix],10h
	jnz	invalid_operand
	test	[vex_required],0F8h
	jnz	store_evex_instruction_code
	test	[vex_register],10000b
	jnz	store_evex_instruction_code
	cmp	[operand_size],64
	je	store_evex_instruction_code
	mov	al,[base_code]
	cmp	al,0Fh
	jne	store_xop_instruction_code
	test	[vex_required],2
	jnz	prepare_vex
	cmp	[evex_mode],0
	je	prepare_vex
	cmp	[displacement_compression],1
	jne	prepare_vex
	cmp	edx,80h
	jb	prepare_vex
	cmp	edx,-80h
	jae	prepare_vex
	mov	al,bl
	or	al,bh
	shr	al,4
	cmp	al,2
	je	prepare_vex
	call	compress_displacement
	cmp	[displacement_compression],2
	ja	prepare_evex
	jb	prepare_vex
	dec	[displacement_compression]
	mov	edx,[uncompressed_displacement]
      prepare_vex:
	mov	ah,[extended_code]
	cmp	ah,38h
	je	store_vex_0f38_instruction_code
	cmp	ah,3Ah
	je	store_vex_0f3a_instruction_code
	test	[rex_prefix],1011b
	jnz	store_vex_0f_instruction_code
	mov	[edi+2],ah
	mov	byte [edi],0C5h
	mov	al,[vex_register]
	not	al
	shl	al,3
	mov	ah,[rex_prefix]
	shl	ah,5
	and	ah,80h
	xor	al,ah
	call	get_vex_lpp_bits
	mov	[edi+1],al
	call	check_vex
	add	edi,3
	ret
      get_vex_lpp_bits:
	cmp	[operand_size],32
	jne	get_vex_pp_bits
	or	al,100b
      get_vex_pp_bits:
	mov	ah,[opcode_prefix]
	cmp	ah,66h
	je	vex_66
	cmp	ah,0F3h
	je	vex_f3
	cmp	ah,0F2h
	je	vex_f2
	test	ah,ah
	jnz	disallowed_combination_of_registers
	ret
      vex_f2:
	or	al,11b
	ret
      vex_f3:
	or	al,10b
	ret
      vex_66:
	or	al,1
	ret
      store_vex_0f38_instruction_code:
	mov	al,11100010b
	mov	ah,[supplemental_code]
	jmp	make_c4_vex
      store_vex_0f3a_instruction_code:
	mov	al,11100011b
	mov	ah,[supplemental_code]
	jmp	make_c4_vex
      store_vex_0f_instruction_code:
	mov	al,11100001b
      make_c4_vex:
	mov	[edi+3],ah
	mov	byte [edi],0C4h
	mov	ah,[rex_prefix]
	shl	ah,5
	xor	al,ah
	mov	[edi+1],al
	call	check_vex
	mov	al,[vex_register]
	xor	al,1111b
	shl	al,3
	mov	ah,[rex_prefix]
	shl	ah,4
	and	ah,80h
	or	al,ah
	call	get_vex_lpp_bits
	mov	[edi+2],al
	add	edi,4
	ret
      check_vex:
	cmp	[code_type],64
	je	vex_ok
	not	al
	test	al,11000000b
	jnz	invalid_operand
	test	[rex_prefix],40h
	jnz	invalid_operand
      vex_ok:
	ret
store_xop_instruction_code:
	mov	[edi+3],al
	mov	byte [edi],8Fh
	mov	al,[xop_opcode_map]
	mov	ah,[rex_prefix]
	test	ah,40h
	jz	xop_ok
	cmp	[code_type],64
	jne	invalid_operand
      xop_ok:
	not	ah
	shl	ah,5
	xor	al,ah
	mov	[edi+1],al
	mov	al,[vex_register]
	xor	al,1111b
	shl	al,3
	mov	ah,[rex_prefix]
	shl	ah,4
	and	ah,80h
	or	al,ah
	call	get_vex_lpp_bits
	mov	[edi+2],al
	add	edi,4
	ret
store_evex_instruction_code:
	test	[vex_required],2
	jnz	invalid_operand
	cmp	[base_code],0Fh
	jne	invalid_operand
	cmp	[displacement_compression],1
	jne	prepare_evex
	call	compress_displacement
      prepare_evex:
	mov	ah,[extended_code]
	cmp	ah,38h
	je	store_evex_0f38_instruction_code
	cmp	ah,3Ah
	je	store_evex_0f3a_instruction_code
	mov	al,11110001b
      make_evex:
	mov	[edi+4],ah
	mov	byte [edi],62h
	mov	ah,[rex_prefix]
	shl	ah,5
	xor	al,ah
	mov	ah,[vex_required]
	and	ah,10h
	xor	al,ah
	mov	[edi+1],al
	call	check_vex
	mov	al,[vex_register]
	not	al
	and	al,1111b
	shl	al,3
	mov	ah,[rex_prefix]
	shl	ah,4
	or	ah,[rex_prefix]
	and	ah,80h
	or	al,ah
	or	al,100b
	call	get_vex_pp_bits
	mov	[edi+2],al
	mov	al,[vex_register]
	not	al
	shr	al,1
	and	al,1000b
	test	[vex_required],80h
	jne	evex_rounding
	mov	ah,[operand_size]
	cmp	ah,16
	jbe	evex_l_ok
	or	al,ah
	jmp	evex_l_ok
      evex_rounding:
	mov	ah,[rounding_mode]
	shl	ah,5
	or	al,ah
      evex_l_ok:
	test	[vex_required],20h
	jz	evex_zaaa_ok
	or	al,[mask_register]
      evex_zaaa_ok:
	test	[vex_required],40h
	jz	evex_b_ok
	or	al,10h
      evex_b_ok:
	mov	[edi+3],al
	add	edi,5
	ret
      store_evex_0f38_instruction_code:
	mov	al,11110010b
	mov	ah,[supplemental_code]
	jmp	make_evex
      store_evex_0f3a_instruction_code:
	mov	al,11110011b
	mov	ah,[supplemental_code]
	jmp	make_evex
compress_displacement:
	mov	ebp,ecx
	mov	[uncompressed_displacement],edx
	or	edx,edx
	jz	displacement_compressed
	xor	ecx,ecx
	mov	cl,[mmx_size]
	test	cl,cl
	jnz	calculate_displacement_scale
	mov	cl,[operand_size]
      calculate_displacement_scale:
	bsf	ecx,ecx
	jz	displacement_compression_ok
	xor	eax,eax
	shrd	eax,edx,cl
	jnz	displacement_not_compressed
	sar	edx,cl
	cmp	edx,80h
	jb	displacement_compressed
	cmp	edx,-80h
	jnb	displacement_compressed
	shl	edx,cl
      displacement_not_compressed:
	inc	[displacement_compression]
	jmp	displacement_compression_ok
      displacement_compressed:
	add	[displacement_compression],2
      displacement_compression_ok:
	mov	ecx,ebp
	ret
