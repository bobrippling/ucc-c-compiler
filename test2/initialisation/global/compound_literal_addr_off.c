// RUN: %ucc -o %t %s
// RUN: %ocheck 6 %t

int *p = &(int[]){1,2,3}[1];

main()
{
	return p[-1] + p[0] + p[1];
}
