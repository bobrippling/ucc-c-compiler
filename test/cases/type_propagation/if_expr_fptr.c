// RUN: %ucc %s
main()
{
	__typeof(0 ? (void *)0 : (int(*)())0) x;
	x();
}
