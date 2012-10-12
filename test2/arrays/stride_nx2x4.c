// RUN: %ucc %s
// RUN: [ `%ucc %s -S -o- | grep -c '(%r[a-d]x)'` -eq 1 ]

extern int x[][2][4];

main()
{
	return x[1][2][3]; // only one dereference
}
