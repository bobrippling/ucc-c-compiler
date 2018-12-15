// RUN: %ucc -fno-pic -S -o- %s | grep 'i+80'

int i;

main()
{
	f(&i + 20);
}
