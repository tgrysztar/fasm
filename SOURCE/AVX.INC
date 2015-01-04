
; flat assembler core
; Copyright (c) 1999-2015, Tomasz Grysztar.
; All rights reserved.

avx_single_source_pd_instruction:
	or	[vex_required],2
	jmp	avx_pd_instruction
avx_pd_instruction_imm8:
	mov	[immediate_size],1
avx_pd_instruction:
	mov	[opcode_prefix],66h
	mov	[mmx_size],0
	jmp	avx_instruction
avx_single_source_ps_instruction:
	or	[vex_required],2
	jmp	avx_ps_instruction
avx_ps_instruction_imm8:
	mov	[immediate_size],1
avx_ps_instruction:
	mov	[mmx_size],0
	jmp	avx_instruction
avx_sd_instruction_imm8:
	mov	[immediate_size],1
avx_sd_instruction:
	mov	[opcode_prefix],0F2h
	mov	[mmx_size],8
	jmp	avx_instruction
avx_ss_instruction_imm8:
	mov	[immediate_size],1
avx_ss_instruction:
	mov	[opcode_prefix],0F3h
	mov	[mmx_size],4
	jmp	avx_instruction
avx_cmp_pd_instruction:
	mov	[opcode_prefix],66h
avx_cmp_ps_instruction:
	mov	[mmx_size],0
	mov	byte [value],al
	mov	al,0C2h
	jmp	avx_instruction
avx_cmp_sd_instruction:
	mov	[opcode_prefix],0F2h
	mov	[mmx_size],8
	mov	byte [value],al
	mov	al,0C2h
	jmp	avx_instruction
avx_cmp_ss_instruction:
	mov	[opcode_prefix],0F3h
	mov	[mmx_size],4
	mov	byte [value],al
	mov	al,0C2h
	jmp	avx_instruction
avx_comiss_instruction:
	or	[vex_required],2
	mov	[mmx_size],4
	jmp	avx_instruction
avx_comisd_instruction:
	or	[vex_required],2
	mov	[opcode_prefix],66h
	mov	[mmx_size],8
	jmp	avx_instruction
avx_haddps_instruction:
	mov	[opcode_prefix],0F2h
	mov	[mmx_size],0
	jmp	avx_instruction
avx_movshdup_instruction:
	or	[vex_required],2
	mov	[opcode_prefix],0F3h
	mov	[mmx_size],0
	jmp	avx_instruction
avx_128bit_instruction:
	mov	[mmx_size],16
	mov	[opcode_prefix],66h
avx_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
      avx_common:
	or	[vex_required],1
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
      avx_reg:
	lods	byte [esi]
	call	convert_avx_register
	mov	[postbyte_register],al
      avx_vex_reg:
	test	[vex_required],2
	jnz	avx_vex_reg_ok
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[vex_register],al
      avx_vex_reg_ok:
	cmp	[mmx_size],0
	je	avx_regs_size_ok
	cmp	ah,16
	jne	invalid_operand
      avx_regs_size_ok:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_rm
	jc	avx_regs_reg
	mov	al,[extended_code]
	mov	ah,[supplemental_code]
	cmp	al,0C2h
	je	sse_cmp_mem_ok
	cmp	ax,443Ah
	je	sse_cmp_mem_ok
	mov	al,[base_code]
	and	al,11011100b
	cmp	al,11001100b
	je	sse_cmp_mem_ok
	cmp	[immediate_size],1
	je	mmx_imm8
	cmp	[immediate_size],0
	jge	instruction_ready
	cmp	byte [esi],','
	jne	invalid_operand
	inc	esi
	call	take_avx_register
	shl	al,4
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
	mov	al,[extended_code]
	mov	ah,[supplemental_code]
	cmp	al,0C2h
	je	sse_cmp_nomem_ok
	cmp	ax,443Ah
	je	sse_cmp_nomem_ok
	mov	al,[base_code]
	and	al,11011100b
	cmp	al,11001100b
	je	sse_cmp_nomem_ok
	cmp	[immediate_size],1
	je	mmx_nomem_imm8
	cmp	[immediate_size],0
	jge	nomem_instruction_ready
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	al,bl
	shl	al,4
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
	mov	[operand_size],cl
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_avx_register
	cmp	[mmx_size],0
	je	avx_reg_ok
	cmp	ah,16
	jne	invalid_operand
      avx_reg_ok:
	stc
	ret
      take_avx_mem:
	push	ecx
	call	get_address
	pop	eax
	cmp	[mmx_size],0
	jne	avx_smem
	xchg	al,[operand_size]
	or	al,al
	jz	avx_mem_ok
	cmp	al,[operand_size]
	jne	operand_sizes_do_not_match
      avx_mem_ok:
	clc
	ret
      avx_smem:
	xchg	al,[operand_size]
	or	al,al
	jz	avx_smem_ok
	cmp	al,[mmx_size]
	jne	invalid_operand_size
      avx_smem_ok:
	clc
	ret
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

avx_single_source_128bit_instruction_38:
	or	[vex_required],2
avx_128bit_instruction_38:
	mov	[mmx_size],16
	jmp	avx_instruction_38_setup
avx_single_source_instruction_38:
	or	[vex_required],2
avx_instruction_38:
	mov	[mmx_size],0
      avx_instruction_38_setup:
	mov	[opcode_prefix],66h
	mov	[supplemental_code],al
	mov	al,38h
	jmp	avx_instruction
avx_instruction_38_w1:
	or	[rex_prefix],8
	jmp	avx_instruction_38

avx_ss_instruction_3a_imm8:
	mov	[mmx_size],4
	jmp	avx_instruction_3a_imm8_setup
avx_sd_instruction_3a_imm8:
	mov	[mmx_size],8
	jmp	avx_instruction_3a_imm8_setup
avx_single_source_128bit_instruction_3a_imm8:
	or	[vex_required],2
avx_128bit_instruction_3a_imm8:
	mov	[mmx_size],16
	jmp	avx_instruction_3a_imm8_setup
avx_triple_source_instruction_3a:
	mov	[mmx_size],0
	mov	[immediate_size],-1
	mov	byte [value],0
	jmp	avx_instruction_3a_setup
avx_single_source_instruction_3a_imm8:
	or	[vex_required],2
avx_instruction_3a_imm8:
	mov	[mmx_size],0
      avx_instruction_3a_imm8_setup:
	mov	[immediate_size],1
      avx_instruction_3a_setup:
	mov	[opcode_prefix],66h
	mov	[supplemental_code],al
	mov	al,3Ah
	jmp	avx_instruction
avx_pclmulqdq_instruction:
	mov	byte [value],al
	mov	[mmx_size],16
	mov	al,44h
	jmp	avx_instruction_3a_setup

avx_permq_instruction:
	or	[vex_required],2
	or	[rex_prefix],8
avx_perm2f128_instruction:
	mov	[immediate_size],1
	mov	ah,3Ah
	jmp	avx_perm_instruction
avx_permd_instruction:
	mov	ah,38h
      avx_perm_instruction:
	mov	[opcode_prefix],66h
	mov	[base_code],0Fh
	mov	[extended_code],ah
	mov	[supplemental_code],al
	mov	[mmx_size],0
	or	[vex_required],1
	call	take_avx_register
	cmp	ah,32
	jne	invalid_operand_size
	mov	[postbyte_register],al
	jmp	avx_vex_reg

avx_movdqu_instruction:
	mov	[opcode_prefix],0F3h
	jmp	avx_movps_instruction
avx_movpd_instruction:
	mov	[opcode_prefix],66h
avx_movps_instruction:
	mov	[mmx_size],0
	or	[vex_required],2
	mov	[base_code],0Fh
	mov	[extended_code],al
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
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[postbyte_register],al
	jmp	instruction_ready
avx_movntpd_instruction:
	mov	[opcode_prefix],66h
avx_movntps_instruction:
	or	[vex_required],1
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	jmp	avx_mem
avx_lddqu_instruction:
	mov	[opcode_prefix],0F2h
	mov	[mmx_size],0
	xor	cx,cx
      avx_load_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	call	take_avx_register
	or	cl,cl
	jz	avx_load_reg_ok
	cmp	ah,cl
	jne	invalid_operand
      avx_load_reg_ok:
	cmp	[mmx_size],0
	je	avx_load_reg_
	xor	ah,ah
      avx_load_reg_:
	xchg	ah,[operand_size]
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	avx_load_reg_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	xchg	ah,[operand_size]
	mov	[postbyte_register],al
	mov	al,[mmx_size]
	or	al,al
	jz	instruction_ready
	or	ah,ah
	jz	instruction_ready
	cmp	al,ah
	jne	invalid_operand_size
	jmp	instruction_ready
      avx_load_reg_reg:
	lods	byte [esi]
	call	convert_avx_register
	cmp	ch,ah
	jne	invalid_operand
	mov	bl,al
	pop	eax
	xchg	ah,[operand_size]
	mov	[postbyte_register],al
	jmp	nomem_instruction_ready

avx_movntdqa_instruction:
	mov	[mmx_size],0
	xor	cx,cx
	jmp	avx_load_instruction_38
avx_broadcastss_instruction:
	mov	[mmx_size],4
	xor	cl,cl
	mov	ch,16
	jmp	avx_load_instruction_38
avx_broadcastsd_instruction:
	mov	[mmx_size],8
	mov	cl,32
	mov	ch,16
	jmp	avx_load_instruction_38
avx_pbroadcastb_instruction:
	mov	[mmx_size],1
	jmp	avx_pbroadcast_instruction
avx_pbroadcastw_instruction:
	mov	[mmx_size],2
	jmp	avx_pbroadcast_instruction
avx_pbroadcastd_instruction:
	mov	[mmx_size],4
	jmp	avx_pbroadcast_instruction
avx_pbroadcastq_instruction:
	mov	[mmx_size],8
      avx_pbroadcast_instruction:
	xor	cl,cl
	mov	ch,16
	jmp	avx_load_instruction_38
avx_broadcastf128_instruction:
	mov	[mmx_size],16
	mov	cl,32
	xor	ch,ch
      avx_load_instruction_38:
	mov	[opcode_prefix],66h
	mov	[supplemental_code],al
	mov	al,38h
	jmp	avx_load_instruction
avx_movlpd_instruction:
	mov	[opcode_prefix],66h
avx_movlps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	mov	[mmx_size],8
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
avx_maskmov_w1_instruction:
	or	[rex_prefix],8
avx_maskmov_instruction:
	call	setup_66_0f_38
	mov	[mmx_size],0
	or	[vex_required],1
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
	call	take_avx_rm
	jc	invalid_operand
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
      setup_66_0f_38:
	mov	[extended_code],38h
	mov	[supplemental_code],al
	mov	[base_code],0Fh
	mov	[opcode_prefix],66h
	ret
avx_movd_instruction:
	or	[vex_required],1
	jmp	movd_instruction
avx_movq_instruction:
	or	[vex_required],1
	jmp	movq_instruction
avx_movddup_instruction:
	or	[vex_required],1
	mov	[opcode_prefix],0F2h
	mov	[base_code],0Fh
	mov	[extended_code],al
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
	call	convert_avx_register
	mov	[postbyte_register],al
	mov	[mmx_size],0
	cmp	ah,32
	je	avx_regs_size_ok
	mov	[mmx_size],8
	jmp	avx_regs_size_ok
avx_movmskpd_instruction:
	mov	[opcode_prefix],66h
avx_movmskps_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],50h
	or	[vex_required],1
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
avx_movsd_instruction:
	mov	[opcode_prefix],0F2h
	mov	[mmx_size],8
	jmp	avx_movs_instruction
avx_movss_instruction:
	mov	[opcode_prefix],0F3h
	mov	[mmx_size],4
      avx_movs_instruction:
	or	[vex_required],1
	mov	[base_code],0Fh
	mov	[extended_code],10h
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	avx_movlps_mem
	lods	byte [esi]
	call	convert_xmm_register
	mov	[postbyte_register],al
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

avx_cvtdq2pd_instruction:
	mov	[opcode_prefix],0F3h
avx_cvtps2pd_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	call	take_avx_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	xor	cl,cl
	xchg	cl,[operand_size]
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	avx_cvtdq2pd_reg_mem
	lods	byte [esi]
	call	convert_xmm_register
	mov	bl,al
	mov	[operand_size],cl
	jmp	nomem_instruction_ready
      avx_cvtdq2pd_reg_mem:
	cmp	al,'['
	jne	invalid_operand
	mov	[mmx_size],cl
	call	get_address
	mov	al,[mmx_size]
	mov	ah,al
	xchg	al,[operand_size]
	or	al,al
	jz	instruction_ready
	shl	al,1
	cmp	al,ah
	jne	invalid_operand_size
	jmp	instruction_ready
avx_cvtpd2dq_instruction:
	mov	[opcode_prefix],0F2h
	jmp	avx_cvtpd_instruction
avx_cvtpd2ps_instruction:
	mov	[opcode_prefix],66h
      avx_cvtpd_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	call	take_avx_register
	mov	[postbyte_register],al
	cmp	ah,16
	jne	invalid_operand
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	mov	[operand_size],0
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	avx_cvtpd2dq_reg_mem
	lods	byte [esi]
	call	convert_avx_register
	mov	bl,al
	jmp	nomem_instruction_ready
      avx_cvtpd2dq_reg_mem:
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	mov	al,[operand_size]
	or	al,al
	jz	operand_size_not_specified
	cmp	al,16
	je	instruction_ready
	cmp	al,32
	jne	invalid_operand_size
	jmp	instruction_ready
avx_cvttps2dq_instruction:
	or	[vex_required],2
	mov	[opcode_prefix],0F3h
	mov	[mmx_size],0
	jmp	avx_instruction
avx_cvtsd2si_instruction:
	or	[vex_required],1
	jmp	cvtsd2si_instruction
avx_cvtss2si_instruction:
	or	[vex_required],1
	jmp	cvtss2si_instruction
avx_cvtsi2ss_instruction:
	mov	[opcode_prefix],0F3h
	jmp	avx_cvtsi_instruction
avx_cvtsi2sd_instruction:
	mov	[opcode_prefix],0F2h
      avx_cvtsi_instruction:
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
	jmp	cvtsi_xmmreg

avx_extractf128_instruction:
	or	[vex_required],1
	call	setup_66_0f_3a
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	avx_extractf128_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	xor	al,al
	xchg	al,[operand_size]
	or	al,al
	jz	avx_extractf128_mem_size_ok
	cmp	al,16
	jne	invalid_operand_size
      avx_extractf128_mem_size_ok:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	cmp	ah,32
	jne	invalid_operand_size
	mov	[postbyte_register],al
	jmp	mmx_imm8
      avx_extractf128_reg:
	lods	byte [esi]
	call	convert_xmm_register
	mov	[operand_size],0
	push	eax
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	cmp	ah,32
	jne	invalid_operand_size
	mov	[postbyte_register],al
	pop	ebx
	jmp	mmx_nomem_imm8
      setup_66_0f_3a:
	mov	[extended_code],3Ah
	mov	[supplemental_code],al
	mov	[base_code],0Fh
	mov	[opcode_prefix],66h
	ret
avx_insertf128_instruction:
	or	[vex_required],1
	call	setup_66_0f_3a
	call	take_avx_register
	cmp	ah,32
	jne	invalid_operand
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[vex_register],al
	mov	[operand_size],0
	mov	[mmx_size],16
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_rm
	mov	[operand_size],32
	jnc	mmx_imm8
	mov	bl,al
	jmp	mmx_nomem_imm8
avx_extractps_instruction:
	or	[vex_required],1
	jmp	extractps_instruction
avx_insertps_instruction:
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
	jmp	insertps_xmmreg
avx_pextrb_instruction:
	or	[vex_required],1
	jmp	pextrb_instruction
avx_pextrw_instruction:
	or	[vex_required],1
	jmp	pextrw_instruction
avx_pextrd_instruction:
	or	[vex_required],1
	jmp	pextrd_instruction
avx_pextrq_instruction:
	or	[vex_required],1
	jmp	pextrq_instruction
avx_pinsrb_instruction:
	mov	[mmx_size],1
	or	[vex_required],1
	jmp	avx_pinsr_instruction_3a
avx_pinsrw_instruction:
	mov	[mmx_size],2
	or	[vex_required],1
	jmp	avx_pinsr_instruction
avx_pinsrd_instruction:
	mov	[mmx_size],4
	or	[vex_required],1
	jmp	avx_pinsr_instruction_3a
avx_pinsrq_instruction:
	mov	[mmx_size],8
	or	[vex_required],1
	call	operand_64bit
      avx_pinsr_instruction_3a:
	mov	[supplemental_code],al
	mov	al,3Ah
      avx_pinsr_instruction:
	mov	[opcode_prefix],66h
	mov	[base_code],0Fh
	mov	[extended_code],al
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
avx_maskmovdqu_instruction:
	or	[vex_required],1
	jmp	maskmovdqu_instruction
avx_pmovmskb_instruction:
	or	[vex_required],1
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
avx_pshufd_instruction:
	or	[vex_required],1
	mov	[mmx_size],0
	mov	[opcode_prefix],al
	mov	[base_code],0Fh
	mov	[extended_code],70h
	call	take_avx_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_rm
	jnc	mmx_imm8
	mov	bl,al
	jmp	mmx_nomem_imm8

avx_pmovsxbw_instruction:
	mov	[mmx_size],8
	jmp	avx_pmovsx_instruction
avx_pmovsxbd_instruction:
	mov	[mmx_size],4
	jmp	avx_pmovsx_instruction
avx_pmovsxbq_instruction:
	mov	[mmx_size],2
	jmp	avx_pmovsx_instruction
avx_pmovsxwd_instruction:
	mov	[mmx_size],8
	jmp	avx_pmovsx_instruction
avx_pmovsxwq_instruction:
	mov	[mmx_size],4
	jmp	avx_pmovsx_instruction
avx_pmovsxdq_instruction:
	mov	[mmx_size],8
      avx_pmovsx_instruction:
	or	[vex_required],1
	call	setup_66_0f_38
	call	take_avx_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	xor	al,al
	xchg	al,[operand_size]
	push	eax
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	avx_pmovsx_xmmreg_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	pop	eax
	cmp	al,32
	jb	avx_pmovsx_size_check
	shl	[mmx_size],1
      avx_pmovsx_size_check:
	xchg	al,[operand_size]
	test	al,al
	jz	instruction_ready
	cmp	al,[mmx_size]
	jne	invalid_operand_size
	jmp	instruction_ready
      avx_pmovsx_xmmreg_reg:
	lods	byte [esi]
	call	convert_xmm_register
	mov	bl,al
	pop	eax
	mov	[operand_size],al
	jmp	nomem_instruction_ready
avx_permil_instruction:
	call	setup_66_0f_3a
	or	[vex_required],1
	call	take_avx_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	lods	byte [esi]
	call	get_size_operator
	cmp	al,'['
	je	avx_permil_reg_mem
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
	xchg	cl,[operand_size]
	pop	esi
	cmp	al,'['
	je	avx_permil_reg_reg_mem
	cmp	al,10h
	jne	avx_permil_reg_reg_imm8
	call	take_avx_register
	mov	bl,al
	mov	[extended_code],38h
	add	[supplemental_code],8
	jmp	nomem_instruction_ready
      avx_permil_reg_reg_mem:
	lods	byte [esi]
	call	get_size_operator
	call	get_address
	mov	[extended_code],38h
	add	[supplemental_code],8
	jmp	instruction_ready
      avx_permil_reg_reg_imm8:
	dec	esi
	xor	bl,bl
	xchg	bl,[vex_register]
	jmp	mmx_nomem_imm8
      avx_permil_reg_mem:
	call	get_address
	jmp	mmx_imm8
avx_bit_shift_instruction:
	mov	[opcode_prefix],66h
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	call	take_avx_register
	mov	[postbyte_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
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
	je	avx_bit_shift_regs_reg
	pop	esi
	cmp	al,'['
	je	avx_bit_shift_regs_mem
	xchg	cl,[operand_size]
	dec	esi
	mov	bl,[extended_code]
	mov	al,bl
	shr	bl,4
	and	al,1111b
	add	al,70h
	mov	[extended_code],al
	sub	bl,0Ch
	shl	bl,1
	xchg	bl,[postbyte_register]
	xchg	bl,[vex_register]
	jmp	mmx_nomem_imm8
      avx_bit_shift_regs_reg:
	pop	eax
	lods	byte [esi]
	call	convert_xmm_register
	xchg	cl,[operand_size]
	mov	bl,al
	jmp	nomem_instruction_ready
      avx_bit_shift_regs_mem:
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
avx_pslldq_instruction:
	mov	[postbyte_register],al
	mov	[opcode_prefix],66h
	mov	[base_code],0Fh
	mov	[extended_code],73h
	or	[vex_required],1
	call	take_avx_register
	mov	[vex_register],al
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	bl,al
	jmp	mmx_nomem_imm8

vzeroall_instruction:
	mov	[operand_size],32
vzeroupper_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],al
	or	[vex_required],1
	call	store_instruction_code
	jmp	instruction_assembled
vldmxcsr_instruction:
	or	[vex_required],1
	jmp	fxsave_instruction
vcvtph2ps_instruction:
	mov	[opcode_prefix],66h
	mov	[supplemental_code],al
	mov	al,38h
	jmp	avx_cvtps2pd_instruction
vcvtps2ph_instruction:
	call	setup_66_0f_3a
	or	[vex_required],1
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	je	vcvtps2ph_reg
	cmp	al,'['
	jne	invalid_operand
	call	get_address
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	shl	[operand_size],1
	call	take_avx_register
	mov	[postbyte_register],al
	jmp	mmx_imm8
      vcvtps2ph_reg:
	lods	byte [esi]
	call	convert_xmm_register
	mov	bl,al
	mov	[operand_size],0
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[postbyte_register],al
	jmp	mmx_nomem_imm8

bmi_instruction:
	mov	[base_code],0Fh
	mov	[extended_code],38h
	mov	[supplemental_code],0F3h
	mov	[postbyte_register],al
      bmi_reg:
	or	[vex_required],1
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
	or	[vex_required],1
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
	or	[vex_required],1
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
	or	[vex_required],1
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
	or	[vex_required],1
	call	get_reg_mem
	jc	rorx_reg_reg
	call	operand_32or64
	jmp	mmx_imm8
      rorx_reg_reg:
	call	operand_32or64
	jmp	mmx_nomem_imm8

fma_instruction_pd:
	or	[rex_prefix],8
fma_instruction_ps:
	mov	[mmx_size],0
	jmp	avx_instruction_38_setup
fma_instruction_sd:
	or	[rex_prefix],8
	mov	[mmx_size],8
	jmp	avx_instruction_38_setup
fma_instruction_ss:
	mov	[mmx_size],4
	jmp	avx_instruction_38_setup

fma4_instruction_p:
	mov	[mmx_size],0
	jmp	fma4_instruction_setup
fma4_instruction_sd:
	mov	[mmx_size],8
	jmp	fma4_instruction_setup
fma4_instruction_ss:
	mov	[mmx_size],4
      fma4_instruction_setup:
	mov	[immediate_size],-2
	mov	byte [value],0
	jmp	avx_instruction_3a_setup

xop_single_source_sd_instruction:
	or	[vex_required],2
	mov	[mmx_size],8
	jmp	xop_instruction_9
xop_single_source_ss_instruction:
	or	[vex_required],2
	mov	[mmx_size],4
	jmp	xop_instruction_9
xop_single_source_instruction:
	or	[vex_required],2
	mov	[mmx_size],0
      xop_instruction_9:
	mov	[base_code],al
	mov	[xop_opcode_map],9
	jmp	avx_common
xop_single_source_128bit_instruction:
	or	[vex_required],2
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
	jmp	avx_common
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
	mov	[mmx_size],16
	mov	[base_code],ah
	mov	[xop_opcode_map],8
	jmp	avx_common
vpcmov_instruction:
	or	[vex_required],1
	mov	[immediate_size],-2
	mov	byte [value],0
	mov	[mmx_size],0
	mov	[base_code],al
	mov	[xop_opcode_map],8
	jmp	avx_common
xop_shift_instruction:
	mov	[base_code],al
	or	[vex_required],1
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
	mov	[base_code],0Fh
	mov	[supplemental_code],al
	mov	al,3Ah
	mov	[mmx_size],0
	jmp	avx_instruction

tbm_instruction:
	mov	[xop_opcode_map],9
	mov	ah,al
	shr	ah,4
	and	al,111b
	mov	[base_code],ah
	mov	[postbyte_register],al
	jmp	bmi_reg

llwpcb_instruction:
	or	[vex_required],1
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
	or	[vex_required],1
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

gather_instruction_pd:
	or	[rex_prefix],8
gather_instruction_ps:
	call	setup_66_0f_38
	or	[vex_required],4
	call	take_avx_register
	mov	[postbyte_register],al
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
	test	al,al
	jz	gather_elements_size_ok
	test	[rex_prefix],8
	jnz	gather_elements_64bit
	cmp	al,4
	jne	invalid_operand_size
	jmp	gather_elements_size_ok
      gather_elements_64bit:
	cmp	al,8
	jne	invalid_operand_size
      gather_elements_size_ok:
	lods	byte [esi]
	cmp	al,','
	jne	invalid_operand
	call	take_avx_register
	mov	[vex_register],al
	cmp	al,[postbyte_register]
	je	disallowed_combination_of_registers
	mov	al,bl
	and	al,1111b
	cmp	al,[postbyte_register]
	je	disallowed_combination_of_registers
	cmp	al,[vex_register]
	je	disallowed_combination_of_registers
	mov	al,bl
	shr	al,4
	cmp	al,0Ch
	je	gather_vr_128bit
	mov	al,[rex_prefix]
	shr	al,3
	xor	al,[supplemental_code]
	test	al,1
	jz	gather_256bit
	test	[supplemental_code],1
	jz	invalid_operand_size
	mov	al,32
	xchg	al,[operand_size]
	cmp	al,16
	jne	invalid_operand_size
	jmp	instruction_ready
      gather_256bit:
	cmp	ah,32
	jne	invalid_operand_size
	jmp	instruction_ready
      gather_vr_128bit:
	cmp	ah,16
	je	instruction_ready
	test	[supplemental_code],1
	jnz	invalid_operand_size
	test	[rex_prefix],8
	jz	invalid_operand_size
	jmp	instruction_ready

take_avx_register:
	lods	byte [esi]
	call	get_size_operator
	cmp	al,10h
	jne	invalid_operand
	lods	byte [esi]
convert_avx_register:
	mov	ah,al
	and	al,0Fh
	and	ah,0F0h
	sub	ah,0B0h
	jbe	invalid_operand
	cmp	ah,32
	ja	invalid_operand
	cmp	al,8
	jb	match_register_size
	cmp	[code_type],64
	jne	invalid_operand
	jmp	match_register_size
store_vex_instruction_code:
	mov	al,[base_code]
	cmp	al,0Fh
	jne	store_xop_instruction_code
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
	jne	vex_l_bit_ok
	or	al,100b
      vex_l_bit_ok:
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
