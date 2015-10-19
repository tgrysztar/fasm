
symbols:
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
	mov	[input],eax
	add	[eax+16],eax
	add	[eax+24],eax
	add	[eax+32],eax
	add	[eax+48],eax
	mov	edx,[eax+16]
	add	[eax+8],edx

	mov	ebx,eax
	mov	eax,[ebx+20]
	add	eax,[ebx+36]
	cmp	eax,1000h
	ja	allocate_output_buffer
	mov	eax,1000h
    allocate_output_buffer:
	call	alloc
	jc	not_enough_memory
	mov	[output_buffer],eax

	mov	edx,[output_file]
	call	create
	jc	writing_error
	mov	[output_handle],ebx

	mov	ebx,[input]
	mov	edx,[ebx+24]
	mov	ebp,[ebx+28]
	add	ebp,edx
    dump_symbols:
	cmp	edx,ebp
	je	dump_done
	test	byte [edx+8],1
	jz	next_symbol
	test	byte [edx+9],4
	jnz	next_symbol
	mov	edi,[output_buffer]
	mov	esi,[edx+24]
	test	esi,esi
	jz	anonymous_symbol
	mov	ebx,[input]
	btr	esi,31
	jc	symbol_name_in_strings_table
	add	esi,[ebx+32]
	lodsb
	movzx	ecx,al
	rep	movsb
	jmp	symbol_name_ok
     symbol_name_in_strings_table:
	add	esi,[ebx+16]
	call	write_string
	jmp	symbol_name_ok
     anonymous_symbol:
	mov	al,'@'
	stosb
	stosb
     symbol_name_ok:
	test	byte [edx+9],2
	jnz	negative_value
	mov	eax,': 0x'
	stosd
	mov	eax,[edx+4]
	call	write_hex_dword
	mov	eax,[edx]
	call	write_hex_dword
	jmp	write_address_registers
     negative_value:
	mov	eax,': -0'
	stosd
	mov	al,'x'
	stosb
	mov	ecx,[edx]
	mov	eax,[edx+4]
	not	ecx
	not	eax
	add	ecx,1
	adc	eax,0
	push	ecx
	or	ecx,eax
	jnz	negative_value_ok
	mov	byte [edi],'1'
	inc	edi
      negative_value_ok:
	call	write_hex_dword
	pop	eax
	call	write_hex_dword
     write_address_registers:
	mov	bl,[edx+12]
	mov	bh,[edx+14]
	call	write_address_register
	mov	bl,[edx+13]
	mov	bh,[edx+15]
	call	write_address_register
	mov	bl,[edx+11]
	cmp	bl,0
	je	symbol_type_ok
	jl	negated_symbol
	mov	ax,', '
	stosw
	jmp	write_symbol_type
     negated_symbol:
	mov	ax,', '
	stosw
	mov	esi,_negated
	call	write_string
	neg	bl
     write_symbol_type:
	cmp	bl,1
	je	segment_type
	cmp	bl,5
	je	rva_type
	cmp	bl,6
	je	plt_type
	test	byte [edx+20+3],80h
	jnz	external
	mov	esi,_relocatable
	call	write_string
	cmp	dword [edx+20],0
	je	symbol_type_ok
	mov	esi,_in_section
	call	write_string
	jmp	write_symbol_base
     simple_relocatable:
	mov	esi,_relocatable
	call	write_string
	jmp	symbol_type_ok
     external:
	mov	esi,_relative_to_external
	call	write_string
	jmp	write_symbol_base
     segment_type:
	mov	esi,_relocatable_segment
	call	write_string
	jmp	symbol_type_ok
     rva_type:
	mov	esi,_rva
	call	write_string
	jmp	write_symbol_base
     plt_type:
	mov	esi,_plt
	call	write_string
     write_symbol_base:
	mov	esi,[edx+20]
	btr	esi,31
	jc	write_external_name
	dec	esi
	shl	esi,2
	mov	ebx,[input]
	add	esi,[ebx+48]
	mov	esi,[esi]
	add	esi,[ebx+16]
	call	write_string
	mov	al,'('
	stosb
	mov	eax,[edx+20]
	call	write_dec_number
	mov	al,')'
	stosb
	jmp	symbol_type_ok
     write_external_name:
	mov	ebx,[input]
	add	esi,[ebx+16]
	call	write_string
	jmp	symbol_type_ok
     symbol_type_ok:
	mov	esi,_defined
	call	write_string
	mov	ebx,[edx+28]
	mov	eax,[input]
	add	ebx,[eax+32]
	call	write_line_identifier
	mov	ax,0A0Dh
	stosw
	push	edx
	mov	ebx,[output_handle]
	mov	ecx,edi
	mov	edx,[output_buffer]
	sub	ecx,edx
	call	write
	pop	edx
    next_symbol:
	add	edx,32
	jmp	dump_symbols
    dump_done:
	mov	ebx,[output_handle]
	call	close
	ret

write_string:
	lodsb
	test	al,al
	jz	string_ok
	stosb
	jmp	write_string
      string_ok:
	ret
write_hex_dword:
	mov	ebx,eax
	mov	ecx,8
      write_hex_digits:
	xor	al,al
	shld	eax,ebx,4
	cmp	al,10
	sbb	al,69h
	das
	stosb
	shl	ebx,4
	loop	write_hex_digits
	ret
write_dec_number:
	push	ebx edx
	mov	ecx,1000000000
	xor	edx,edx
	xor	bl,bl
      dec_number_loop:
	div	ecx
	push	edx
	cmp	ecx,1
	je	write_dec_digit
	or	bl,bl
	jnz	write_dec_digit
	or	al,al
	jz	dec_digit_ok
	not	bl
      write_dec_digit:
	add	al,30h
	stosb
      dec_digit_ok:
	mov	eax,ecx
	xor	edx,edx
	mov	ecx,10
	div	ecx
	mov	ecx,eax
	pop	eax
	or	ecx,ecx
	jnz	dec_number_loop
	pop	edx ebx
	ret
write_address_register:
	cmp	bh,0
	je	register_ok
	jl	negative_register
	mov	al,'+'
	jmp	register_sign_ok
      negative_register:
	mov	al,'-'
      register_sign_ok:
	stosb
	push	esi
	mov	esi,address_registers
      find_register:
	lodsb
	test	al,al
	jz	register_not_found
	cmp	al,bl
	je	register_found
	cmp	bl,[esi]
	je	register_found
	lodsb
	movzx	eax,al
	add	esi,eax
	jmp	find_register
      register_not_found:
	mov	al,bl
	shr	al,5
	cmp	al,0Ch shr 1
	je	xmm_register
	cmp	al,0Eh shr 1
	je	ymm_register
	cmp	al,6 shr 1
	je	zmm_register
      register_found:
	lodsb
	movzx	ecx,al
	rep	movsb
      write_register_scale:
	pop	esi
	cmp	bh,1
	je	register_ok
	mov	al,'*'
	stosb
	movzx	eax,bh
	call	write_dec_number
      register_ok:
	ret
      zmm_register:
	mov	al,'z'
	jmp	vector_address_register
      ymm_register:
	mov	al,'y'
	jmp	vector_address_register
      xmm_register:
	mov	al,'x'
      vector_address_register:
	stosb
	mov	ax,'mm'
	stosw
	mov	al,bl
	and	eax,11111b
	call	write_dec_number
	jmp	write_register_scale

write_line_identifier:
	test	dword [ebx+4],80000000h
	jnz	identify_macro_generated_line
	mov	esi,[ebx]
	mov	eax,[input]
	test	esi,esi
	jz	main_file
	add	esi,[eax+32]
	jmp	file_name_ok
      main_file:
	mov	esi,[eax+8]
      file_name_ok:
	call	write_string
	mov	al,'['
	stosb
	mov	eax,[ebx+4]
	call	write_dec_number
	mov	al,']'
	stosb
	ret
      identify_macro_generated_line:
	mov	al,'{'
	stosb
	mov	esi,_generated_by
	call	write_string
	push	ebx
	mov	ebx,[ebx+8]
	mov	eax,[input]
	add	ebx,[eax+32]
	call	write_line_identifier
	pop	ebx
	mov	eax,[ebx+8]
	cmp	eax,[ebx+12]
	je	macro_generated_line_identifier_ok
	mov	esi,_from
	call	write_string
	push	ebx
	mov	ebx,[ebx+12]
	mov	eax,[input]
	add	ebx,[eax+32]
	call	write_line_identifier
	pop	ebx
      macro_generated_line_identifier_ok:
	mov	al,'}'
	stosb
	ret

not_enough_memory:
	call	error
	db	'not enough memory to load the required data',0
input_not_found:
	call	error
	db	'the input file was not found',0
code_not_found:
	call	error
	db	'the assembled file was not found',0
source_not_found:
	call	error
	db	'could not find some of the source files',0
reading_error:
	call	error
	db	'some error occured while trying to read file',0
writing_error:
	call	error
	db	'some error occured while trying to write file',0
invalid_input:
	call	error
	db	'input file is not a recognized assembly information format',0

address_registers db 23h,2,'bx'
		  db 25h,2,'bp'
		  db 26h,2,'si'
		  db 27h,2,'di'
		  db 40h,3,'eax'
		  db 41h,3,'ecx'
		  db 42h,3,'edx'
		  db 43h,3,'ebx'
		  db 44h,3,'esp'
		  db 45h,3,'ebp'
		  db 46h,3,'esi'
		  db 47h,3,'edi'
		  db 48h,3,'r8d'
		  db 49h,3,'r9d'
		  db 4Ah,4,'r10d'
		  db 4Bh,4,'r11d'
		  db 4Ch,4,'r12d'
		  db 4Dh,4,'r13d'
		  db 4Eh,4,'r14d'
		  db 4Fh,4,'r15d'
		  db 80h,3,'rax'
		  db 81h,3,'rcx'
		  db 82h,3,'rdx'
		  db 83h,3,'rbx'
		  db 84h,3,'rsp'
		  db 85h,3,'rbp'
		  db 86h,3,'rsi'
		  db 87h,3,'rdi'
		  db 88h,2,'r8'
		  db 89h,2,'r9'
		  db 8Ah,3,'r10'
		  db 8Bh,3,'r11'
		  db 8Ch,3,'r12'
		  db 8Dh,3,'r13'
		  db 8Eh,3,'r14'
		  db 8Fh,3,'r15'
		  db 94h,3,'eip'
		  db 98h,3,'rip'
		  db 0,1,'?'

_negated db 'negated ',0
_relocatable_segment db 'relocatable segment',0
_relocatable db 'relocatable',0
_in_section db ' in section ',0
_relative_to_external db 'relative to external ',0
_rva db 'relative to RVA or GOT-based offset of ',0
_plt db 'relative to address of PLT entry for ',0
_defined db ', defined in ',0
_generated_by db 'line generated by ',0
_from db ' from ',0
