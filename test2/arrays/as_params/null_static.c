// RUN: %ucc -c %s
// RUN: %check %s

int f(int [static 2]);
int g(int [static]);

main()
{
	f((void *)0); // CHECK: /warning: passing null-pointer where array expected/
}
