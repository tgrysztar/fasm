
; flat assembler interface for DOS
; Copyright (c) 1999-2003, Tomasz Grysztar.
; All rights reserved.

init_flatrm:
	mov	ax,7202h
	push	ax
	popf
	pushf
	pop	bx
	cmp	ax,bx
	je	processor_ok
	call	fatal_error
	db	'required 80386 or better',24h
    processor_ok:
	smsw	ax
	test	al,1
	jz	mode_ok
	call	fatal_error
	db	'processor is in protected mode',24h
    mode_ok:
	mov	eax,cs
	shl	eax,4
	mov	[program_base],eax
	mov	ax,350Dh
	int	21h
	mov	word [irq_5],bx
	mov	word [irq_5+2],es
	push	cs
	pop	es
	mov	ax,250Dh
	mov	dx,int_13
	int	21h
	ret

int_13:
	push	eax
	mov	al,00001011b		; OCW3 - read IRQ in-service register
	out	20h,al
	in	al,20h
	test	al,00100000b		; is IRQ 5 in service?
	jz	exception
	pop	eax
	jmp	dword 0:0
	label	irq_5 dword at $-4
    exception:
	push	ds es fs gs
	cli				; disable interrupts
	xor	eax,eax 		; calculate linear address of GDT
	mov	ax,cs
	shl	eax,4
	add	eax,GDT
	mov	dword [cs:GDTR+2],eax
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
	sti				; enable interrupts
	pop	gs fs es ds
	pop	eax
	iret

GDTR dw 2*8-1				; limit of GDT
     dd ?				; linear address of GDT

GDT rw 4				; null descriptor
    dw 0FFFFh,0,9200h,8Fh		; 32-bit data descriptor

use32 equ use16

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
	add	eax,0Fh 		; align on paragraph
	and	eax,0FFFFF0h		; address is only 24bit
    no_vdisk:
	push	0FFFFh			; DS := FFFFh for ext mem addressing
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
	mov	ah,0Ch			; lock extended memory block
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
	mov	[additional_memory],edi
	mov	[additional_memory_end],edi
	add	[additional_memory_end],ecx
	cmp	[memory_start],0	; if extended memory available, done
	jne	memory_ok
	shr	ecx,3			; use part of conventional memory
	add	edi,ecx 		; as a substitute for extended memory
	mov	[memory_start],edi
	xchg	[additional_memory_end],edi
	mov	[memory_end],edi
    memory_ok:
	ret

exit_program:
	push	ax
	cmp	[xms_handle],0
	je	byte .exit
	mov	ah,0Dh			; unlock extended memory block
	mov	dx,[xms_handle]
	call	far dword [xms_call]
	mov	ah,0Ah			; free extended memory block
	call	far dword [xms_call]
      .exit:
	mov	ax,250Dh
	mov	dx,word [irq_5]
	mov	ds,word [irq_5+2]
	int	21h
	pop	ax
	mov	ah,4Ch
	int	21h

xms_call dd ?				; XMS driver pointer
xms_handle dw ? 			; handle of XMS memory block

get_params:
	mov	si,81h
	mov	di,params
    find_param:
	lodsb
	cmp	al,20h
	je	find_param
	cmp	al,0Dh
	je	all_params
	or	al,al
	jz	all_params
	inc	di
	mov	bx,di
    copy_param:
	stosb
	lodsb
	cmp	al,20h
	je	param_end
	cmp	al,0Dh
	je	param_end
	or	al,al
	jz	param_end
	jmp	copy_param
    param_end:
	dec	si
	xor	al,al
	stosb
	mov	ax,di
	sub	ax,bx
	mov	[bx-1],al
	jmp	find_param
    all_params:
	xor	al,al
	stosb
	ret

open:
	push	esi di
	call	convert_path
	mov	ax,3D00h
	int	21h
	mov	bx,ax
	pop	di esi
	ret
    convert_path:
	mov	esi,edx
	mov	di,buffer
      copy_path:
	lods	byte [esi]
	cmp	al,'%'
	je	environment_variable
	cmp	al,'/'
	jne	path_char_ok
	mov	al,'\'
      path_char_ok:
	stos	byte [di]
	or	al,al
	jnz	copy_path
	sub	di,buffer
	cmp	di,1000h
	ja	out_of_memory
	mov	dx,buffer
	ret
      environment_variable:
	mov	ebx,esi
      find_variable_end:
	lods	byte [esi]
	or	al,al
	jz	not_environment_variable
	cmp	al,'%'
	jne	find_variable_end
	push	esi
	mov	si,cs
	neg	si
	add	si,[2Ch]
	movsx	esi,si
	sal	esi,4
      compare_variable_names:
	mov	edx,ebx
      compare_character:
	lods	byte [esi]
	mov	ah,[edx]
	inc	edx
	cmp	al,'='
	je	end_of_variable_name
	cmp	ah,'%'
	je	next_variable
	sub	ah,al
	jz	compare_character
	cmp	ah,20h
	jne	next_variable
	cmp	al,41h
	jb	next_variable
	cmp	al,5Ah
	jna	compare_character
      next_variable:
	lods	byte [esi]
	or	al,al
	jnz	next_variable
	cmp	byte [esi],0
	jne	compare_variable_names
	pop	esi
	jmp	copy_path
      end_of_variable_name:
	cmp	ah,'%'
	jne	next_variable
      copy_variable_value:
	lods	byte [esi]
	cmp	al,'/'
	jne	value_char_ok
	mov	al,'\'
      value_char_ok:
	stos	byte [di]
	or	al,al
	jnz	copy_variable_value
	dec	di
	pop	esi
	jmp	copy_path
      not_environment_variable:
	mov	al,'%'
	stos	byte [di]
	mov	esi,ebx
	jmp	copy_path
create:
	push	esi di
	call	convert_path
	mov	ah,3Ch
	xor	cx,cx
	int	21h
	mov	bx,ax
	pop	di esi
	ret
write:
	push	edx esi edi ebp
	mov	ebp,ecx
	mov	esi,edx
      .loop:
	mov	ecx,1000h
	sub	ebp,1000h
	jnc	.write
	add	ebp,1000h
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
	pop	ebp edi esi edx
	ret
read:
	push	edx esi edi ebp
	mov	ebp,ecx
	mov	edi,edx
      .loop:
	mov	ecx,1000h
	sub	ebp,1000h
	jnc	.read
	add	ebp,1000h
	mov	cx,bp
	xor	ebp,ebp
      .read:
	push	cx
	mov	dx,buffer
	mov	ah,3Fh
	int	21h
	cmp	ax,cx
	jne	.eof
	mov	esi,buffer
	mov	cx,[esp]
	shr	cx,2
	rep	movs dword [edi],[esi]
	pop	cx
	and	cx,11b
	rep	movs byte [edi],[esi]
	or	ebp,ebp
	jnz	.loop
      .exit:
	pop	ebp edi esi edx
	ret
      .eof:
	pop	cx
	stc
	jmp	.exit
close:
	mov	ah,3Eh
	int	21h
	ret
lseek:
	mov	ah,42h
	mov	ecx,edx
	shr	ecx,16
	int	21h
	pushf
	shl	edx,16
	popf
	mov	dx,ax
	mov	eax,edx
	ret

display_string:
	lods	byte [esi]
	or	al,al
	jz	string_end
	mov	dl,al
	mov	ah,2
	int	21h
	jmp	display_string
      string_end:
	ret
display_block:
	lods	byte [esi]
	mov	dl,al
	mov	ah,2
	int	21h
	loopd	display_block
	ret
display_number:
	push	ebx
	mov	ecx,1000000000
	xor	edx,edx
	xor	bl,bl
      display_loop:
	div	ecx
	push	edx
	cmp	ecx,1
	je	display_digit
	or	bl,bl
	jnz	display_digit
	or	al,al
	jz	digit_ok
	not	bl
      display_digit:
	mov	dl,al
	add	dl,30h
	mov	ah,2
	int	21h
      digit_ok:
	mov	eax,ecx
	xor	edx,edx
	mov	ecx,10
	div	ecx
	mov	ecx,eax
	pop	eax
	or	ecx,ecx
	jnz	display_loop
	pop	ebx
	ret

fatal_error:
	mov	dx,error_prefix
	mov	ah,9
	int	21h
	pop	dx
	int	21h
	mov	dx,error_suffix
	int	21h
	mov	al,0FFh
	jmp	exit_program
assembler_error:
	call	flush_display_buffer
	mov	ebx,[current_line]
      find_error_home:
	test	byte [ebx+7],80h
	jz	error_home_ok
	mov	ebx,[ebx+8]
	jmp	find_error_home
      error_home_ok:
	mov	esi,[ebx]
	call	display_string
	mov	dx,line_number_start
	mov	ah,9
	int	21h
	mov	eax,[ebx+4]
	call	display_number
	mov	dl,']'
	mov	ah,2
	int	21h
	cmp	ebx,[current_line]
	je	line_number_ok
	mov	dl,20h
	mov	ah,2
	int	21h
	mov	esi,[current_line]
	mov	esi,[esi]
	movzx	ecx,byte [esi]
	inc	esi
	call	display_block
	mov	dx,line_number_start
	mov	ah,9
	int	21h
	mov	esi,[current_line]
	mov	eax,[esi+4]
	and	eax,7FFFFFFFh
	call	display_number
	mov	dl,']'
	mov	ah,2
	int	21h
      line_number_ok:
	mov	dx,line_data_start
	mov	ah,9
	int	21h
	mov	esi,ebx
	mov	edx,[esi]
	call	open
	mov	al,2
	xor	edx,edx
	call	lseek
	mov	edx,[esi+8]
	sub	eax,edx
	mov	[counter],eax
	xor	al,al
	call	lseek
	mov	esi,[memory_start]
      read_line_data:
	mov	ecx,100h
	cmp	ecx,[counter]
	jbe	bytes_count_ok
	mov	ecx,[counter]
      bytes_count_ok:
	sub	[counter],ecx
	push	ecx
	mov	edx,esi
	call	read
	pop	ecx
      get_line_data:
	mov	al,[esi]
	cmp	al,0Ah
	je	display_line_data
	cmp	al,0Dh
	je	display_line_data
	cmp	al,1Ah
	je	display_line_data
	or	al,al
	jz	display_line_data
	inc	esi
	loop	get_line_data
	cmp	[counter],0
	ja	read_line_data
      display_line_data:
	call	close
	mov	ecx,esi
	mov	esi,[memory_start]
	sub	ecx,esi
	call	display_block
	mov	dx,cr_lf
	mov	ah,9
	int	21h
	mov	dx,error_prefix
	int	21h
	pop	dx
	int	21h
	mov	dx,error_suffix
	int	21h
	mov	al,1
	jmp	exit_program

make_timestamp:
	mov	ah,2Ah
	int	21h
	push	dx cx
	movzx	ecx,cx
	mov	eax,ecx
	sub	eax,1970
	mov	ebx,365
	mul	ebx
	mov	ebp,eax
	mov	eax,ecx
	sub	eax,1969
	shr	eax,2
	add	ebp,eax
	mov	eax,ecx
	sub	eax,1901
	mov	ebx,100
	div	ebx
	sub	ebp,eax
	mov	eax,ecx
	xor	edx,edx
	sub	eax,1601
	mov	ebx,400
	div	ebx
	add	ebp,eax
	movzx	ecx,byte [esp+3]
	mov	eax,ecx
	dec	eax
	mov	ebx,30
	mul	ebx
	add	ebp,eax
	cmp	ecx,8
	jbe	months_correction
	mov	eax,ecx
	sub	eax,7
	shr	eax,1
	add	ebp,eax
	mov	ecx,8
      months_correction:
	mov	eax,ecx
	shr	eax,1
	add	ebp,eax
	cmp	ecx,2
	jb	months_correction_ok
	sub	ebp,2
      months_correction_ok:
	cmp	ecx,2
	jbe	day_correction_ok
	pop	cx
	test	ecx,11b
	jnz	day_correction_ok
	xor	edx,edx
	mov	eax,ecx
	mov	ebx,100
	div	ebx
	or	edx,edx
	jnz	day_correction
	mov	eax,ecx
	mov	ebx,400
	div	ebx
	or	edx,edx
	jnz	day_correction_ok
      day_correction:
	inc	ebp
      day_correction_ok:
	pop	dx
	movzx	eax,dl
	dec	eax
	add	eax,ebp
	mov	ebx,24
	mul	ebx
	push	eax
	mov	ah,2Ch
	int	21h
	pop	eax
	push	dx
	movzx	ebx,ch
	add	eax,ebx
	mov	ebx,60
	mul	ebx
	movzx	ebx,cl
	add	eax,ebx
	mov	ebx,60
	mul	ebx
	pop	dx
	movzx	ebx,dh
	add	eax,ebx
	ret

character db ?,0
bytes_count dd ?

error_prefix db 'error: ',24h
error_suffix db '.'
cr_lf db 0Dh,0Ah,24h
line_number_start db ' [',24h
line_data_start db ':',0Dh,0Ah,24h

macro dm string { db string,24h }
