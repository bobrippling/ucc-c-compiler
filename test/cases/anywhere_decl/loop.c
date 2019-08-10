// RUN: %ucc %s -o %t
// RUN: %t | %stdoutcheck %s

x(){}

int printf(const char *, ...);

main()
{
	for(int i = 0; i < 5; i++){
		x();

		int j = 3;

		printf("%d, %d\n", i, j);
		j;
	}
}

// STDOUT: 0, 3
// STDOUT-NEXT: 1, 3
// STDOUT-NEXT: 2, 3
// STDOUT-NEXT: 3, 3
// STDOUT-NEXT: 4, 3
