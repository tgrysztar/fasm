
; flat assembler project
; by privalov
; started at: 1999-03-23 14:24:33
; version 0.90 finished at: 1999-05-04 15:23:26

HF
code 32
entry start

include 'tables.inc'
include 'expressions.inc'
include 'symbols.inc'
include 'errors.inc'
include 'preprocessor.inc'
include 'assembler.inc'

start:

	call	get_params
	cmp	[params],0
	je	information
	lea	eax,[params+1]
	mov	[input_file],eax
	movzx	ecx,byte [eax-1]
	add	eax,ecx
	cmp	byte [eax],0
	je	information
	inc	eax
	mov	[output_file],eax
	mov	ax,4811h
	mov	edx,2000h
	mov	[additional_memory_end],edx
	int	31h
	jc	allocation_error
	mov	[additional_memory],edx
	add	[additional_memory_end],edx
	mov	ax,4810h
	int	31h
	sub	edx,4000h
	jc	allocation_error
	mov	[memory_end],edx
	mov	ax,4811h
	int	31h
	jnc	allocation_ok
    allocation_error:
	jmp	out_of_memory
    allocation_ok:
	mov	[memory_start],edx
	add	[memory_end],edx
	mov	[source_start],edx
	mov	edx,[additional_memory]
	mov	[files_list],edx

	call	preprocessor

	call	assembler
	sub	edi,[code_start]
	mov	[code_size],edi

	mov	ah,3Ch
	xor	cx,cx
	mov	edx,[output_file]
	int	21h
	cmp	[program_format],48h
	je	save_HF_program
	jmp	save_raw_code
    save_HF_program:
	cmp	[entry_code_type],32
	jne	HF_code_type_ok
	or	[HF_attributes],1
      HF_code_type_ok:
	mov	eax,[code_size]
	mov	ecx,eax
	mov	edi,[code_start]
	add	edi,ecx
	cmp	[undefined_data_end],edi
	jne	create_HF_header
	sub	edi,[undefined_data_start]
	sub	eax,edi
      create_HF_header:
	mov	[code_size],eax
	mov	[HF_code_size],eax
	mov	[HF_code_size+4],eax
	mov	[HF_memory_needed],ecx
	mov	edx,HF_header
	mov	ecx,HF_header_end-HF_header
	add	eax,ecx
	sub	eax,10h
	mov	[HF_size],eax
	mov	eax,[entry_point]
	mov	[HF_initial_EIP],eax
	mov	[HF_initial_ESP],0
	mov	ah,40h
	int	21h
	mov	edx,[code_start]
	mov	ecx,[code_size]
	mov	ah,40h
	int	21h
	jmp	exit
    save_raw_code:
	mov	edx,[code_start]
	mov	ecx,[code_size]
	mov	ah,40h
	int	21h
    exit:
	mov	ax,4C00h
	int	21h

information:
	mov	edx,_information
	mov	ah,9
	int	21h
	mov	ax,4C00h
	int	21h

get_params:
	mov	ax,4801h
	int	31h
	add	esi,81h
	mov	edi,params
	mov	edx,switches
    find_param:
	lodsb
	cmp	al,20h
	je	find_param
	cmp	al,0Dh
	je	all_params
	or	al,al
	jz	all_params
	cmp	al,'/'
	je	get_switch
	cmp	al,'-'
	je	get_switch
    no_switch:
	inc	edi
	mov	ebx,edi
    move_param:
	stosb
	lodsb
	cmp	al,20h
	je	param_end
	cmp	al,0Dh
	je	param_end
	cmp	al,'/'
	je	param_end
	cmp	al,'-'
	je	param_end
	or	al,al
	jz	param_end
	jmp	move_param
    get_switch:
	cmp	byte [esi],20h
	je	no_switch
	cmp	byte [esi],0Dh
	je	no_switch
	cmp	byte [esi],0
	je	no_switch
	mov	ebx,edi
	mov	edi,edx
	inc	edi
    move_switch:
	lodsb
	stosb
	cmp	al,20h
	je	switch_end
	cmp	al,0Dh
	je	switch_end
	or	al,al
	jz	switch_end
	cmp	al,'/'
	je	switch_end
	cmp	al,'-'
	je	switch_end
	jmp	move_switch
    switch_end:
	dec	esi
	mov	byte [edi-1],0
	mov	eax,edi
	sub	eax,edx
	dec	eax
	xchg	edi,edx
	stosb
	mov	edi,ebx
	jmp	find_param
    param_end:
	dec	esi
	xor	al,al
	stosb
	mov	eax,edi
	sub	eax,ebx
	mov	[ebx-1],al
	jmp	find_param
    all_params:
	xor	al,al
	stosb
	mov	edi,edx
	stosb
	ret

_information db 'flat assembler  version 0.90',0Dh,0Ah
	     db 'copyright (c) 1999 by privalov',0Dh,0Ah
	     db 'usage: fasm source output',0Dh,0Ah
	     db 0

program_format db 0

HF_header:
  HF_signature db 'HF: Program',1Ah
  HF_size dd 0
  dd 'head',0,10h,10h
  HF_attributes dd 0
  HF_memory_needed dd 0
  HF_initial_EIP dd 0
  HF_initial_ESP dd 0
  dd 'code',0
  HF_code_size dd 0,0
HF_header_end:

input_file rd 1
output_file rd 1

memory_start dd 0
memory_end dd 0
additional_memory dd 0
additional_memory_end dd 0
source_start dd 0

params rb 100h
switches rb 100h
code_start dd 0
code_size dd 0
