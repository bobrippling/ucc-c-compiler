// RUN: %ucc %s
// RUN: [ `%ucc -S -o- %s | grep 'subq' | awk '{print $2}' | grep -o '[0-9]\+'` -ge 48 ]

main()
{
	int a[4];
	typeof(a) b;
	typeof(int[4]) c;
}
