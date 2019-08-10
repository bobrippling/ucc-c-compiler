// RUN: %ucc %s -o %t
// RUN: %t

struct A
{
	int i, j, k;
};

main()
{
	return sizeof(struct A) == 12 ? 0 : 1;
}
