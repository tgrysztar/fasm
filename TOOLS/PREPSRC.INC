
preprocessed_source:
	mov	edx,[input_file]
	call	open
	jc	input_not_found
	mov	al,2
	xor	edx,edx
	call	lseek
	cmp	eax,30h
	jb	invalid_input
	push	eax
	call	alloc
	jc	not_enough_memory
	push	eax
	xor	al,al
	xor	edx,edx
	call	lseek
	mov	ecx,[esp+4]
	mov	edx,[esp]
	call	read
	jc	reading_error
	pop	eax ecx
	cmp	dword [eax],1A736166h
	jne	invalid_input
	mov	esi,[eax+32]
	add	esi,eax
	mov	ebp,[eax+36]
	add	ebp,esi
	mov	edi,eax
	push	eax
      preprocessed_to_text:
	cmp	esi,ebp
	jae	conversion_done
	add	esi,16
	xor	dl,dl
      convert_preprocessed_line:
	lodsb
	cmp	al,1Ah
	je	copy_symbol
	cmp	al,22h
	je	copy_symbol
	cmp	al,3Bh
	je	preprocessor_symbols
	or	al,al
	jz	line_converted
	stosb
	xor	dl,dl
	jmp	convert_preprocessed_line
      copy_symbol:
	or	dl,dl
	jz	space_ok
	mov	byte [edi],20h
	inc	edi
      space_ok:
	cmp	al,22h
	je	quoted
	lodsb
	movzx	ecx,al
	rep	movsb
	or	dl,-1
	jmp	convert_preprocessed_line
      quoted:
	mov	al,27h
	stosb
	lodsd
	mov	ecx,eax
	jecxz	quoted_copied
      copy_quoted:
	lodsb
	stosb
	cmp	al,27h
	jne	quote_ok
	stosb
      quote_ok:
	loop	copy_quoted
      quoted_copied:
	mov	al,27h
	stosb
	or	dl,-1
	jmp	convert_preprocessed_line
      preprocessor_symbols:
	mov	al,3Bh
	stosb
	jmp	copy_symbol
      line_converted:
	mov	ax,0A0Dh
	stosw
	jmp	preprocessed_to_text
      conversion_done:
	mov	edx,[output_file]
	call	create
	jc	writing_error
	pop	edx
	mov	ecx,edi
	sub	ecx,edx
	call	write
	jc	writing_error
	call	close
	ret

not_enough_memory:
	call	error
	db	'not enough memory to load the required data',0
input_not_found:
	call	error
	db	'the input file was not found',0
reading_error:
	call	error
	db	'some error occured while trying to read file',0
writing_error:
	call	error
	db	'some error occured while trying to write file',0
invalid_input:
	call	error
	db	'input file is not a recognized assembly information format',0
