
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
alloc:
	push	ebx esi edi
	mov	cx,ax
	shr	eax,16
	mov	bx,ax
	mov	ax,501h
	int	31h
	jc	dpmi_allocation_failed
	mov	ax,bx
	shl	eax,16
	mov	ax,cx
	mov	edx,main
	shl	edx,4
	sub	eax,edx
	mov	bx,si
	shl	ebx,16
	mov	bx,di
	mov	ecx,[memory_handles_count]
	inc	[memory_handles_count]
	shl	ecx,3
	add	ecx,memory_handles
	mov	[ecx],eax
	mov	[ecx+4],ebx
	pop	edi esi ebx
	clc
	ret
    dpmi_allocation_failed:
	pop	edi esi ebx
	stc
	ret
free:
	push	ebx esi edi
	mov	esi,memory_handles
	mov	ecx,[memory_handles_count]
    find_memory_handle:
	cmp	eax,[esi]
	je	memory_handle_found
	add	esi,8
	loop	find_memory_handle
	pop	edi esi
	ret
    memory_handle_found:
	mov	ebx,[esi+4]
	dec	[memory_handles_count]
	dec	ecx
	jz	free_memory
    remove_memory_handle:
	mov	edx,[esi+8]
	mov	edi,[esi+8+4]
	mov	[esi],edx
	mov	[esi+4],edi
	add	esi,8
	loop	remove_memory_handle
    free_memory:
	mov	esi,ebx
	shr	esi,16
	mov	di,bx
	mov	ax,502h
	int	31h
	pop	edi esi ebx
	ret
open:
	push	esi edi ebp
	call	adapt_path
	mov	ax,716Ch
	mov	bx,100000b
	mov	dx,1
	xor	cx,cx
	xor	si,si
	call	dos_int
	jnc	open_done
	cmp	ax,7100h
	je	old_open
	stc
	jmp	open_done
      old_open:
	mov	ax,3D00h
	xor	dx,dx
	call	dos_int
      open_done:
	mov	bx,ax
	pop	ebp edi esi
	ret
    adapt_path:
	mov	esi,edx
	mov	edi,buffer
      copy_path:
	lodsb
	cmp	al,'/'
	jne	path_char_ok
	mov	al,'\'
      path_char_ok:
	stosb
	or	al,al
	jnz	copy_path
	ret
    dos_int:
	push	0 0 0
	pushw	buffer_segment buffer_segment
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
create:
	push	esi edi ebp
	call	adapt_path
	mov	ax,716Ch
	mov	bx,100001b
	mov	dx,10010b
	xor	cx,cx
	xor	si,si
	xor	di,di
	call	dos_int
	jnc	create_done
	cmp	ax,7100h
	je	old_create
	stc
	jmp	create_done
    old_create:
	mov	ah,3Ch
	xor	cx,cx
	xor	dx,dx
	call	dos_int
    create_done:
	mov	bx,ax
	pop	ebp edi esi
	ret
write:
	push	edx esi edi ebp
	mov	ebp,ecx
	mov	esi,edx
      write_loop:
	mov	ecx,1000h
	sub	ebp,1000h
	jnc	do_write
	add	ebp,1000h
	mov	ecx,ebp
	xor	ebp,ebp
      do_write:
	push	ecx
	mov	edi,buffer
	shr	ecx,2
	rep	movsd
	mov	ecx,[esp]
	and	ecx,11b
	rep	movsb
	pop	ecx
	mov	ah,40h
	xor	dx,dx
	call	dos_int
	or	ebp,ebp
	jnz	write_loop
	pop	ebp edi esi edx
	ret
read:
	push	edx esi edi ebp
	mov	ebp,ecx
	mov	edi,edx
      read_loop:
	mov	ecx,1000h
	sub	ebp,1000h
	jnc	do_read
	add	ebp,1000h
	mov	ecx,ebp
	xor	ebp,ebp
      do_read:
	push	ecx
	mov	ah,3Fh
	xor	dx,dx
	call	dos_int
	cmp	ax,cx
	jne	eof
	mov	esi,buffer
	mov	ecx,[esp]
	shr	ecx,2
	rep	movsd
	pop	ecx
	and	ecx,11b
	rep	movsb
	or	ebp,ebp
	jnz	read_loop
      read_done:
	pop	ebp edi esi edx
	ret
      eof:
	pop	ecx
	stc
	jmp	read_done
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
