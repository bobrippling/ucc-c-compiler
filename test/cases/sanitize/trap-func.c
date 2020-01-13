// RUN: %ucc -S -o- %s -fsanitize=undefined -ftrap-function=TRAP_FN -fsanitize-undefined-trap-on-error -target x86_64-linux | grep 'callq TRAP_FN@PLT'

void g(int i);

void f(int i)
{
	g(1 << i);
}
