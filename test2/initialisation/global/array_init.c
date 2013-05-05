// RUN: %ucc -o %t %s
// RUN: %ocheck 1 %t

int x[4] = { 1 };

main()
{
	return x[0] + x[1] + x[2] + x[3];
}
