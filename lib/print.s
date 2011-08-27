extern write
global printd_rec
printd_rec:
push rbp
mov rbp, rsp
sub rsp, 16
mov rax, [rbp + 16] ; n
push rax
mov rax, 10
push rax
xor rdx,rdx
pop rbx
pop rax
idiv rbx
push rax
pop rax
mov [rbp - 16], rax ; d
mov rax, [rbp + 16] ; n
push rax
mov rax, 10
push rax
xor rdx,rdx
pop rbx
pop rax
idiv rbx
push rdx
pop rax
mov [rbp - 8], rax ; m
mov rax, [rbp - 16] ; d
push rax
pop rax
test rax, rax
jz else_1
mov rax, [rbp - 16] ; d
push rax
call printd_rec
add rsp, 8 ; 1 args
jmp fi_2
else_1:
fi_2:
mov rax, [rbp - 8] ; m
push rax
mov rax, 48
push rax
pop rbx
pop rax
add rax, rbx
push rax
pop rax
mov [rbp - 8], rax ; m
mov rax, 1
push rax
lea rax, [rbp - 8] ; &m
push rax
mov rax, 1
push rax
call write
add rsp, 24 ; 3 args
add rsp, 16
leave
ret
global printd
printd:
push rbp
mov rbp, rsp
sub rsp, 32
mov rax, [rbp + 16] ; n
push rax
mov rax, 0
push rax
pop rbx
pop rax
xor rcx,rcx
cmp rax,rbx
setl cl
push rcx
pop rax
test rax, rax
jz else_3
mov rax, [rbp + 16] ; n
push rax
pop rax
neg rax
push rax
pop rax
mov [rbp + 16], rax ; n
mov rax, 45
push rax
pop rax
mov [rbp - 8], rax ; neg
mov rax, 1
push rax
lea rax, [rbp - 8] ; &neg
push rax
mov rax, 1
push rax
call write
add rsp, 24 ; 3 args
jmp fi_4
else_3:
fi_4:
mov rax, [rbp + 16] ; n
push rax
call printd_rec
add rsp, 8 ; 1 args
mov rax, 10
push rax
pop rax
mov [rbp - 16], rax ; nl
mov rax, 1
push rax
lea rax, [rbp - 16] ; &nl
push rax
mov rax, 1
push rax
call write
add rsp, 24 ; 3 args
add rsp, 32
leave
ret
global printc
printc:
push rbp
mov rbp, rsp
sub rsp, 32
mov rax, 1
push rax
lea rax, [rbp + 16] ; &c
push rax
mov rax, 1
push rax
call write
add rsp, 24 ; 3 args
add rsp, 32
leave
ret
global printstr
printstr:
push rbp
mov rbp, rsp
sub rsp, 32
while_5:
mov rax, [rbp + 16] ; str
push rax
pop rax
movzx rax, byte [rax]
push rax
pop rax
test rax, rax
jz while_fin_6
mov rax, [rbp + 16] ; str
push rax
pop rax
movzx rax, byte [rax]
push rax
call printc
add rsp, 8 ; 1 args
mov rax, [rbp + 16] ; str
push rax
mov rax, 1
push rax
pop rbx
pop rax
add rax, rbx
push rax
pop rax
mov [rbp + 16], rax ; str
jmp while_5
while_fin_6:
add rsp, 32
leave
ret
