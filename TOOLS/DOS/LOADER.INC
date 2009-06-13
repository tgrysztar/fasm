
segment loader use16

init:

	mov	ax,1687h
	int	2Fh
	or	ax,ax			; DPMI installed?
	jnz	short no_dpmi
	test	bl,1			; 32-bit programs supported?
	jz	short no_dpmi
	mov	word [cs:mode_switch],di
	mov	word [cs:mode_switch+2],es
	mov	bx,si			; allocate memory for DPMI data
	mov	ah,48h
	int	21h
	jnc	init_protected_mode
  init_failed:
	call	init_error
	db	'error: DPMI initialization failed.',0Dh,0Ah,0
  no_dpmi:
	call	init_error
	db	'error: 32-bit DPMI services are not available.',0Dh,0Ah,0
  init_error:
	pop	si
	push	cs
	pop	ds
      display_error:
	lodsb
	test	al,al
	jz	short error_finish
	mov	dl,al
	mov	ah,2
	int	21h
	jmp	short display_error
      error_finish:
	mov	ax,4CFFh
	int	21h
  init_protected_mode:
	mov	es,ax
	mov	ds,[ds:2Ch]
	mov	ax,1
	call	far [cs:mode_switch]	; switch to protected mode
	jc	init_failed
	mov	cx,1
	xor	ax,ax
	int	31h			; allocate descriptor for code
	jc	init_failed
	mov	si,ax
	xor	ax,ax
	int	31h			; allocate descriptor for data
	jc	init_failed
	mov	di,ax
	mov	dx,cs
	lar	cx,dx
	shr	cx,8
	or	cx,0C000h
	mov	bx,si
	mov	ax,9
	int	31h			; set code descriptor access rights
	jc	init_failed
	mov	dx,ds
	lar	cx,dx
	shr	cx,8
	or	cx,0C000h
	mov	bx,di
	int	31h			; set data descriptor access rights
	jc	init_failed
	mov	ecx,main
	shl	ecx,4
	mov	dx,cx
	shr	ecx,16
	mov	ax,7
	int	31h			; set data descriptor base address
	jc	init_failed
	mov	bx,si
	int	31h			; set code descriptor base address
	jc	init_failed
	mov	cx,0FFFFh
	mov	dx,0FFFFh
	mov	ax,8			; set segment limit to 4 GB
	int	31h
	jc	init_failed
	mov	bx,di
	int	31h
	jc	init_failed
	mov	ax,ds
	mov	ds,di
	mov	[psp_selector],es
	mov	[environment_selector],ax
	cli
	mov	ss,di
	mov	esp,stack_top
	sti
	mov	es,di
	xor	eax,eax
	mov	[memory_handles_count],eax
	push	si
	push	start
	retf

  mode_switch dd ?
