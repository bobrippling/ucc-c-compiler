// RUN: %ucc %s
// RUN: [ `%ucc -S -o- %s | grep 'subq'` | awk '{print $2}' | sed 's#.0x##'` -ge 48 ]

main()
{
	int a[4];
	typeof(a) b;
	typeof(int[4]) c;
}
