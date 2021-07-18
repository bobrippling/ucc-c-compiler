// RUN: %check -e %s

main()
{
	int x = 5;
	int vla[x];

	vla++; // CHECK: error: compound assignment to int[vla] - arrays not assignable
}
