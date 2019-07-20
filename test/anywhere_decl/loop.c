// RUN: %ucc %s -o %t
// RUN: %t | %output_check '0, 3' '1, 3' '2, 3' '3, 3' '4, 3'

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
