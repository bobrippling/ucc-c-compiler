// RUN: %ucc %s

f(int [][static 2]);

h(int [][static  0]);

fine(int [static]);
fine(int [static 2]);

main()
{
	int x[static 3];
	int y[const  3];
}
