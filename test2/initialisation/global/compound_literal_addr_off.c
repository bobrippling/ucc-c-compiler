// RUN: %ocheck 6 %s

int *p = &(int[]){1,2,3}[1];

main()
{
	return p[-1] + p[0] + p[1];
}
