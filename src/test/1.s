extern printd
global tim
tim:
push rbp
mov rbp, rsp
mov rax, [rbp + 32] ; a
push rax
mov rax, [rbp + 24] ; b
push rax
mov rax, [rbp + 16] ; c
push rax
pop rbx
pop rax
add rax, rbx
push rax
pop rbx
pop rax
add rax, rbx
push rax
call printd
add rsp, 8 ; 1 args
leave
ret
global main
main:
push rbp
mov rbp, rsp
sub rsp, 24
mov rax, 4
push rax
pop rax
mov [rbp - 24], rax ; i
mov rax, 2
push rax
pop rax
mov [rbp - 16], rax ; j
mov rax, 3
push rax
pop rax
mov [rbp - 8], rax ; k
mov rax, [rbp - 24] ; i
push rax
mov rax, [rbp - 16] ; j
push rax
mov rax, [rbp - 8] ; k
push rax
call tim
add rsp, 24 ; 3 args
add rsp, 24
leave
ret
