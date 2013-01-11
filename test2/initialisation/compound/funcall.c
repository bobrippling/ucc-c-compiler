// RUN: [ `%ucc %s -S -o- | grep 'mov.*[123456789]' | wc -l` -eq 9 ]

void f(char *);

main()
{
	f((char[]){1, 2, 3, 4, 5, 6, 7, 8, 9});
}
