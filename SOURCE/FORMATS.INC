
; flat assembler  version 1.0
; Copyright (c) 1999-2000, Tomasz Grysztar
; All rights reserved.

formatter:
	mov	[written_size],0
	mov	edx,[output_file]
	call	create
	call	write_header
	mov	edx,[code_start]
	mov	ecx,[code_size]
	add	[written_size],ecx
	call	write
	call	close
	ret

mark_relocation:
	cmp	[value_type],0
	je	relocation_ok
	cmp	[virtual_mode],0
	jne	relocation_ok
	cmp	[output_format],2
	je	mark_mz_relocation
      relocation_ok:
	ret

mark_mz_relocation:
	push	eax ebx edi
	movzx	ebx,[number_of_relocations]
	shl	ebx,2
	inc	[number_of_relocations]
	add	ebx,[conventional_memory]
	add	ebx,1Ch
	cmp	ebx,[conventional_memory_end]
	je	out_of_memory
	mov	eax,edi
	sub	eax,[code_start]
	mov	[ebx],eax
	pop	edi ebx eax
	ret

write_header:
	cmp	[output_format],2
	je	write_mz_header
	ret

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
	mov	edi,[conventional_memory]
	movzx	eax,[number_of_relocations]
	shl	eax,2
	add	eax,1Ch
	add	edi,eax
	mov	ecx,Fh
	add	eax,Fh
	and	eax,1111b
	sub	ecx,eax
	xor	al,al
	rep	stos byte [es:edi]
	sub	edi,[conventional_memory]
	mov	ecx,edi
	shr	edi,4
	mov	edx,[conventional_memory]
	mov	word [edx],'MZ' 	; signature
	mov	[edx+8],di		; header size in paragraphs
	mov	ax,[number_of_relocations]
	mov	[edx+6],ax		; number of relocation entries
	mov	eax,[code_size]
	add	eax,ecx
	mov	esi,eax
	and	eax,111111111b
	mov	[edx+2],ax		; number of bytes in last page
	shr	esi,9
	inc	si
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
	ret

number_of_relocations dw ?
heap_size dw ?
initial_cs dw ?
initial_ip dw ?
initial_ss dw ?
initial_sp dw ?
initial_ss_defined db ?

