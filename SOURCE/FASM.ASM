
; flat assembler project
; by privalov
; started at: 1999-03-23 14:24:33
; version 0.90 finished at: 1999-05-04 15:23:26
; version 1.00 finished at: 1999-07-01 21:38:14

HF
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
	mov	ax,4811h
	mov	edx,8000h
	mov	[relocations_memory_end],edx
	int	31h
	mov	[relocations_memory],edx
	add	[relocations_memory_end],edx
	mov	ax,4811h
	mov	edx,8000h
	mov	[exports_memory_end],edx
	int	31h
	mov	[exports_memory],edx
	add	[exports_memory_end],edx
	mov	ax,4811h
	mov	edx,10000h
	mov	[imports_memory_end],edx
	int	31h
	mov	[imports_memory],edx
	mov	[imports_end],edx
	add	[imports_memory_end],edx
	mov	ax,4811h
	mov	edx,10000h
	mov	[import_relocations_memory_end],edx
	int	31h
	mov	[import_relocations_memory],edx
	add	[import_relocations_memory_end],edx
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
	push	ebx
	call	convert_imports
	pop	ebx
	or	[HF_attributes],1
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
	cmp	[program_relocatable],0
	je	HF_relocations_ok
	or	[HF_attributes],10b
	mov	eax,[relocations_end]
	sub	eax,[relocations_memory]
	mov	[relocations_size],eax
	mov	[relocations_size+4],eax
	or	eax,eax
	jz	HF_relocations_ok
	add	[HF_size],10h
	add	[HF_size],eax
      HF_relocations_ok:
	mov	eax,[exports_end]
	sub	eax,[exports_memory]
	mov	[exports_size],eax
	mov	[exports_size+4],eax
	or	eax,eax
	jz	HF_exports_ok
	add	[HF_size],10h
	add	[HF_size],eax
      HF_exports_ok:
	mov	eax,[imports_end]
	sub	eax,[imports_memory]
	mov	[imports_size],eax
	mov	[imports_size+4],eax
	or	eax,eax
	jz	HF_imports_ok
	add	[HF_size],10h
	add	[HF_size],eax
      HF_imports_ok:
	mov	eax,[entry_point]
	mov	[HF_initial_EIP],eax
	mov	[HF_initial_ESP],0
	mov	ah,40h
	int	21h
	mov	edx,[code_start]
	mov	ecx,[code_size]
	mov	ah,40h
	int	21h
	cmp	[program_relocatable],0
	je	relocations_saved
	cmp	[relocations_size],0
	je	relocations_saved
	mov	edx,relocations_header
	mov	ecx,10h
	mov	ah,40h
	int	21h
	mov	edx,[relocations_memory]
	mov	ecx,[relocations_size]
	mov	ah,40h
	int	21h
      relocations_saved:
	cmp	[exports_size],0
	je	exports_saved
	mov	edx,exports_header
	mov	ecx,10h
	mov	ah,40h
	int	21h
	mov	edx,[exports_memory]
	mov	ecx,[exports_size]
	mov	ah,40h
	int	21h
      exports_saved:
	cmp	[imports_size],0
	je	exit
	mov	edx,imports_header
	mov	ecx,10h
	mov	ah,40h
	int	21h
	mov	edx,[imports_memory]
	mov	ecx,[imports_size]
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

macro skip_string
 {
	local	_loop
      _loop:
	lodsb
	or	al,al
	jnz	_loop
 }

convert_imports:
	mov	edi,[imports_end]
	mov	esi,[imports_memory]
      find_DLL:
	cmp	esi,[imports_end]
	jae	imports_converted
	cmp	byte [esi],0Dh
	je	next_import_entry
	mov	ebx,edi
	mov	[value],edi
	jmp	copy_DLL_name
      next_import_entry:
	skip_string
	skip_string
	jmp	find_DLL
      copy_DLL_name:
	lodsb
	stosb
	or	al,al
	jnz	copy_DLL_name
	mov	[temp_dword],edi
	mov	ebp,edi
	mov	esi,ebx
	mov	ebx,edi
	sub	ebx,esi
	mov	edi,[imports_memory]
	mov	[import_number],0
      find_import_for_current_DLL:
	cmp	edi,[imports_end]
	jae	current_DLL_ok
	mov	edx,edi
	mov	ecx,ebx
	push	esi
	repe	cmpsb
	jne	next_import
	mov	byte [edx],0Dh
	mov	esi,edi
	mov	edi,ebp
      copy_import_name:
	lodsb
	stosb
	or	al,al
	jnz	copy_import_name
	push	ebx ebp
	mov	ebp,edi
	mov	ecx,[import_number]
	mov	ebx,[import_relocations_memory]
	add	edi,4
      copy_import_relocations:
	cmp	ebx,[import_relocations_end]
	je	import_relocations_ok
	cmp	dword [ebx+4],ecx
	jne	next_import_relocation
	mov	eax,[ebx]
	stosd
      next_import_relocation:
	add	ebx,8
	jmp	copy_import_relocations
      import_relocations_ok:
	mov	eax,edi
	sub	eax,ebp
	shr	eax,2
	dec	eax
	mov	[ds:ebp],eax
	pop	ebp ebx
	or	eax,eax
	jz	import_converted
	mov	ebp,edi
      import_converted:
	mov	edi,esi
	pop	esi
	inc	[import_number]
	jmp	find_import_for_current_DLL
      next_import:
	pop	esi
	xchg	esi,edi
	skip_string
	skip_string
	xchg	esi,edi
	inc	[import_number]
	jmp	find_import_for_current_DLL
      current_DLL_ok:
	mov	esi,[imports_memory]
	mov	edi,ebp
	cmp	edi,[temp_dword]
	je	empty_DLL
	xor	al,al
	stosb
	jmp	find_DLL
      empty_DLL:
	mov	edi,[value]
	jmp	find_DLL
      imports_converted:
	mov	eax,[imports_end]
	mov	[imports_memory],eax
	mov	[imports_end],edi
	ret

_information db 'flat assembler  version 1.00',0Dh,0Ah
	     db 'copyright (c) 1999-2000 by privalov',0Dh,0Ah
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

relocations_header:
  dd 'relt',0
  relocations_size dd 0,0

exports_header:
  dd 'expt',0
  exports_size dd 0,0

imports_header:
  dd 'impt',0
  imports_size dd 0,0

input_file rd 1
output_file rd 1

memory_start dd 0
memory_end dd 0
additional_memory dd 0
additional_memory_end dd 0
relocations_memory dd 0
relocations_memory_end dd 0
relocations_end dd 0
exports_memory dd 0
exports_memory_end dd 0
exports_end dd 0
imports_memory dd 0
imports_memory_end dd 0
imports_end dd 0
import_relocations_memory dd 0
import_relocations_memory_end dd 0
import_relocations_end dd 0
source_start dd 0
code_start dd 0
code_size dd 0

params rb 100h
switches rb 100h
