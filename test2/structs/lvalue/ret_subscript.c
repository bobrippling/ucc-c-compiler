// RUN: %ucc -fsyntax-only -std=c99 %s
// RUN: %check -e %s -std=c89

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
	f().x[1]; // CHECK: error: mismatching types, subscript:
	*(f().x + 1); // CHECK: error: mismatching types, +:
}
