// RUN: %ucc -o %t %s -g
// RUN: %t
// RUN: echo ptype struct A | gdb %t | grep 'int i;'

main()
{
	return (struct A { int i, j; }){ .i = 5 }.j;
}
