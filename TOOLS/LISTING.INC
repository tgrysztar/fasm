
listing:
	mov	edx,[input_file]
	call	open
	jc	input_not_found
	call	load_file
	mov	[input],eax
	cmp	ecx,38h
	jb	invalid_input
	cmp	dword [eax],1A736166h
	jne	invalid_input
	cmp	dword [eax+44],0
	je	incomplete_input
	add	[eax+16],eax
	add	[eax+24],eax
	add	[eax+32],eax
	add	[eax+40],eax
	add	[eax+48],eax
	mov	edx,[eax+16]
	add	[eax+8],edx
	add	[eax+12],edx
	mov	edx,[eax+12]
	call	open
	jc	code_not_found
	call	load_file
	mov	[assembled_code],eax
	mov	[assembled_code_length],ecx
	call	close
	mov	[maximum_address_length],0
	mov	ebx,[input]
	mov	esi,[ebx+40]
	lea	ebp,[esi-4]
	add	ebp,[ebx+44]
    get_offsets_for_lines:
	cmp	esi,ebp
	je	offsets_prepared
	mov	edx,[esi+4]
	add	edx,[ebx+32]
      find_line_loaded_from_source:
	test	byte [edx+7],1 shl 7
	jz	store_offset_in_line
	mov	edx,[edx+8]
	add	edx,[ebx+32]
	jmp	find_line_loaded_from_source
      store_offset_in_line:
	cmp	dword [edx+12],0
	jne	get_next_offset
	mov	[edx+12],esi
	movzx	ecx,byte [esi+27]
	and	cl,1
	mov	edi,[esi+20]
	test	edi,edi
	jz	base_name_length_ok
	xor	ecx,ecx
	btr	edi,31
	jc	count_base_name_characters
	dec	edi
	shl	edi,2
	add	edi,[ebx+48]
	mov	edi,[edi]
      count_base_name_characters:
	mov	ecx,[ebx+20]
	sub	ecx,edi
	add	edi,[ebx+16]
	mov	edx,edi
	xor	al,al
	repne	scasb
	mov	ecx,edi
	sub	ecx,edx
      base_name_length_ok:
	cmp	byte [esi+18],1
	jb	first_register_length_ok
	ja	first_register_with_scale
	add	ecx,5
	jmp	first_register_length_ok
      first_register_with_scale:
	add	ecx,5+3
      first_register_length_ok:
	cmp	byte [esi+19],1
	jb	second_register_length_ok
	ja	second_register_with_scale
	add	ecx,5
	jmp	second_register_length_ok
      second_register_with_scale:
	add	ecx,5+3
      second_register_length_ok:
	cmp	ecx,[maximum_address_length]
	jb	get_next_offset
	mov	[maximum_address_length],ecx
      get_next_offset:
	add	esi,28
	jmp	get_offsets_for_lines
    offsets_prepared:
	mov	eax,[esi]
	mov	[code_end],eax
	add	[maximum_address_length],19
	mov	edi,characters
	xor	al,al
      make_characters_table:
	stosb
	inc	al
	jnz	make_characters_table
	mov	edi,characters
	mov	esi,symbol_characters+1
	movzx	ecx,byte [esi-1]
	xor	eax,eax
      mark_symbol_characters:
	lodsb
	mov	byte [edi+eax],0
	loop	mark_symbol_characters
	mov	eax,[code_bytes_per_line]
	imul	eax,3
	add	eax,[maximum_address_length]
	add	eax,18
	call	alloc
	jc	not_enough_memory
	mov	[output_buffer],eax
	mov	esi,[ebx+32]
	mov	ebp,esi
	add	ebp,[ebx+36]
	mov	edx,[output_file]
	call	create
	jc	writing_error
	mov	[output_handle],ebx
	xor	eax,eax
	mov	[current_source_file],eax
	mov	[last_listed_address],eax
	mov	[code_length],eax
    generate_listing:
	cmp	esi,ebp
	jae	listing_done
	mov	edi,[output_buffer]
	test	byte [esi+7],1 shl 7
	jnz	next_line
	mov	ebx,[esi+12]
	test	ebx,ebx
	jz	no_code_listing
	test	byte [ebx+26],11b
	jnz	no_code_listing
	push	esi
	mov	edx,[esi]
	mov	ecx,[esi+4]
     find_next_code_point:
	add	esi,16
	call	skip_preprocessed_line
	cmp	esi,ebp
	je	last_code_point
	cmp	edx,[esi]
	jne	next_line_ok
	cmp	ecx,[esi+4]
	je	find_next_code_point
     next_line_ok:
	test	byte [esi+7],1 shl 7
	jnz	find_next_code_point
	mov	eax,[esi+12]
	test	eax,eax
	jz	find_next_code_point
	test	byte [eax+26],11b
	jnz	find_next_code_point
	mov	eax,[eax]
	jmp	calculate_code_length
     last_code_point:
	mov	eax,[code_end]
     calculate_code_length:
	pop	esi
	mov	edx,[ebx]
	sub	eax,edx
	jz	no_code_listing
	mov	[code_length],eax
	mov	[code_offset],edx
	add	eax,edx
	cmp	eax,[assembled_code_length]
	jbe	write_file_offset
	mov	[code_length],0
      write_file_offset:
	call	write_hex_dword
	mov	ax,': '
	stosw
	call	list_address
	call	list_code
	jmp	code_listing_ok
      no_code_listing:
	mov	al,20h
	mov	ecx,8+2
	rep	stosb
	call	list_address
	mov	ecx,[code_bytes_per_line]
	imul	ecx,3
	mov	al,20h
	rep	stosb
      code_listing_ok:
	call	write_listing_data
	mov	eax,[input]
	mov	edx,[esi]
	test	edx,edx
	jz	main_source_file
	add	edx,[eax+32]
	jmp	source_name_ok
      main_source_file:
	mov	edx,[eax+8]
      source_name_ok:
	cmp	edx,[current_source_file]
	je	source_loaded
	push	ebx
	push	edx
	call	open
	jc	source_not_found
	pop	eax
	xchg	eax,[current_source_file]
	test	eax,eax
	jz	load_source
	mov	eax,[source]
	call	free
      load_source:
	call	load_file
	mov	[source],eax
	mov	[source_length],ecx
	call	close
	pop	ebx
      source_loaded:
	mov	eax,[source]
	add	eax,[esi+8]
	mov	[current_source_line],eax
	push	esi ebp
	call	write_source_line
	pop	ebp esi
      write_supplemental_rows:
	mov	eax,[code_length]
	or	eax,[current_source_line]
	jz	next_line
	mov	edi,[output_buffer]
	mov	ecx,8+2
	movzx	eax,[show_addresses]
	imul	eax,[maximum_address_length]
	add	ecx,eax
	mov	al,20h
	rep	stosb
	call	list_code
	call	write_listing_data
	push	esi ebp
	call	write_source_line
	pop	ebp esi
	jmp	write_supplemental_rows
      next_line:
	mov	edx,[esi]
	mov	ecx,[esi+4]
      find_next_line:
	add	esi,16
	call	skip_preprocessed_line
	cmp	edx,[esi]
	jne	generate_listing
	cmp	ecx,[esi+4]
	jne	generate_listing
	jmp	find_next_line
    list_address:
	cmp	[show_addresses],0
	je	address_ok
	mov	[address_start],edi
	mov	eax,[esi+12]
	test	eax,eax
	jz	address_finished
	cmp	[last_listed_address],0
	je	make_address
	push	esi edi
	lea	esi,[eax+8]
	mov	edi,[last_listed_address]
	mov	ecx,17
	repe	cmpsb
	pop	edi esi
	je	address_finished
      make_address:
	mov	ebx,[esi+12]
	lea	eax,[ebx+8]
	mov	[last_listed_address],eax
	mov	al,'['
	stosb
	mov	edx,[ebx+20]
	test	edx,edx
	jz	write_main_address
	push	esi
	mov	esi,edx
	mov	eax,[input]
	btr	esi,31
	jc	base_name_ready
	dec	esi
	shl	esi,2
	add	esi,[eax+48]
	mov	esi,[esi]
      base_name_ready:
	add	esi,[eax+16]
      copy_section_name:
	lodsb
	test	al,al
	jz	section_name_ok
	stosb
	jmp	copy_section_name
      section_name_ok:
	pop	esi
	mov	al,':'
	test	edx,80000000h
	jz	address_separator_ok
	cmp	byte [ebx+27],0
	jne	address_separator_ok
	mov	al,'+'
      address_separator_ok:
	stosb
      write_main_address:
	cmp	byte [ebx+27],0
	jne	write_negative_address
	mov	edx,[ebx+8+4]
	call	write_hex_dword
	mov	edx,[ebx+8]
	call	write_hex_dword
	jmp	write_address_registers
      write_negative_address:
	mov	al,'-'
	stosb
	mov	eax,[ebx+8]
	mov	edx,[ebx+8+4]
	not	eax
	not	edx
	add	eax,1
	adc	edx,0
	push	eax
	call	write_hex_dword
	pop	edx
	call	write_hex_dword
      write_address_registers:
	mov	dl,[ebx+16]
	mov	dh,[ebx+18]
	call	address_register
	mov	dl,[ebx+17]
	mov	dh,[ebx+19]
	call	address_register
	mov	ax,']'
	stosb
      address_finished:
	mov	ecx,[maximum_address_length]
	sub	ecx,edi
	add	ecx,[address_start]
	mov	al,20h
	rep	stosb
      address_ok:
	ret
      address_register:
	cmp	dh,0
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
	jz	register_found
	cmp	al,dl
	je	register_found
	cmp	dl,[esi]
	je	register_found
	lodsb
	movzx	eax,al
	add	esi,eax
	jmp	find_register
      register_found:
	lodsb
	movzx	ecx,al
	rep	movsb
	pop	esi
	cmp	dh,1
	je	register_ok
	mov	al,'*'
	stosb
	test	dh,0F0h
	jz	first_scale_digit_ok
	mov	al,dh
	shr	al,4
	cmp	al,10
	sbb	al,69h
	das
	stosb
      first_scale_digit_ok:
	mov	al,dh
	and	al,1111b
	cmp	al,10
	sbb	al,69h
	das
	stosb
      register_ok:
	ret
    list_code:
	mov	ecx,[code_length]
	cmp	ecx,[code_bytes_per_line]
	jb	code_bytes_count_ready
	mov	ecx,[code_bytes_per_line]
      code_bytes_count_ready:
	sub	[code_length],ecx
	mov	edx,[code_offset]
	add	[code_offset],ecx
	jecxz	code_bytes_ok
	push	ecx
	add	edx,[assembled_code]
      list_code_bytes:
	mov	al,[edx]
	and	al,1111b
	cmp	al,10
	sbb	al,69h
	das
	mov	ah,al
	mov	al,[edx]
	shr	al,4
	cmp	al,10
	sbb	al,69h
	das
	stosw
	mov	al,20h
	stosb
	inc	edx
	loop	list_code_bytes
	pop	ecx
      code_bytes_ok:
	neg	ecx
	add	ecx,[code_bytes_per_line]
	imul	ecx,3
	mov	al,20h
	rep	stosb
	ret
    write_listing_data:
	mov	ecx,[output_buffer]
	sub	ecx,edi
	and	ecx,111b
	mov	al,20h
	rep	stosb
	mov	edx,[output_buffer]
	mov	ecx,edi
	sub	ecx,edx
	mov	ebx,[output_handle]
	call	write
	jc	writing_error
	ret
    write_source_line:
	mov	esi,[current_source_line]
	test	esi,esi
	je	write_line_break
	mov	ebp,[source_length]
	add	ebp,[source]
	mov	ebx,characters
	xor	cl,cl
      start_cutting:
	xor	dl,dl
      cut_source_line:
	cmp	esi,ebp
	je	end_of_file
	lodsb
	cmp	al,0Dh
	je	cr_character
	cmp	al,0Ah
	je	lf_character
	cmp	al,1Ah
	je	end_of_line
	or	al,al
	jz	end_of_line
	cmp	dl,3Bh
	je	cut_source_line
	cmp	al,3Bh
	je	start_special_block
	cmp	dl,22h
	je	inside_string
	cmp	dl,27h
	je	inside_string
	cmp	al,'\'
	je	check_for_line_continuation
	xlatb
	test	al,al
	jz	start_cutting
	cmp	dl,0FFh
	je	cut_source_line
	cmp	al,22h
	je	start_special_block
	cmp	al,27h
	je	start_special_block
	mov	dl,0FFh
	jmp	cut_source_line
      start_special_block:
	mov	dl,al
	jmp	cut_source_line
      inside_string:
	cmp	al,dl
	jne	cut_source_line
	jmp	start_cutting
      check_for_line_continuation:
	or	cl,0FFh
	cmp	esi,ebp
	je	end_of_file
	mov	al,[esi]
	cmp	al,20h
	je	start_cutting
	cmp	al,0Dh
	je	start_cutting
	cmp	al,0Ah
	je	start_cutting
	cmp	al,3Bh
	je	start_cutting
	xor	cl,cl
	jmp	start_cutting
      cr_character:
	mov	edx,esi
	mov	word [line_break],0Dh
	cmp	esi,ebp
	je	line_with_break
	mov	al,[esi]
	cmp	al,0Ah
	jne	line_with_break
	inc	edx
	mov	[line_break+1],al
	jmp	line_with_break
      lf_character:
	mov	edx,esi
	mov	word [line_break],0Ah
	cmp	esi,ebp
	je	line_with_break
	mov	al,[esi]
	cmp	al,0Dh
	jne	line_with_break
	inc	edx
	mov	[line_break+1],al
      line_with_break:
	dec	esi
	jmp	write_line
      end_of_line:
	dec	esi
      end_of_file:
	mov	edx,esi
      write_line:
	cmp	cl,0FFh
	je	continued_line
	xor	edx,edx
      continued_line:
	xchg	edx,[current_source_line]
	mov	ecx,esi
	sub	ecx,edx
	mov	ebx,[output_handle]
	call	write
	jc	writing_error
      write_line_break:
	mov	edx,line_break
	mov	ecx,2
	cmp	[line_break+1],0
	jne	line_break_size_ok
	dec	ecx
      line_break_size_ok:
	call	write
	jc	writing_error
	ret
    listing_done:
	mov	ebx,[output_handle]
	call	close
	ret

load_file:
	push	ebx
	mov	al,2
	xor	edx,edx
	call	lseek
	test	eax,eax
	jz	empty_file
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
	pop	ebx
	ret
      empty_file:
	pop	ebx
	mov	ecx,eax
	ret

write_hex_dword:
	mov	ecx,8
      write_hex_digits:
	xor	al,al
	shld	eax,edx,4
	cmp	al,10
	sbb	al,69h
	das
	stosb
	shl	edx,4
	loop	write_hex_digits
	ret

skip_preprocessed_line:
	lods	byte [esi]
	cmp	al,1Ah
	je	skip_preprocessed_symbol
	cmp	al,3Bh
	je	skip_preprocessed_symbol
	cmp	al,22h
	je	skip_preprocessed_string
	or	al,al
	jnz	skip_preprocessed_line
	ret
      skip_preprocessed_symbol:
	lods	byte [esi]
	movzx	eax,al
	add	esi,eax
	jmp	skip_preprocessed_line
      skip_preprocessed_string:
	lods	dword [esi]
	add	esi,eax
	jmp	skip_preprocessed_line


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
incomplete_input:
	call	error
	db	'input file does not contain an assembly dump',0

symbol_characters db 27, 9,0Ah,0Dh,1Ah,20h,'+-/*=<>()[]{}:,|&~#`;\'

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
		  db 0F4h,3,'eip'
		  db 0F8h,3,'rip'
		  db 0,1,'?'
