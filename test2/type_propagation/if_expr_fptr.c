// RUN: %ucc %s
main()
{
	typeof(0 ? (void *)0 : (int(*)())0) x;
	x();
}
