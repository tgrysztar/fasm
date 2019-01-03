
; flat assembler interface for DOS
; Copyright (c) 1999-2019, Tomasz Grysztar.
; All rights reserved.

segment modes use16

real32:
	mov	ax,7202h
	push	ax
	popf
	pushf
	pop	bx
	cmp	ax,bx
	je	processor_ok
	call	init_error
	db	'required 80386 or better',24h
    processor_ok:
	mov	eax,ds
	shl	eax,4
	mov	[program_base],eax
	mov	eax,buffer
	shl	eax,4
	sub	eax,[program_base]
	mov	[buffer_address],eax

if UNREAL_ENABLED>0

	smsw	ax
	test	al,1
	jnz	dpmi
	mov	eax,cs			; calculate linear address of GDT
	shl	eax,4
	or	dword [cs:real32_GDT+10],eax
	or	dword [cs:real16_GDT+10],eax
	add	[cs:real32_GDT_address],eax
	add	[cs:real16_GDT_address],eax
	cli
	lgdt	[cs:real32_GDTR]
	mov	eax,cr0
	or	al,1
	mov	cr0,eax
	jmp	1 shl 3:test_pm32
    no_rm32:
	sti
	jmp	dpmi
    test_pm32:
	use32
	mov	eax,cr0
	and	al,not 1
	mov	cr0,eax
	mov	ebx,0FFFFh
	jmp	modes:test_rm32
    test_rm32:
	inc	ebx
	jz	short no_rm32
	lgdt	[cs:real16_GDTR]
	mov	eax,cr0
	or	al,1
	mov	cr0,eax
	jmp	1 shl 3:test_pm16
    test_pm16:
	use16
	mov	eax,cr0
	and	al,not 1
	mov	cr0,eax
	jmp	modes:test_rm16
    test_rm16:
	sti
	mov	bx,(400h+(100h*interrupt.size)) shr 4
	mov	ah,48h
	int	21h
	jc	not_enough_memory
	push	ds es
	mov	es,ax
	push	cs
	pop	ds
	movzx	eax,ax
	shl	eax,4
	mov	[real32_IDT_base],eax
	mov	dx,100h
	xor	bx,bx
	mov	di,400h
    init_interrupts:
	mov	si,interrupt
	mov	[si+interrupt.vector],bx
	mov	word [es:bx],di
	mov	word [es:bx+2],es
	mov	cx,interrupt.size
	rep	movsb
	add	bx,4
	dec	dx
	jnz	init_interrupts
	pop	es ds
	call	modes:switch_real32
	use32
	mov	[mode],real32
	retfw
	use16

switch_real32:
	pushfw
	push	eax
	push	word ds
	push	word es
	push	word fs
	push	word gs
	cli
	mov	eax,ss
	mov	cr3,eax
	lgdt	[cs:real32_GDTR]
	mov	eax,cr0 		; switch to protected mode
	or	al,1
	mov	cr0,eax
	jmp	1 shl 3:pm32_start
    pm32_start:
	use32
	mov	ax,2 shl 3		; load 32-bit data descriptor
	mov	ds,ax			; to all data segment registers
	mov	es,ax
	mov	fs,ax
	mov	gs,ax
	mov	ss,ax
	mov	eax,cr0 		; switch back to real mode
	and	al,not 1
	mov	cr0,eax
	jmp	modes:pm32_end
    pm32_end:
	mov	eax,cr3
	mov	ss,ax
	lidt	[cs:real32_IDTR]
	pop	word gs
	pop	word fs
	pop	word es
	pop	word ds
	pop	eax
	popfw
	retfw

switch_real16:
	pushfw
	push	eax
	cli
	lgdt	[cs:real16_GDTR]
	mov	eax,cr0 		; switch to protected mode
	or	al,1
	mov	cr0,eax
	jmp	1 shl 3:pm16_start
    pm16_start:
	use16
	mov	eax,cr0 		; switch back to real mode
	and	al,not 1
	mov	cr0,eax
	jmp	modes:pm16_end
    pm16_end:
	lidt	[cs:real16_IDTR]
	pop	eax
	popfw
	retfd
	use32

interrupt:
	call	modes:switch_real16
	use16
	movzx	esp,sp
	push	word [esp+4]
	push	cs
	call	.real16
	pushfw
	pop	word [esp+4]
	call	modes:switch_real32
	use32
	iretw
      .real16:
	use16
	push	eax
	push	ds
	xor	ax,ax
	mov	ds,ax
	mov	eax,[word 0]
	label	.vector word at $-2-interrupt
	pop	ds
	xchg	eax,[esp]
	retfw
      .size = $-interrupt

label real32_GDTR pword
real32_GDT_limit dw 3*8-1		; limit of GDT
real32_GDT_address dd real32_GDT	; linear address of GDT
real32_GDT rw 4 			; null descriptor
	   dw 0FFFFh,0,9A00h,0CFh	; 32-bit code descriptor
	   dw 0FFFFh,0,9200h,08Fh	; 4 GB data descriptor
label real16_GDTR pword
real16_GDT_limit dw 2*8-1		; limit of GDT
real16_GDT_address dd real16_GDT	; linear address of GDT
real16_GDT rw 4 			; null descriptor
	   dw 0FFFFh,0,9A00h,0		; 16-bit code descriptor

label real32_IDTR pword
real32_IDT_limit dw 3FFh
real32_IDT_base dd ?
label real16_IDTR pword
real16_IDT_limit dw 3FFh
real16_IDT_base dd 0

end if

dpmi:
	mov	ax,1687h
	int	2Fh
	or	ax,ax			; DPMI installed?
	jnz	no_dpmi
	test	bl,1			; 32-bit programs supported?
	jz	no_dpmi
	mov	word [cs:mode_switch],di
	mov	word [cs:mode_switch+2],es
	mov	bx,si			; allocate memory for DPMI data
	mov	ah,48h
	int	21h
	jc	not_enough_memory
	mov	ds,[environment_segment]
	mov	es,ax
	mov	ax,1
	call	far [cs:mode_switch]	; switch to protected mode
	jc	no_dpmi
	mov	cx,1
	xor	ax,ax
	int	31h			; allocate descriptor for code
	mov	si,ax
	xor	ax,ax
	int	31h			; allocate descriptor for data
	mov	di,ax
	mov	dx,cs
	lar	cx,dx
	shr	cx,8
	or	cx,0C000h
	mov	bx,si
	mov	ax,9
	int	31h			; set code descriptor access rights
	mov	dx,ds
	lar	cx,dx
	shr	cx,8
	or	cx,0C000h
	mov	bx,di
	int	31h			; set data descriptor access rights
	mov	ecx,main
	shl	ecx,4
	mov	dx,cx
	shr	ecx,16
	mov	ax,7
	int	31h			; set data descriptor base address
	movzx	ecx,word [esp+2]
	shl	ecx,4
	mov	dx,cx
	shr	ecx,16
	mov	bx,si
	int	31h			; set code descriptor base address
	mov	cx,0FFFFh
	mov	dx,0FFFFh
	mov	ax,8			; set segment limit to 4 GB
	int	31h
	mov	bx,di
	int	31h
	mov	ax,ds
	mov	ds,di
	mov	[psp_segment],es
	mov	[environment_segment],ax
	mov	es,di
	mov	[mode],dpmi
	pop	ebx
	movzx	ebx,bx
	push	esi
	push	ebx
	retfd

init_error:
	push	cs
	pop	ds
	mov	dx,init_error_prefix
	mov	ah,9
	int	21h
	pop	dx
	int	21h
	mov	dx,init_error_suffix
	int	21h
	mov	ax,04CFFh
	int	21h

init_error_prefix db 0Dh,0Ah,'error: ',24h
init_error_suffix db '.',0Dh,0Ah,24h

mode_switch dd ?

    not_enough_memory:
	call	init_error
	db	'not enough conventional memory',24h

if UNREAL_ENABLED>0

    no_dpmi:
	smsw	ax
	test	al,1
	jz	no_real32
	call	init_error
	db	'system is in protected mode without 32-bit DPMI services',24h
    no_real32:
	call	init_error
	db	'processor is not able to enter 32-bit real mode',24h

else

    no_dpmi:
	call	init_error
	db	'no 32-bit DPMI services are available',24h

end if

use32

if UNREAL_ENABLED>0

init_real32_memory:
	mov	ax,4300h		; check for XMS
	int	2Fh
	cmp	al,80h			; XMS present?
	je	xms_init
	mov	ax,0E801h		; check for large free extended memory
	int	15h
	jnc	large_raw_memory
	mov	ah,88h			; how much extended memory free?
	int	15h
	or	ax,ax
	jz	no_extended_memory
	movzx	eax,ax			; convert AX kilobytes to pointer
	shl	eax,10
	jmp	use_raw_memory
    large_raw_memory:
	movzx	ecx,cx
	shl	ecx,10
	movzx	edx,dx
	shl	edx,16
	mov	eax,ecx
	add	eax,edx
    use_raw_memory:
	add	eax,100000h
	sub	eax,[program_base]
	mov	[memory_end],eax
	push	ds
	push	0			; DS := 0
	pop	ds
	call	enable_a20		; enable A20
	call	test_a20		; is A20 enabled?
	jz	a20_ok
	pop	ds
	jmp	no_extended_memory
    a20_ok:
	lds	bx,dword [4*19h]
	mov	eax,100000h		; initial free extended memory base
	cmp	dword [bx+12h],'VDIS'	; VDISK memory allocation?
	jne	short no_vdisk		; if present, get base of free memory
	mov	eax,dword [bx+2Ch]	; get first free extended memory byte
	add	eax,0Fh 		; align on paragraph
	and	eax,0FFFFF0h		; address is only 24bit
    no_vdisk:
	push	0FFFFh			; DS := FFFFh for ext mem addressing
	pop	ds
	cmp	dword [13h],'VDIS'	; VDISK memory allocation?
	jne	short vdisk_ok		; if present, get base of free memory
	movzx	ebx,word [2Eh]		; get first free kilobyte
	shl	ebx,10
	cmp	eax,ebx 		; pick larger of 2 addresses
	ja	short vdisk_ok
	mov	eax,ebx
    vdisk_ok:
	pop	ds
	sub	eax,[program_base]
	mov	[memory_start],eax
	mov	edx,[memory_setting]
	shl	edx,10
	jz	extended_memory_ok
	mov	eax,[memory_end]
	sub	eax,[memory_start]
	sub	eax,edx
	jbe	extended_memory_ok
	sub	[memory_end],eax
	jmp	extended_memory_ok
enable_a20:
	call	test_a20		; is A20 already enabled?
	jz	a20_enabled		; if yes, done
	in	al,92h			; PS/2 A20 enable
	or	al,2
	out	92h,al
	call	test_a20		; is A20 enabled?
	jz	a20_enabled		; if yes, done
	call	kb_wait 		; AT A20 enable
	jnz	a20_enabled
	mov	al,0D1h
	out	64h,al
	call	kb_wait
	jnz	a20_enabled
	mov	al,0DFh
	out	60h,al
	call	kb_wait
    a20_enabled:
	retn
    kb_wait:				; wait for safe to write to 8042
	xor	cx,cx
      .loop:
	in	al,64h			; read 8042 status
	test	al,2			; buffer full?
	loopnz	.loop			; if yes, loop
	retn
    test_a20:				; test for enabled A20
	mov	al,[0]			; get byte from 0:0
	mov	ah,al			; preserve old byte
	not	al			; modify byte
	xchg	al,[100000h]		; put modified byte to 0FFFFh:10h
	cmp	ah,[0]			; set zero if byte at 0:0 not modified
	mov	[100000h],al		; restore byte at 0FFFFh:10h
	retn				; return, zero if A20 enabled
xms_init:
	push	es
	mov	ax,4310h		; get XMS driver address
	int	2Fh
	mov	word [cs:xms_proc],bx	; store XMS driver address
	mov	word [cs:xms_proc+2],es
	pop	es
	mov	ah,3			; enable A20
	call	call_xms
	cmp	ax,1			; error enabling A20?
	jne	no_extended_memory
	mov	ah,88h			; get free extended memory size (XMS 3.0)
	xor	bl,bl
	call	call_xms
	or	bl,bl
	jz	xms_large_init
	mov	ah,8			; get free extended memory size
	xor	bl,bl
	call	call_xms
	or	bl,bl
	jnz	no_extended_memory
	mov	dx,ax
	movzx	eax,ax
	shl	eax,10
	mov	[memory_end],eax
	mov	ah,9			; allocate largest memory block
    xms_allocate:
	mov	ecx,[memory_setting]
	shl	ecx,10
	jz	xms_size_ok
	cmp	ecx,[memory_end]
	jae	xms_size_ok
	mov	[memory_end],ecx
	mov	edx,[memory_setting]
    xms_size_ok:
	call	call_xms
	mov	[cs:xms_handle],dx
	cmp	ax,1
	jne	no_extended_memory
	mov	ah,0Ch			; lock extended memory block
	call	call_xms
	cmp	ax,1
	jne	no_extended_memory
	shl	edx,16
	mov	dx,bx
	sub	edx,[program_base]
	mov	[memory_start],edx	; store memory block address
	add	[memory_end],edx
	jmp	extended_memory_ok
    xms_large_init:
	mov	edx,eax
	shl	eax,10
	mov	[memory_end],eax
	mov	ah,89h			; allocate largest memory block (XMS 3.0)
	jmp	xms_allocate
    call_xms:
	call	modes:switch_real16
	use16
	call	far dword [cs:xms_proc]
	call	modes:switch_real32
	use32
	retn
no_extended_memory:
	xor	eax,eax
	mov	[memory_start],eax
extended_memory_ok:
	mov	ah,48h			; get free conventional memory size
	mov	bx,-1
	int	21h
	movzx	ecx,bx
	shl	ecx,4
	mov	ah,48h			; allocate all conventional memory
	int	21h
	movzx	edi,ax
	shl	edi,4
	sub	edi,[program_base]
	mov	[additional_memory],edi
	mov	[additional_memory_end],edi
	add	[additional_memory_end],ecx
	cmp	[memory_start],0
	je	only_conventional_memory
	mov	eax,[memory_end]
	sub	eax,[memory_start]
	shr	eax,2
	cmp	eax,ecx
	jbe	real32_memory_ok
	mov	eax,[memory_end]
	mov	ebx,[memory_start]
	sub	eax,ebx
	shr	eax,2
	mov	[additional_memory],ebx
	add	ebx,eax
	mov	[additional_memory_end],ebx
	mov	[memory_start],ebx
    real32_memory_ok:
	retf
    only_conventional_memory:
	shr	ecx,2			; use part of conventional memory
	add	edi,ecx 		; as a substitute for extended memory
	mov	[memory_start],edi
	xchg	[additional_memory_end],edi
	mov	[memory_end],edi
	retf

free_real32_memory:
	cmp	[cs:xms_handle],0
	je	no_xms
	mov	ah,0Dh			; unlock extended memory block
	mov	dx,[cs:xms_handle]
	call	call_xms
	mov	ah,0Ah			; free extended memory block
	call	call_xms
    no_xms:
	retf

xms_proc dd ?				; XMS driver pointer
xms_handle dw ? 			; handle of XMS memory block

end if
