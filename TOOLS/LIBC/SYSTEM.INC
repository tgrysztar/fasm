
extrn malloc
extrn getenv
extrn fopen
extrn fclose
extrn fread
extrn fwrite
extrn fseek
extrn ftell
extrn time
extrn exit
extrn 'free' as libc_free
extrn 'write' as libc_write

alloc:
	ccall	malloc,eax
	test	eax,eax
	jz	allocation_failed
	clc
	ret
      allocation_failed:
	stc
	ret
free:
	ccall	libc_free,eax
	ret
display_string:
	lodsb
	or	al,al
	jz	string_displayed
	mov	dl,al
	call	display_character
	jmp	display_string
      string_displayed:
	ret
    display_character:
	mov	[character],dl
	ccall	libc_write,[display_handle],character,1
	ret
open:
	push	esi edi ebp
	call	adapt_path
	ccall	fopen,buffer,open_mode
	pop	ebp edi esi
	or	eax,eax
	jz	file_error
	mov	ebx,eax
	clc
	ret
    adapt_path:
	mov	esi,edx
	mov	edi,buffer
      copy_path:
	lods	byte [esi]
	cmp	al,'\'
	jne	path_char_ok
	mov	al,'/'
      path_char_ok:
	stos	byte [edi]
	or	al,al
	jnz	copy_path
	cmp	edi,buffer+1000h
	ja	not_enough_memory
	ret
create:
	push	esi edi ebp
	call	adapt_path
	ccall	fopen,buffer,create_mode
	pop	ebp edi esi
	or	eax,eax
	jz	file_error
	mov	ebx,eax
	clc
	ret
close:
	ccall	fclose,ebx
	ret
read:
	push	ebx ecx edx esi edi
	ccall	fread,edx,1,ecx,ebx
	pop	edi esi edx ecx ebx
	cmp	eax,ecx
	jne	file_error
	clc
	ret
    file_error:
	stc
	ret
write:
	push	ebx ecx edx esi edi
	ccall	fwrite,edx,1,ecx,ebx
	pop	edi esi edx ecx ebx
	cmp	eax,ecx
	jne	file_error
	clc
	ret
lseek:
	push	ebx
	movzx	eax,al
	ccall	fseek,ebx,edx,eax
	mov	ebx,[esp]
	ccall	ftell,ebx
	pop	ebx
	ret

open_mode db 'r',0
create_mode db 'w',0
