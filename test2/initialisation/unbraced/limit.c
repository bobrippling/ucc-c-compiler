// RUN: %ucc -c -o /dev/null %s
// RUN: %ucc -S -o- %s | grep 5; [ $? -ne 0 ]
// RUN: %ucc -S -o- %s | %asmcheck %s

a()
{
	int (*p)[] = (int[][2]){ 1, 2, { 3, 4, 5 } };
	// should be: { { 1, 2 }, { 3, 4 } } - 5 ignored
}

struct A
{
	int i, j;
};

b()
{
	struct A *p = (struct A []){
		1, 2,
		3, 4
	};
}

c()
{
	/* this creates two structs, { 1, 2 } and { 3, 0 }
	 * would've expected three { 1, 0 }, { 2, 0 } and { 3, 0 } ... ???
	 */
	struct A x[] = {
		1, { 2 }, 3
	};
}
