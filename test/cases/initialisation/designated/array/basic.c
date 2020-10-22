// RUN: %ucc -o %t %s
// RUN: %ocheck 2 %t

int x[] = {
	[5] = 2
};

main()
{
	return
		x[0] +
		x[1] +
		x[2] +
		x[3] +
		x[4] +
		x[5];
}
