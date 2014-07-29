// RUN: %check -e %s -std=c89
// RUN: %check -e %s -std=c99

struct A
{
	int x[5];
	short n;
} f()
{
	return (struct A){
		{ 1, 2, 3, 4, 5 }, 5
	};
}

main()
{
	int (*p)[5] = &f().x; // CHECK: error: can't take the address of struct (int[5])
}
