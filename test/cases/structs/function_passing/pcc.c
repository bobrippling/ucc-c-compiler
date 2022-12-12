// RUN: %ucc -S -o- %s -target x86_64-linux                     | %stdoutcheck %s
// RUN: %ucc -S -o- %s -target x86_64-linux -fpcc-struct-return | %stdoutcheck --prefix=pcc %s

typedef struct A { int x; } A;

A f(){
	// STDOUT: f:
	// STDOUT: /movl .*%rbp\), %eax/
	// STDOUT: ret

	// STDOUT-pcc: f:
	// STDOUT-pcc: /movq -8\(%rbp\), %rax/
	//                   ^~ ok/stable - stret ptr
	// STDOUT-pcc: ret
	return (A){ 3 };
}

int main(){
	// STDOUT: main:
	// STDOUT: call
	// STDOUT: /movl %eax, .*%rbp/
	// STDOUT: ret

	// STDOUT-pcc main:
	// STDOUT-pcc call
	// STDOUT-NOT-pcc: /mov/
	// STDOUT-pcc movl (%rax), %eax
	// STDOUT-NOT-pcc: /mov/
	// STDOUT-pcc ret
	return f().x;
}
