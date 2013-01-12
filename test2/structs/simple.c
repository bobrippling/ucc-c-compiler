// RUN: %ucc %s -o %t
// RUN: %t

struct A
{
	int i, j, k;
};

main()
{
	return sizeof(struct A) == 16 ? 0 : 1;
}
