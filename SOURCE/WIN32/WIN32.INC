
; flat assembler  version 1.04
; Copyright (c) 1999-2000, Tomasz Grysztar
; All rights reserved.

CREATE_NEW	       =   1
CREATE_ALWAYS	       =   2
OPEN_EXISTING	       =   3
OPEN_ALWAYS	       =   4
TRUNCATE_EXISTING      =   5

GENERIC_READ	       =   80000000h
GENERIC_WRITE	       =   40000000h

STD_INPUT_HANDLE       =   0FFFFFFF6h
STD_OUTPUT_HANDLE      =   0FFFFFFF5h
STD_ERROR_HANDLE       =   0FFFFFFF4h

MEM_COMMIT	       =   1000h
MEM_RESERVE	       =   2000h
MEM_DECOMMIT	       =   4000h
MEM_RELEASE	       =   8000h
MEM_FREE	       =   10000h
MEM_PRIVATE	       =   20000h
MEM_MAPPED	       =   40000h
MEM_RESET	       =   80000h
MEM_TOP_DOWN	       =   100000h

PAGE_NOACCESS	       =   1
PAGE_READONLY	       =   2
PAGE_READWRITE	       =   4
PAGE_WRITECOPY	       =   8
PAGE_EXECUTE	       =   10h
PAGE_EXECUTE_READ      =   20h
PAGE_EXECUTE_READWRITE =   40h
PAGE_EXECUTE_WRITECOPY =   80h
PAGE_GUARD	       =   100h
PAGE_NOCACHE	       =   200h
