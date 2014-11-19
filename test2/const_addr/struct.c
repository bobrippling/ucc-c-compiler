// RUN: %ocheck 1 %s

struct RT
{
  char A; // 1
  int B[10][20]; // 20*10*4 = 800
  char C; // 1
}; // sz = 1 + 3 + 800 + 1 + 3 = 808

struct ST
{
  int X; // 4
  double Y; // 8
  struct RT Z; // 808
}; // sz = 4 + 4 + 8 + 808 = 824

int *f(struct ST *s)
{
  return &s[1].Z.B[5][13];

	/* s[1] = 824
	 * .Z   = 824 + (4+4+8) = 840
	 * .B   = 840 + 4 = 844
	 * [5]  = 844 + (20*4)*5 = 844+400 = 1244
	 * [13] = 1244 + 4*13 = 1244 + 52 = 1296
	 */
}

main()
{
	return f((void *)0) == 1296;
}
