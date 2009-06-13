
display_string:
	invoke	GetStdHandle,[display_handle]
	mov	edx,eax
	mov	edi,esi
	or	ecx,-1
	xor	al,al
	repne	scasb
	neg	ecx
	sub	ecx,2
	invoke	WriteFile,edx,esi,ecx,bytes_count,0
	retn
alloc:
	invoke	VirtualAlloc,0,eax,MEM_COMMIT,PAGE_READWRITE
	or	eax,eax
	jz	allocation_error
	clc
	retn
     allocation_error:
	stc
	retn
free:
	invoke	VirtualFree,eax,0,MEM_RELEASE
	retn
open:
	invoke	CreateFile,edx,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0
	cmp	eax,-1
	je	file_error
	mov	ebx,eax
	clc
	retn
    file_error:
	stc
	retn
create:
	invoke	CreateFile,edx,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0
	cmp	eax,-1
	je	file_error
	mov	ebx,eax
	clc
	retn
write:
	invoke	WriteFile,ebx,edx,ecx,bytes_count,0
	or	eax,eax
	jz	file_error
	clc
	retn
read:
	push	ecx
	invoke	ReadFile,ebx,edx,ecx,bytes_count,0
	pop	edx
	or	eax,eax
	jz	file_error
	cmp	edx,[bytes_count]
	jne	file_error
	clc
	retn
close:
	invoke	CloseHandle,ebx
	retn
lseek:
	movzx	eax,al
	invoke	SetFilePointer,ebx,edx,0,eax
	cmp	eax,-1
	je	file_error
	retn
