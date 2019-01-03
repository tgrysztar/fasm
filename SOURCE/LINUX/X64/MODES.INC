
; flat assembler interface for Linux x64
; Copyright (c) 1999-2019, Tomasz Grysztar.
; All rights reserved.

esp equ +rsp

macro pushD [arg]
{
	common
		local offset,total
		offset = 0
		lea rsp,[rsp-total]
	forward
		offset = offset + 4
		if arg eqtype eax
			mov dword [rsp+total-offset],arg
		else
			mov r8d,dword arg
			mov [rsp+total-offset],r8d
		end if
	common
		total = offset
}

macro popD [arg]
{
	common
		local offset
		offset = 0
	forward
		if arg eqtype [mem]
			mov r8d,[rsp+offset]
			mov dword arg,r8d
		else
			mov arg,dword [rsp+offset]
		end if
		offset = offset + 4
	common
		lea rsp,[rsp+offset]
}

macro add dest,src
{
	if dest eq esp
		add rsp,src
	else
		add dest,src
	end if
}

macro mov dest,src
{
	if src eq esp
		mov dest,ESP
	else
		mov dest,src
	end if
}

macro cmp dest,src
{
	if dest eq esp
		cmp ESP,src
	else
		cmp dest,src
	end if
}

macro use32
{

	macro push args
	\{
		local list,arg,status
		define list
		define arg
		irps sym, args \\{
			define status
			match =dword, sym \\\{
				define status :
			\\\}
			match [any, status arg sym \\\{
				define arg [any
				match [mem], arg \\\\{
					match previous, list \\\\\{ define list previous,[mem] \\\\\}
					match , list \\\\\{ define list [mem] \\\\\}
					define arg
				\\\\}
				define status :
			\\\}
			match [, status arg sym \\\{
				define arg [
				define status :
			\\\}
			match , status \\\{
				match previous, list \\\\{ define list previous,sym \\\\}
				match , list \\\\{ define list sym \\\\}
			\\\}
		\\}
		match arg, list \\{ pushD arg \\}
	\}

	macro pop args
	\{
		local list,arg,status
		define list
		define arg
		irps sym, args \\{
			define status
			match =dword, sym \\\{
				define status :
			\\\}
			match [any, status arg sym \\\{
				define arg [any
				match [mem], arg \\\\{
					match previous, list \\\\\{ define list previous,[mem] \\\\\}
					match , list \\\\\{ define list [mem] \\\\\}
					define arg
				\\\\}
				define status :
			\\\}
			match [, status arg sym \\\{
				define arg [
				define status :
			\\\}
			match , status \\\{
				match previous, list \\\\{ define list previous,sym \\\\}
				match , list \\\\{ define list sym \\\\}
			\\\}
		\\}
		match arg, list \\{ popD arg \\}
	\}

	macro jmp arg
	\{
		if arg eq near eax
			jmp near rax
		else if arg eq near edx
			jmp near rdx
		else if arg eqtype [mem]
			mov r8d,arg
			jmp near r8
		else
			jmp arg
		end if
	\}

	macro call arg
	\{
		if 1
		match =near =dword [mem], arg \\{
			mov r8d,[mem]
			call near r8
		else
		\\}
			call arg
		end if
	\}

	macro salc		; for fasm's core it does not need to preserve flags
	\{
		setc al
		neg al
	\}

	macro jcxz target	; for fasm's core it does not need to preserve flags
	\{
		test cx,cx
		jz target
	\}

	use64

}

macro use16
{

	purge push,pop,jmp,call,salc,jcxz

	use16

}

use32
