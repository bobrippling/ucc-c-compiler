// RUN: %ocheck 0 %s

int x[2][2][4];

main()
{
	return x[1][1][3]; // only one dereference
}
