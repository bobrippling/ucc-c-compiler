// RUN: %ucc -o %t %s
// RUN: %ocheck 4 %t

int x = (long)&(((struct { int i, j; } *)0)->j);

main()
{
	return x;
}
