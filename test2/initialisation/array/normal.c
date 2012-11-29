// RUN: %ucc -c %s
// RUN: [ `%ucc %s -S -o- | grep 'mov.*[12]' | wc -l` -eq 2 ]

main()
{
	int x[] = { 1, 2 };
}
