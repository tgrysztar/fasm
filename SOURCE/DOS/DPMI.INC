
; flat assembler interface for DOS
; Copyright (c) 1999-2019, Tomasz Grysztar.
; All rights reserved.

init_dpmi_memory:
	mov	ax,500h 		; get free memory information
	mov	edi,[buffer_address]
	int	31h
	mov	ebx,[edi]
    allocate_dpmi_memory:
	mov	edx,[memory_setting]
	shl	edx,10
	jz	dpmi_memory_size_ok
	cmp	ebx,edx
	jbe	dpmi_memory_size_ok
	mov	ebx,edx
    dpmi_memory_size_ok:
	mov	[memory_end],ebx
	mov	ecx,ebx
	shr	ebx,16
	mov	ax,501h
	int	31h
	jnc	dpmi_memory_ok
	mov	ebx,[memory_end]
	shr	ebx,1
	cmp	ebx,4000h
	jb	out_of_memory
	jmp	allocate_dpmi_memory
    dpmi_memory_ok:
	shl	ebx,16
	mov	bx,cx
	sub	ebx,[program_base]
	jc	out_of_memory
	mov	[memory_start],ebx
	add	[memory_end],ebx
	mov	ax,100h 		; get free conventional memory size
	mov	bx,-1
	int	31h
	movzx	ecx,bx
	shl	ecx,4
	jecxz	no_conventional_memory
	mov	ax,100h 		; allocate all conventional memory
	int	31h
	movzx	edi,ax
	shl	edi,4
	sub	edi,[program_base]
	jc	no_conventional_memory
	mov	[additional_memory],edi
	mov	[additional_memory_end],edi
	add	[additional_memory_end],ecx
	mov	eax,[memory_end]
	sub	eax,[memory_start]
	shr	eax,2
	cmp	eax,ecx
	ja	no_conventional_memory
	ret
    no_conventional_memory:
	mov	eax,[memory_end]
	mov	ebx,[memory_start]
	sub	eax,ebx
	shr	eax,2
	mov	[additional_memory],ebx
	add	ebx,eax
	mov	[additional_memory_end],ebx
	mov	[memory_start],ebx
	ret

dpmi_dos_int:
	mov	[real_mode_segment],main
    simulate_real_mode:
	push	0			; SS:SP (DPMI will allocate stack)
	push	0			; CS:IP (ignored)
	push	0
	push	[real_mode_segment]	; DS
	push	[real_mode_segment]	; ES
	stc
	pushfw
	push	eax
	push	ecx
	push	edx
	push	ebx
	push	0
	push	ebp
	push	esi
	push	edi
	mov	ax,300h
	mov	bx,21h
	xor	cx,cx
	mov	edi,esp
	push	es ss
	pop	es
	int	31h
	pop	es
	mov	edi,[esp]
	mov	esi,[esp+4]
	mov	ebp,[esp+8]
	mov	ebx,[esp+10h]
	mov	edx,[esp+14h]
	mov	ecx,[esp+18h]
	mov	ah,[esp+20h]
	add	esp,32h
	sahf
	mov	eax,[esp-32h+1Ch]
	ret
dpmi_dos_int_with_buffer:
	mov	[real_mode_segment],buffer
	jmp	simulate_real_mode
