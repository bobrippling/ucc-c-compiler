// RUN: %ucc -target x86_64-linux -S -o- %s | %stdoutcheck %s

// STDOUT: /movq %rbx, -[0-9]+\(%rbp\)/
// STDOUT: /movq %r12, -[0-9]+\(%rbp\)/
// STDOUT: /movq %r13, -[0-9]+\(%rbp\)/
// STDOUT: /movq -[0-9]+\(%rbp\), %rbx/
// STDOUT: /movq -[0-9]+\(%rbp\), %r12/
// STDOUT: /movq -[0-9]+\(%rbp\), %r13/

typedef int fn(void);

fn a, b, c, d;

void f(int, int, int, int);

main()
{
	f(a(), b(), c(), d());
}
