
; flat assembler  version 1.0
; Copyright (c) 1999-2000, Tomasz Grysztar
; All rights reserved.

init_flatrm:
	push	sp			; store stack frame
	pop	ax
	cmp	ax,sp			; compare stack frame values
	jne	processor_error 	; if not equal, CPU is 8086-80186
	smsw	ax
	test	al,1			; processor is in protected mode?
	jnz	pm_error		; if yes, generate error
	pushf
	pop	ax
	or	ah,F0h			; try to set high 4 bits of FLAGS
	push	ax
	popf
	pushf
	pop	ax
	test	ah,F0h
	jz	processor_error 	; if bits not set, CPU is 80286
	xor	eax,eax 		; calculate linear address of GDT
	mov	ax,cs
	shl	eax,4
	add	eax,GDT
	mov	dword [cs:GDTR+2],eax
	cli				; disable interrupts
	lgdt	pword [cs:GDTR] 	; load GDT register
	mov	eax,cr0 		; switch to protected mode
	or	al,1
	mov	cr0,eax
	jmp	pm_start
    pm_start:
	mov	ax,1 shl 3		; load 32-bit data descriptor
	mov	ds,ax			; to all data segment registers
	mov	es,ax
	mov	fs,ax
	mov	gs,ax
	mov	eax,cr0 		; switch back to real mode
	and	al,not 1
	mov	cr0,eax
	jmp	pm_end
    pm_end:
	xor	eax,eax
	mov	ax,cs			; restore visible part of registers
	mov	ds,ax
	mov	es,ax
	mov	fs,ax
	mov	gs,ax
	shl	eax,4
	mov	[program_base],eax	; linear address of program segment
	sti
	ret

GDTR dw 2*8-1				; limit of GDT
     dd ?				; linear address of GDT

GDT rw 4				; null descriptor
    dw FFFFh,0,9200h,8Fh		; 32-bit data descriptor

init_memory:
	mov	ax,4300h		; check for XMS
	int	2Fh
	cmp	al,80h			; XMS present?
	je	xms_init
	mov	ah,88h			; how much extended memory free?
	int	15h
	or	ax,ax
	jz	no_extended_memory
	movzx	eax,ax			; convert AX kilobytes to pointer
	shl	eax,10
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
	jne	byte no_vdisk		; if present, get base of free memory
	mov	eax,dword [bx+2Ch]	; get first free extended memory byte
	add	eax,Fh			; align on paragraph
	and	eax,FFFFF0h		; address is only 24bit
    no_vdisk:
	push	FFFFh			; DS := FFFFh for ext mem addressing
	pop	ds
	cmp	dword [13h],'VDIS'	; VDISK memory allocation?
	jne	byte vdisk_ok		; if present, get base of free memory
	movzx	ebx,word [2Eh]		; get first free kilobyte
	shl	ebx,10
	cmp	eax,ebx 		; pick larger of 2 addresses
	ja	byte vdisk_ok
	mov	eax,ebx
    vdisk_ok:
	pop	ds
	sub	eax,[program_base]
	mov	[memory_start],eax
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
	ret
    kb_wait:				; wait for safe to write to 8042
	xor	cx,cx
      .loop:
	in	al,64h			; read 8042 status
	test	al,2			; buffer full?
	loopnz	.loop			; if yes, loop
	ret
    test_a20:				; test for enabled A20
	mov	al,[0]			; get byte from 0:0
	mov	ah,al			; preserve old byte
	not	al			; modify byte
	xchg	al,[100000h]		; put modified byte to 0FFFFh:10h
	cmp	ah,[0]			; set zero if byte at 0:0 not modified
	mov	[100000h],al		; restore byte at 0FFFFh:10h
	ret				; return, zero if A20 enabled

xms_init:
	push	es
	mov	ax,4310h		; get XMS driver address
	int	2Fh
	mov	word [xms_call],bx	; store XMS driver address
	mov	word [xms_call+2],es
	pop	es
	mov	ah,3			; enable A20
	call	far dword [xms_call]
	cmp	ax,1			; error enabling A20?
	jne	no_extended_memory
	mov	ah,8			; get free extended memory size
	xor	bl,bl
	call	far dword [xms_call]
	or	bl,bl
	jnz	no_extended_memory
	mov	dx,ax
	movzx	eax,ax
	shl	eax,10
	mov	[memory_end],eax
	mov	ah,9			; allocate largest memory block
	call	far dword [xms_call]
	mov	[xms_handle],dx
	cmp	ax,1
	jne	no_extended_memory
	mov	ah,Ch			; lock extended memory block
	call	far dword [xms_call]
	cmp	ax,1
	jne	no_extended_memory
	shl	edx,16
	mov	dx,bx
	sub	edx,[program_base]
	mov	[memory_start],edx	; store memory block address
	add	[memory_end],edx
	jmp	extended_memory_ok

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
	mov	[conventional_memory],edi
	mov	[conventional_memory_end],edi
	add	[conventional_memory_end],ecx
	cmp	[memory_start],0	; if extended memory available, done
	jne	memory_ok
	shr	ecx,2			; use part of conventional memory
	add	edi,ecx 		; as a substitute for extended memory
	mov	[memory_start],edi
	xchg	[conventional_memory_end],edi
	mov	[memory_end],edi
    memory_ok:
	ret

exit_program:
	cmp	[xms_handle],0
	je	byte .exit
	push	ax
	mov	ah,Dh			; unlock extended memory block
	mov	dx,[xms_handle]
	call	far dword [xms_call]
	mov	ah,Ah			; free extended memory block
	call	far dword [xms_call]
	pop	ax
      .exit:
	mov	ah,4Ch
	int	21h

xms_call dd ?				; XMS driver pointer
xms_handle dw ? 			; handle of XMS memory block

open:
	push	esi di
	mov	esi,edx
	mov	di,buffer
      .name:
	lods	byte [esi]
	stos	byte [di]
	or	al,al
	jnz	.name
	mov	dx,buffer
	mov	ax,3D00h
	int	21h
	mov	bx,ax
	pop	di esi
	ret
create:
	push	esi edi
	mov	esi,edx
	mov	di,buffer
      .name:
	lods	byte [esi]
	stos	byte [es:di]
	or	al,al
	jnz	.name
	mov	dx,buffer
	mov	ah,3Ch
	xor	cx,cx
	int	21h
	mov	bx,ax
	pop	edi esi
	ret
write:
	push	esi edi ebp
	mov	ebp,ecx
	mov	esi,edx
      .loop:
	mov	ecx,4000h
	sub	ebp,4000h
	jnc	.write
	add	ebp,4000h
	mov	cx,bp
	xor	ebp,ebp
      .write:
	push	cx
	mov	edi,buffer
	shr	cx,2
	rep	movs dword [edi],[esi]
	mov	cx,[esp]
	and	cx,11b
	rep	movs byte [edi],[esi]
	pop	cx
	mov	dx,buffer
	mov	ah,40h
	int	21h
	or	ebp,ebp
	jnz	.loop
	pop	ebp edi esi
	ret
read:
	push	esi edi ebp
	mov	ebp,ecx
	mov	edi,edx
      .loop:
	mov	ecx,4000h
	sub	ebp,4000h
	jnc	.read
	add	ebp,4000h
	mov	cx,bp
	xor	ebp,ebp
      .read:
	push	cx
	mov	dx,buffer
	mov	ah,3Fh
	int	21h
	mov	esi,buffer
	mov	cx,[esp]
	shr	cx,2
	rep	movs dword [edi],[esi]
	pop	cx
	and	cx,11b
	rep	movs byte [edi],[esi]
	or	ebp,ebp
	jnz	.loop
	pop	ebp edi esi
	ret
close:
	mov	ah,3Eh
	int	21h
	ret
lseek:
	mov	ah,42h
	mov	ecx,edx
	shr	ecx,16
	int	21h
	shl	edx,16
	mov	dx,ax
	mov	eax,edx
	ret
