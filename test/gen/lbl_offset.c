// RUN: %ucc -fno-pic -S -o- %s | grep 'i+80'

int i;

void f(int *);

main()
{
	f(&i + 20);
}
