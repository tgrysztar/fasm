
macro ccall proc,[arg]
  { common
     push ebp
     mov ebp,esp
     local size
     size = 0
     if ~ arg eq
    forward
     size = size + 4
    common
     sub esp,size
     end if
     and esp,-16
     if ~ arg eq
     add esp,size
    reverse
     pushd arg
    common
     end if
     call proc
     leave }

